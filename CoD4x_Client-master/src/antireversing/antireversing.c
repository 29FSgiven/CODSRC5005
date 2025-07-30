#include "antireversing.h"

#include <windows.h>
#include <winternl.h>
#include "../q_shared.h"
#include "../r_shared.h"
#include "../exeobfus/exeobfus.h"
#include "../libudis86/extern.h"
#include <time.h>
#include <winbase.h>
#include <assert.h>
#include <tchar.h>
// #include "d3d9vtable.h"
// #include "loadlibrary.h"
#include "LoadDLL.h"

typedef LOAD_DLL_INFO* MODULE_HANDLE;

/*MODULE_HANDLE LoadModule(const char* dll_path)
{
	LOAD_DLL_INFO* p = new LOAD_DLL_INFO;
	DWORD res = LoadDLLFromFileName(dll_path, 0, p);
	if (res != ELoadDLLResult_OK)
	{
		delete p;
		return NULL;
	}
	return p;
}




bool UnloadModule(MODULE_HANDLE handle)
{
	bool res = FALSE != UnloadDLL(handle);
	delete handle;
	return res;
}*/

void* GetModuleFunction(MODULE_HANDLE handle, const char* func_name)
{
	return (void*)myGetProcAddress_LoadDLLInfo(handle, func_name);
}


/*__inline__ int readfile(unsigned char **data, char* filename)
{
	FILE* fh = fopen(filename, "rb");
	if(!fh)
	{
		*data = NULL;
		return 0;
	}
	// obtain file size:
	fseek (fh , 0 , SEEK_END);
	int lSize = ftell (fh);
	rewind (fh);
	// allocate memory to contain the whole file:
	unsigned char* buffer = malloc (sizeof(char)*lSize);
	if (buffer == NULL)
	{
		fclose(fh);
		*data = NULL;
		return 0;
	}
	if (fread(buffer, 1, lSize, fh) != lSize)
	{
		fclose(fh);
		free(buffer);
		*data = NULL;
		return 0;
	}
	fclose(fh);
	*data = buffer;
	return lSize;
}*/


/*__inline__ DWORD findVirtualAddress(unsigned char* imagestart, DWORD virtualaddress)
{
	DWORD dwSectionSize;
	DWORD dwSectionBase;
	int i;
	IMAGE_SECTION_HEADER sectionHeader;
	IMAGE_DOS_HEADER dosHeader;
	IMAGE_NT_HEADERS ntHeader;
	int nSections;
	IMAGE_FILE_HEADER imageHeader;
	bool hasDoSHeader = false;
	dosHeader = *(IMAGE_DOS_HEADER*)(imagestart);
	if(dosHeader.e_magic == *(WORD*)"MZ")
	{
		hasDoSHeader = true;
	}
	if(hasDoSHeader)
	{
		// get nt header
		ntHeader = *(IMAGE_NT_HEADERS*)((DWORD)imagestart + dosHeader.e_lfanew);
		imageHeader = ntHeader.FileHeader;
	}else{
		imageHeader = *(IMAGE_FILE_HEADER*)(imagestart);
	}
	nSections = imageHeader.NumberOfSections;
	// iterate through section headers and search for "sectionname" section
	for ( i = 0; i < nSections; i++)
	{
		if(hasDoSHeader)
		{
			sectionHeader = *(IMAGE_SECTION_HEADER*) ((DWORD)imagestart + dosHeader.e_lfanew + sizeof(IMAGE_NT_HEADERS) + i*sizeof(IMAGE_SECTION_HEADER));
		}else{
			sectionHeader = *(IMAGE_SECTION_HEADER*) ((DWORD)imagestart + sizeof(IMAGE_FILE_HEADER) + i*sizeof(IMAGE_SECTION_HEADER));
		}
		if (virtualaddress >= sectionHeader.VirtualAddress && virtualaddress < sectionHeader.VirtualAddress + sectionHeader.SizeOfRawData) // if virtual address in section
		{
			// found correct section 
            DWORD sectionOffset = virtualaddress - sectionHeader.VirtualAddress;
            return sectionOffset + sectionHeader.PointerToRawData;
		}
	}
	
  return NULL;
}*/

/*static inline char adbg_BeingDebuggedPEB(void)
{
	PPEB      pPEB  = 0;
	uintptr_t pHEAP = 0;
	
	// warning, shit's about to get dirty
	__asm__("movl %%fs:0x30, %0" : "=r" (pPEB) : : ); // get the PEB pointer using evil assembly voodoo
	pHEAP = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(pPEB) + 0x18); // get the HEAP pointer using other evil stuff like typecasting and offset addition o_O
	uint32_t ForceFlags   = *reinterpret_cast<uint32_t*>(pHEAP + 0x44); // more ugly hacking, can't we just use a GUI? :-(
	uint32_t Flags        = *reinterpret_cast<uint32_t*>(pHEAP + 0x40); // make enterprise developers cry
	int32_t  NtGlobalFlag = *reinterpret_cast<int32_t*> (reinterpret_cast<uintptr_t>(pPEB) + 0x68); // i can read this, can you?
	
	if(pPEB->BeingDebugged || NtGlobalFlag || (Flags & ~HEAP_GROWABLE) || ForceFlags)
	{
		// you are being debugged
	}
}*/




static inline unsigned int crc32add(unsigned int val, unsigned int crc) 
{
    int j;
#ifndef COD4XDEV
    DISASM_MISALIGN;
#endif // COD4XDEV
    crc = crc ^ val;
    for (j = 7; j >= 0; j--)     // Do eight times.
        crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));

    return ~crc;
}

unsigned int computeCrcFromFunc(unsigned char* funstart)
{
    ud_t ud_obj;

	ud_init(&ud_obj);
	//ud_set_input_file(&ud_obj, stdin);
    ud_set_input_buffer(&ud_obj, funstart, 1000);
	ud_set_mode(&ud_obj, 32);
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);

    unsigned int crc = 0xFFFFFFFF;

	while (ud_disassemble(&ud_obj)) 
    {
        if(ud_obj.mnemonic == UD_Iint3) // break at function end
            break;

		//printf("\t%s\n", ud_insn_asm(&ud_obj));
        //Com_Printf("\t%s\n", ud_insn_asm(&ud_obj));

        crc = crc32add(ud_obj.mnemonic, crc);
	}

    return crc;
}

/*#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <malloc.h>

#include "../../MemoryModule.h"
*/

static inline unsigned char* ReadFileBinary(char *name)
{
	FILE *file;
	unsigned char *buffer;
	unsigned long fileLen;
	char buf[256];

	//Open file
	PROTECTSTRING("rb", buf);
	file = fopen(name, buf);
	if (!file)
	{
		return NULL;
	}
	
	//Get file length
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	fseek(file, 0, SEEK_SET);

	//Allocate memory
	buffer=(unsigned char *)malloc(fileLen+1);
	if (!buffer)
	{
		fclose(file);
		return NULL;
	}

	//Read file contents into buffer
	fread(buffer, fileLen, 1, file);
	fclose(file);

	return buffer;
}

typedef IDirect3D9* WINAPI pDirect3DCreate9(UINT);
//typedef IDirect3D9*(*pDirect3DCreate9)(UINT);

static inline IDirect3DDevice9* CreateSearchDevice(IDirect3D9* pD3D)
{
    HWND hWindow = GetDesktopWindow();

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory( &pp, sizeof( pp ) );
    pp.Windowed = TRUE;
    pp.hDeviceWindow = hWindow;
    pp.BackBufferCount = 1;
    pp.BackBufferWidth = 4;
    pp.BackBufferHeight = 4;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    
    IDirect3DDevice9* pDevice = NULL;

    if( SUCCEEDED( pD3D->lpVtbl->CreateDevice( pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &pDevice ) ) ) 
    {
        return pDevice;
    }
    else
    {
        return NULL;
    }
}

unsigned int computeCrcFromDrawIndexedPrimitiveDummy()
{
	unsigned char* buffer = NULL;
    char dllpath[MAX_PATH];
	// HMODULE handle = NULL;
    //MODULE_HANDLE handle = NULL;
	pDirect3DCreate9* createD3D = NULL;
	IDirect3D9* pD3D = NULL;
    IDirect3DDevice9* pD3DDevice = NULL;
	unsigned int crc = 0;
    DWORD res = ELoadDLLResult_UnknownError;
	char buf[128];

	PROTECTSTRING("d3d9.dll", buf);
    HMODULE d3d9module = GetModuleHandleA(buf);
    if(!d3d9module)
	{
		goto exit;
	}

    GetModuleFileNameA(d3d9module, dllpath, MAX_PATH);


    // MessageBoxA(NULL, dllpath, "load", MB_OK);

    LOAD_DLL_INFO dllInfo;
    res = LoadDLLFromFileName(dllpath, 0, &dllInfo);
    if (res != ELoadDLLResult_OK)
    {
        goto exit;
    }

    // MessageBoxA(NULL, dllpath, "loaded", MB_OK);

    // handle = LoadModule(dllpath);

	/*buffer = ReadFileBinary(dllpath);
	if(!buffer)
	{
		// Com_Printf("failed to load file d3d9.dll\n");
		goto exit;
	}

	handle = mem_load_library(buffer);

	if(!handle)
	{
		// Com_Printf("failed to load library d3d9.dll\n");
		goto exit;
	}*/

	PROTECTSTRING("Direct3DCreate9", buf);

	// createD3D = ( pDirect3DCreate9 *) get_proc_address(handle, buf);
    createD3D = ( pDirect3DCreate9 *) GetModuleFunction(&dllInfo, buf);

	if(!createD3D)
	{
		// Com_Printf("failed to import Direct3DCreate9\n");
		goto exit;
	}

	//HMODULE handle = LoadLibraryA("d3d9.dll"); // CustomLoadLibrary();
	//Com_Printf("Module: %x\n", handle);
	//createD3D = ( pDirect3DCreate9 *)GetProcAddress(handle, "Direct3DCreate9");

	pD3D = createD3D(D3D_SDK_VERSION);

	if(!pD3D)
	{
		// Com_Printf("Direct3DCreate9 failed\n");
		goto exit;
	}

	pD3DDevice = CreateSearchDevice(pD3D);

	if(!pD3DDevice)
	{
		// Com_Printf("CreateDevice failed\n");
		goto exit;
	}

	//Com_Printf("createD3D: %x\n", pD3DDevice);
	int* pInterface = (int*)*((int*)pD3DDevice);
    crc = computeCrcFromFunc((void*)pInterface[82]);

	// Com_Printf("DrawIndexedPrimitive CRC32: %x\n", crc);

exit:
	if(pD3DDevice)
		pD3DDevice->lpVtbl->Release(pD3DDevice);
	if(pD3D)
		pD3D->lpVtbl->Release(pD3D);
	if(res == ELoadDLLResult_OK)
		//mem_free_library(handle);
        //UnloadModule(handle);
        UnloadDLL(&dllInfo);
	if(buffer)
		free(buffer);

	return crc;
}