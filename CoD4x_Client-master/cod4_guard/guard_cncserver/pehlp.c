#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>



void AcHlp_GetSectionBaseLen(unsigned char* imagestart, char* sectionname, DWORD* sectiondata, DWORD* sectionlen, DWORD *virtualaddr)
{
	DWORD dwSectionSize;
	DWORD dwSectionBase;
	DWORD dwVirtualAddr;
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
		if (strcmp((char*)sectionHeader.Name, sectionname) == 0)
		{
			// the values you need
			dwSectionBase = (DWORD)imagestart + sectionHeader.PointerToRawData;

			dwSectionSize = sectionHeader.SizeOfRawData;

			if(sectionHeader.Misc.VirtualSize > 0 && sectionHeader.Misc.VirtualSize < sectionHeader.SizeOfRawData)
			{
				dwSectionSize = sectionHeader.Misc.VirtualSize;
			}

			dwVirtualAddr = sectionHeader.VirtualAddress;
			break;
		}
	}

	if(i == nSections)
	{
		*sectionlen = 0;
		*sectiondata = 0;
		*virtualaddr = 0;
	}else{
		*sectiondata = dwSectionBase;
		*sectionlen = dwSectionSize;
		*virtualaddr = dwVirtualAddr;
	}
}



int ACHlp_LoadFile(unsigned char **data, const char* filename)
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
	unsigned char* buffer = malloc (sizeof(char) * (lSize +1));
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
	buffer[lSize] = 0;
	return lSize;
}


int ACHlp_GetNextLine(const char* content, int *filepos, char* ol, int len)
{
	int i;
	if(content[*filepos] == '\0')
	{
		return -1;
	}

	for(i = 0; content[*filepos] != '\n' && content[*filepos] != '\0'; ++i, ++*filepos)
	{
		if(i-1 < len)
		{
			ol[i] = content[*filepos];
		}
	}
	if(content[*filepos] == '\n')
	{
		++*filepos;
	}
	ol[i] = 0;
	return i;
}



int AcHlp_PerformRelocation(void* imagestart, DWORD imagebase) //Only for .text right now
{
	DWORD i, num_items;
	DWORD_PTR diff;
	IMAGE_BASE_RELOCATION* r;
	IMAGE_BASE_RELOCATION* r_end;
	WORD* reloc_item;
	IMAGE_DOS_HEADER dosHeader;
	IMAGE_NT_HEADERS ntHeader;
	bool hasDoSHeader = false;
	DWORD offset;

	dosHeader = *(IMAGE_DOS_HEADER*)(imagestart);

	if(dosHeader.e_magic == *(WORD*)"MZ")
	{
		hasDoSHeader = true;
	}

	if(hasDoSHeader)
	{
		// get nt header
		ntHeader = *(IMAGE_NT_HEADERS*)((DWORD)imagestart + dosHeader.e_lfanew);
	}else{
		return 0;
	}

	/* Do we need to do the base relocations? Are there any relocatable items? */
	if (imagebase == ntHeader.OptionalHeader.ImageBase)
	{
		return 1;
	}	
	if (!ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress ||
		!ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
	{
		return 0;
	}
	
	DWORD reloc_size = 0;
	DWORD reloc_virt;

	AcHlp_GetSectionBaseLen(imagestart, ".reloc", (DWORD*)&r, &reloc_size, &reloc_virt);

	if(reloc_size == 0)
	{
		return 0;
	}
printf("reloc_size = 0x%x\n", reloc_size);

	DWORD text;
	DWORD text_size = 0;
	DWORD text_virt;

	AcHlp_GetSectionBaseLen(imagestart, ".text", &text, &text_size, &text_virt);

	if(text_size == 0)
	{
		return 0;
	}

	diff = imagebase - ntHeader.OptionalHeader.ImageBase;
	r_end = (IMAGE_BASE_RELOCATION*)((DWORD_PTR)r + reloc_size - sizeof (IMAGE_BASE_RELOCATION));

	for (; r<r_end; r=(IMAGE_BASE_RELOCATION*)((DWORD_PTR)r + r->SizeOfBlock))
	{
			reloc_item = (WORD*)(r + 1);
			num_items = (r->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

			for (i=0; i<num_items; ++i, ++reloc_item)
			{
				switch (*reloc_item >> 12)
				{
				case IMAGE_REL_BASED_ABSOLUTE:
					break;
				case IMAGE_REL_BASED_HIGHLOW:
					offset = r->VirtualAddress + (*reloc_item & 0xFFF);

			//		printf("offset: %x\n", offset);

					if(offset >= text_virt && offset < text_virt + text_size)
					{
						offset -= text_virt;
						*((DWORD_PTR*)(text + offset)) += diff;
					}
					break;
				default:
					return 0;
				}
			}
	}
	

	return 1;
}