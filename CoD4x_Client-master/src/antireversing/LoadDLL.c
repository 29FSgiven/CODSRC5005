#include "LoadDLL.h"

#include <windows.h>
#include <winnt.h>
#include <stdlib.h>
#include <string.h>

#define LIBSEH_USE_COMPILER_SEH_IMPL

#include "../libseh-0.0.4/seh.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define READ_PROC(buff, pos, size) \
	__seh_try { \
		if (!read_proc((buff), (pos), (size), (read_proc_param))) \
			return ELoadDLLResult_ReadProcError; \
	} __seh_except (EXCEPTION_EXECUTE_HANDLER) { \
		return ELoadDLLResult_ReadProcError; \
	} \
	__seh_end_except


typedef struct LOAD_DLL_CONTEXT
{
	IMAGE_DOS_HEADER dos_hdr;
	IMAGE_NT_HEADERS hdr;

	IMAGE_SECTION_HEADER* sect;
	size_t file_offset_section_headers;
	size_t size_section_headers;

	DWORD_PTR image_base;
	void* image;

	HMODULE* loaded_import_modules_array;
	unsigned num_import_modules;
	unsigned import_modules_array_capacity;

	LPDLLMAIN dll_main;
} LOAD_DLL_CONTEXT;



static inline ELoadDLLResult LoadDLL_LoadHeaders(LOAD_DLL_CONTEXT* ctx, LOAD_DLL_READPROC read_proc, void* read_proc_param)
{
	/* Read and check the DOS header... */

	READ_PROC(&ctx->dos_hdr, 0, sizeof(ctx->dos_hdr));
	if (ctx->dos_hdr.e_magic != IMAGE_DOS_SIGNATURE || !ctx->dos_hdr.e_lfanew) 
		return ELoadDLLResult_InvalidImage;

	/* Read and check the NT header... */

	READ_PROC(&ctx->hdr, ctx->dos_hdr.e_lfanew, sizeof(ctx->hdr))
	if (ctx->hdr.Signature != IMAGE_NT_SIGNATURE ||
		ctx->hdr.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC ||
		!ctx->hdr.FileHeader.NumberOfSections)
		return ELoadDLLResult_InvalidImage;

	/* Allocating memory and reading section headers. */

	ctx->size_section_headers = ctx->hdr.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
	ctx->sect = (IMAGE_SECTION_HEADER*)malloc(ctx->size_section_headers);
	if (!ctx->sect)
		return ELoadDLLResult_MemoryAllocationError;

	ctx->file_offset_section_headers = ctx->dos_hdr.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + ctx->hdr.FileHeader.SizeOfOptionalHeader;
	READ_PROC(ctx->sect, ctx->file_offset_section_headers, ctx->size_section_headers);

	return ELoadDLLResult_OK;
}

static inline ELoadDLLResult LoadDLL_AllocateMemory(LOAD_DLL_CONTEXT* ctx, int flags)
{
	DWORD rva_low, rva_high, i;
	IMAGE_SECTION_HEADER* s;

	/*
	// We do not trust the value of hdr.OptionalHeader.SizeOfImage so we calculate our own SizeOfImage.
	// This is the size of the continuous memory block that can hold the headers and all sections.
	// We will search the lowest and highest RVA among the section boundaries. If we must load the
	// header then its RVA will be the lowest (zero) and its size will be (file_offset_section_headers + size_section_headers).
	*/

	rva_low = (flags & ELoadDLLFlag_NoHeaders) ? 0xFFFFFFFF : 0;
	rva_high = 0;

	for (i=0,s=ctx->sect; i<ctx->hdr.FileHeader.NumberOfSections; ++i,++s)
	{
		if (!s->Misc.VirtualSize)
			continue;
		if (s->VirtualAddress < rva_low)
			rva_low = s->VirtualAddress;
		if ((s->VirtualAddress + s->Misc.VirtualSize) > rva_high)
			rva_high = s->VirtualAddress + s->Misc.VirtualSize;
	}

	/*
	// RVAlow and RVA high holds the boundaries of the memory block needed to load the sections & header of the image.
	// Now we allocate the memory block to load the sections and additionally the headers. We will try to allocate
	// the block at the preferred image base to avoid doing the base relocations.
	*/

	ctx->image = VirtualAlloc(
		(LPVOID)(ctx->hdr.OptionalHeader.ImageBase + rva_low),
		rva_high - rva_low,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE
		);

	ctx->image_base = ctx->hdr.OptionalHeader.ImageBase;

	/*
	// Note: image may differ from the address (LPVOID)(hdr.OptionalHeader.ImageBase + RVAlow)
	// because windows rounds down addresses to next page boundary.
	*/

	if (!ctx->image)
	{
		/*
		// If allocation at preferred imagebase failed then we let windows to allocate memory at arbitrary location.
		// Unfortunately we can not skip doing base relocations so we are in crap if the relocation info is stripped
		// from the DLL. :(
		*/

		if (ctx->hdr.FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
			return ELoadDLLResult_RelocationError;

		ctx->image = VirtualAlloc(
			NULL, 
			rva_high - rva_low,
			MEM_COMMIT | MEM_RESERVE, 
			PAGE_EXECUTE_READWRITE);

		ctx->image_base = (DWORD_PTR)ctx->image - rva_low;
	}

	if (!ctx->image)
		return ELoadDLLResult_MemoryAllocationError;

	return ELoadDLLResult_OK;
}

static inline ELoadDLLResult LoadDLL_LoadSections(LOAD_DLL_CONTEXT* ctx, LOAD_DLL_READPROC read_proc, void* read_proc_param, int flags)
{
	DWORD i;
	DWORD section_size;
	IMAGE_SECTION_HEADER* s;

	/* Loading the header if required. */

	if (!(flags & ELoadDLLFlag_NoHeaders))
	{
		READ_PROC((void*)ctx->image_base, 0, ctx->file_offset_section_headers + ctx->size_section_headers);
	}

	/* Loading the sections. */

	for (i=0,s=ctx->sect; i<ctx->hdr.FileHeader.NumberOfSections; ++i,++s)
	{
		section_size = MIN(s->Misc.VirtualSize, s->SizeOfRawData);
		if (section_size)
		{
			READ_PROC((void*)(s->VirtualAddress + ctx->image_base),	s->PointerToRawData, section_size);
		}
	}

	return ELoadDLLResult_OK;
}

static inline ELoadDLLResult LoadDLL_PerformRelocation(LOAD_DLL_CONTEXT* ctx)
{
	DWORD i, num_items;
	DWORD_PTR diff;
	IMAGE_BASE_RELOCATION* r;
	IMAGE_BASE_RELOCATION* r_end;
	WORD* reloc_item;

	/* Do we need to do the base relocations? Are there any relocatable items? */

	if (ctx->image_base == ctx->hdr.OptionalHeader.ImageBase)
		return ELoadDLLResult_OK;
		
	if (!ctx->hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress ||
		!ctx->hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
		return ELoadDLLResult_RelocationError;

	__seh_try
	{
		diff = ctx->image_base - ctx->hdr.OptionalHeader.ImageBase;
		r = (IMAGE_BASE_RELOCATION*)(ctx->image_base + ctx->hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
		r_end = (IMAGE_BASE_RELOCATION*)((DWORD_PTR)r + ctx->hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size - sizeof (IMAGE_BASE_RELOCATION));

		for (; r<r_end; r=(IMAGE_BASE_RELOCATION*)((DWORD_PTR)r + r->SizeOfBlock))
		{
			reloc_item = (WORD*)(r + 1);
			num_items = (r->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

			for (i=0; i<num_items; ++i,++reloc_item)
			{
				switch (*reloc_item >> 12)
				{
				case IMAGE_REL_BASED_ABSOLUTE:
					break;
				case IMAGE_REL_BASED_HIGHLOW:
					*(DWORD_PTR*)(ctx->image_base + r->VirtualAddress + (*reloc_item & 0xFFF)) += diff;
					break;
				default:
					return ELoadDLLResult_BadRelocationTable;
				}
			}
		}
	}
	__seh_except (EXCEPTION_EXECUTE_HANDLER)
	{
		return ELoadDLLResult_BadRelocationTable;
	}
	__seh_end_except

	return ELoadDLLResult_OK;
}


static inline ELoadDLLResult LoadDLL_ResolveImports(LOAD_DLL_CONTEXT* ctx)
{
	IMAGE_IMPORT_DESCRIPTOR* import_desc;
	HMODULE hDLL;
	DWORD_PTR* src_iat;
	DWORD_PTR* dest_iat;
	HMODULE* new_module_array = NULL;

	if (!ctx->hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress ||
		!ctx->hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
		return ELoadDLLResult_OK;

	import_desc = (IMAGE_IMPORT_DESCRIPTOR*)(ctx->image_base + ctx->hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	for (; import_desc->Name; ++import_desc)
	{
		hDLL = LoadLibraryA((char*)(ctx->image_base + import_desc->Name));
		if (!hDLL)
			return ELoadDLLResult_ImportModuleError;

		if (ctx->num_import_modules >= ctx->import_modules_array_capacity)
		{
			ctx->import_modules_array_capacity = ctx->import_modules_array_capacity ? ctx->import_modules_array_capacity*2 : 16;
			new_module_array = (HMODULE*)malloc(sizeof(HMODULE)*ctx->import_modules_array_capacity);
			if (!new_module_array)
				return ELoadDLLResult_MemoryAllocationError;
			if (ctx->num_import_modules)
				memcpy(new_module_array, ctx->loaded_import_modules_array, sizeof(HMODULE)*ctx->num_import_modules);
			free(ctx->loaded_import_modules_array);
			ctx->loaded_import_modules_array = new_module_array;
		}
		if(new_module_array)
		{
			new_module_array[ctx->num_import_modules++] = hDLL;
		}
		/*
		// We do not trust bound imports. If FirstThunk is not bound then we use that to find the function ordinals/names.
		// If it's bound we must use OriginalFirstThunk. If FirstThunk is bound and OriginalFirstThunk is not present then
		// we return with error.
		*/

		src_iat = dest_iat = (DWORD_PTR*)(ctx->image_base + import_desc->FirstThunk);

		if (import_desc->TimeDateStamp)
		{
			if (!import_desc->OriginalFirstThunk)
				return ELoadDLLResult_BoundImportDirectoriesNotSupported;
			src_iat = (DWORD_PTR*)(ctx->image_base + import_desc->OriginalFirstThunk);
		}

		for (; *src_iat; ++src_iat,++dest_iat)
		{
			*dest_iat = (DWORD_PTR)GetProcAddress(hDLL, (const char*)((*src_iat & IMAGE_ORDINAL_FLAG) ? IMAGE_ORDINAL(*src_iat) : ctx->image_base + *src_iat + 2));
			if (!*dest_iat)
				return ELoadDLLResult_ImportFunctionError;
		}
	}

	return ELoadDLLResult_OK;
}


static inline ELoadDLLResult LoadDLL_SetSectionMemoryProtection(LOAD_DLL_CONTEXT* ctx)
{
	IMAGE_SECTION_HEADER* s;
	DWORD i, protection;

	/*
	// Setting the protection of memory pages. We leave the protection of header PAGE_EXECUTE_READWRITE.
	// Note: We must convert the section characterictics flag into memory page protection flag of VirtualProtect().
	// In the characterictics flags of a section there are 4 bits that are important for us now:
	//
	// IMAGE_SCN_CNT_CODE       0x00000020
	// IMAGE_SCN_MEM_EXECUTE    0x20000000
	// IMAGE_SCN_MEM_READ       0x40000000
	// IMAGE_SCN_MEM_WRITE      0x80000000
	//
	// IMAGE_SC_CNT_CODE is equivalent to (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ)
	// so we have only 3 flags that means 8 cases.
	*/

	for (i=0,s=ctx->sect; i<ctx->hdr.FileHeader.NumberOfSections; ++i,++s)
	{
		if (s->Characteristics & IMAGE_SCN_CNT_CODE)
			s->Characteristics |= IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

		/* We rotate the upper 3 important bits down so the resulting value is in the range 0-7. */
		switch ((DWORD)s->Characteristics >> (32-3))
		{
			/* meaning of bits: 1: execute, 2: read, 4: write */
		case 1: protection = PAGE_EXECUTE; break;

		case 0: // case 0: should we use PAGE_NOACCESS? What is it good for?
		case 2: protection = PAGE_READONLY; break;

		case 3: protection = PAGE_EXECUTE_READ; break;

		case 4:
		case 6: protection = PAGE_READWRITE; break;

		case 5:
		default: protection = PAGE_EXECUTE_READWRITE; break;
		}

		if (!VirtualProtect(
			(LPVOID)(ctx->image_base + s->VirtualAddress),
			s->Misc.VirtualSize,
			protection,
			&protection
			))
			return ELoadDLLResult_ErrorSettingMemoryProtection;
	}

	return ELoadDLLResult_OK;
}

static inline ELoadDLLResult LoadDLL_CallDLLEntryPoint(LOAD_DLL_CONTEXT* ctx, int flags)
{
	/*
	// Do we have to call the entrypoint of the DLL? If yes then we do the DLL_PROCESS_ATTACH
	// notification but if it returns FALSE we unload the whole image immediately.
	*/

	if (flags & ELoadDLLFlag_NoEntryCall)
		return ELoadDLLResult_OK;

	if (ctx->hdr.OptionalHeader.AddressOfEntryPoint)
	{
		ctx->dll_main = (LPDLLMAIN)(ctx->hdr.OptionalHeader.AddressOfEntryPoint + ctx->image_base);
		__seh_try
		{
			if (!ctx->dll_main(ctx->image_base, DLL_PROCESS_ATTACH, NULL))
				return ELoadDLLResult_DllMainCallError;
		}
		__seh_except (EXCEPTION_EXECUTE_HANDLER)
		{
			return ELoadDLLResult_DllMainCallError;
		}
		__seh_end_except
	}

	return ELoadDLLResult_OK;
}

ELoadDLLResult LoadDLL(LOAD_DLL_READPROC read_proc, void* read_proc_param, int flags, LOAD_DLL_INFO* info)
{
	LOAD_DLL_CONTEXT ctx;
	ELoadDLLResult res;
	BOOL finished_successfully = FALSE;
	unsigned i;

	if (!read_proc)
		return ELoadDLLResult_WrongFunctionParameters;

	ctx.sect = NULL;
	ctx.loaded_import_modules_array = NULL;
	ctx.import_modules_array_capacity = 0;
	ctx.num_import_modules = 0;
	ctx.dll_main = NULL;
	__seh_try
	{
		__seh_try
		{
			res = LoadDLL_LoadHeaders(&ctx, read_proc, read_proc_param);
			if (res != ELoadDLLResult_OK)
				return res;

			res = LoadDLL_AllocateMemory(&ctx, flags);
			if (res != ELoadDLLResult_OK)
				return res;

			__seh_try
			{
				res = LoadDLL_LoadSections(&ctx, read_proc, read_proc_param, flags);
				if (res != ELoadDLLResult_OK)
					return res;

				res = LoadDLL_PerformRelocation(&ctx);
				if (res != ELoadDLLResult_OK)
					return res;

				res = LoadDLL_ResolveImports(&ctx);
				if (res != ELoadDLLResult_OK)
					return res;

				res = LoadDLL_SetSectionMemoryProtection(&ctx);
				if (res != ELoadDLLResult_OK)
					return res;

				res = LoadDLL_CallDLLEntryPoint(&ctx, flags);
				if (res != ELoadDLLResult_OK)
					return res;

				/* We finished!!! :) Filling in the callers info structure... */

				if (info)
				{
					__seh_try
					{
						info->size = sizeof(*info);
						info->flags = flags;
						info->image_base = ctx.image_base;
						info->mem_block = ctx.image;
						info->dll_main = ctx.dll_main;
						info->export_dir_rva = ctx.hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
						info->loaded_import_modules_array = ctx.loaded_import_modules_array;
						info->num_import_modules = ctx.num_import_modules;
					}
					__seh_except (EXCEPTION_EXECUTE_HANDLER) 
					{
						return ELoadDLLResult_WrongFunctionParameters;
					}
					__seh_end_except
				}

				finished_successfully = TRUE;
				return ELoadDLLResult_OK;
			}
			__seh_finally
			{
				if (!finished_successfully)
					VirtualFree(ctx.image, 0, MEM_RELEASE);

				if (!finished_successfully || !info)
				{
					if (ctx.loaded_import_modules_array)
					{
						for (i=0; i<ctx.num_import_modules; ++i)
							FreeLibrary(ctx.loaded_import_modules_array[i]);
						free(ctx.loaded_import_modules_array);
					}
				}
			}
			__seh_end_finally
		}
		__seh_finally
		{
			if (ctx.sect)
				free(ctx.sect);
		}
		__seh_end_finally
	}
	__seh_except (EXCEPTION_EXECUTE_HANDLER)
	{
		return ELoadDLLResult_UnknownError;
	}
	__seh_end_except
	return ELoadDLLResult_UnknownError;
}


//------------------------------------------------------------------------------

BOOL UnloadDLL(LOAD_DLL_INFO* info)
{
	unsigned i;
	BOOL res = TRUE;

	__seh_try
	{
		if (!info || info->size!=sizeof(*info) || !info->image_base || !info->mem_block)
			return FALSE;

		if (info->loaded_import_modules_array)
		{
			for (i=0; i<info->num_import_modules; ++i)
				FreeLibrary(info->loaded_import_modules_array[i]);
			free(info->loaded_import_modules_array);
		}

		if (!(info->flags & ELoadDLLFlag_NoEntryCall) && info->dll_main)
		{
			__seh_try
			{
				res = info->dll_main(info->image_base, DLL_PROCESS_DETACH, NULL);
			}
			__seh_except (EXCEPTION_EXECUTE_HANDLER) 
			{
				res = FALSE;
			}
			__seh_end_except
		}

		VirtualFree(info->mem_block, 0, MEM_RELEASE);
		return res;
	}
	__seh_except (EXCEPTION_EXECUTE_HANDLER) 
	{
		return FALSE;
	}
	__seh_end_except
}


//------------------------------------------------------------------------------

/*
// LoadDLLFromFile...
*/

typedef struct _LOAD_DLL_FROM_FILE_STRUCT {
	FILE*	f;
	size_t	dll_offset;
	size_t	dll_size;
} LOAD_DLL_FROM_FILE_STRUCT;

static inline BOOL LoadDLLFromFileCallback(void* buff, size_t position, size_t size, LOAD_DLL_FROM_FILE_STRUCT* param)
{
	if (!size)
		return TRUE;
	if ((position + size) > param->dll_size)
		return FALSE;
	fseek(param->f, param->dll_offset + position, SEEK_SET);
	return fread(buff, 1, size, param->f) == size;
}


ELoadDLLResult LoadDLLFromCFile(FILE* f, size_t dll_offset, size_t dll_size, int flags, LOAD_DLL_INFO* info)
{
	LOAD_DLL_FROM_FILE_STRUCT ldffs = { f, dll_offset, dll_size };
	return LoadDLL((LOAD_DLL_READPROC)&LoadDLLFromFileCallback, &ldffs, flags, info);
}


ELoadDLLResult LoadDLLFromFileNameOffset(const char* filename, size_t dll_offset, size_t dll_size, int flags, LOAD_DLL_INFO* info)
{
	ELoadDLLResult res;
	FILE* f = fopen(filename, "rb");
	if (!f)
		return ELoadDLLResult_DLLFileNotFound;
	res = LoadDLLFromCFile(f, dll_offset, dll_size, flags, info);
	fclose(f);
	return res;
}

ELoadDLLResult LoadDLLFromFileName(const char* filename, int flags, LOAD_DLL_INFO* info)
{
	return LoadDLLFromFileNameOffset(filename, 0, DLL_SIZE_UNKNOWN, flags, info);
}


/*
// LoadDLLFromMemory()
*/

typedef struct _LOAD_DLL_FROM_MEMORY_STRUCT {
	const void*	dll_data;
	size_t		dll_size;
} LOAD_DLL_FROM_MEMORY_STRUCT;


static inline BOOL LoadDLLFromMemoryCallback(void* buff, size_t position, size_t size, LOAD_DLL_FROM_MEMORY_STRUCT* param)
{
	if (!size)
		return TRUE;
	if ((position + size) > param->dll_size)
		return FALSE;
	memcpy(buff, (char*)param->dll_data + position, size);
	return TRUE;
}


ELoadDLLResult LoadDLLFromMemory(const void* dll_data, size_t dll_size, int flags, LOAD_DLL_INFO* info)
{
	LOAD_DLL_FROM_MEMORY_STRUCT ldfms = { dll_data, dll_size };
	return LoadDLL ((LOAD_DLL_READPROC)&LoadDLLFromMemoryCallback, &ldfms, flags, info);
}


//------------------------------------------------------------------------------

/*
// GetProcAddress functions:
*/


FARPROC myGetProcAddress_LoadDLLInfo(LOAD_DLL_INFO* info, const char* func_name)
{
	return MyGetProcAddress_ExportDir(info->export_dir_rva, info->image_base, func_name);
}

FARPROC MyGetProcAddress(HMODULE module, const char* func_name)
{
	IMAGE_NT_HEADERS* hdr;
	__seh_try
	{
		if (((IMAGE_DOS_HEADER*)module)->e_magic != IMAGE_DOS_SIGNATURE)
			return NULL;
		hdr = (IMAGE_NT_HEADERS*)((DWORD_PTR)module + ((IMAGE_DOS_HEADER*)module)->e_lfanew);

		if (hdr->Signature != IMAGE_NT_SIGNATURE || hdr->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
			return NULL;

		return MyGetProcAddress_ExportDir(
			hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
			(DWORD_PTR)module,
			func_name
			);
	}
	__seh_except (EXCEPTION_EXECUTE_HANDLER) 
	{
		return NULL;
	}
	__seh_end_except
}


FARPROC MyGetProcAddress_ExportDir(DWORD export_dir_rva, DWORD_PTR image_base, const char* func_name)
{
	IMAGE_EXPORT_DIRECTORY* exp;
	DWORD_PTR ord;
	DWORD i;

	if (!export_dir_rva)
		return NULL;
	exp = (IMAGE_EXPORT_DIRECTORY*)(image_base + export_dir_rva);
	ord = (DWORD_PTR)func_name;

	__seh_try
	{
		if (ord < 0x10000)
		{
			/* Search for ordinal. */

			if (ord < exp->Base)
				return NULL;
			ord -= exp->Base;
		}
		else
		{
			/* Search for name. */

			for (i=0; i<exp->NumberOfNames; ++i)
			{
				if ( !strcmp( (char*)(((DWORD*)(exp->AddressOfNames + image_base))[i] + image_base), func_name) )
				{
					ord = ((WORD*)(exp->AddressOfNameOrdinals + image_base))[i];
					break;    
				}
			}
		}

		if (ord >= exp->NumberOfFunctions)
			return NULL;
		return (FARPROC)(((DWORD*)(exp->AddressOfFunctions + image_base))[ord] + image_base);

	}
	__seh_except (EXCEPTION_EXECUTE_HANDLER)
	{
		return NULL;
	}
	__seh_end_except
}
