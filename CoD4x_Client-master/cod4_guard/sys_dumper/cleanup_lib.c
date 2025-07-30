#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>



void AcHlp_GetSectionBaseLen(unsigned char* imagestart, char* sectionname, bool virtual, DWORD* sectiondata, DWORD* sectionlen, DWORD* virtualsectionlen)
{
	DWORD dwSectionSize, dwVirtualSectionSize;
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
		if (strcmp((char*)sectionHeader.Name, sectionname) == 0)
		{
			// the values you need
			if(!virtual)
			{
				dwSectionBase = (DWORD)imagestart + sectionHeader.PointerToRawData;
				dwSectionSize = sectionHeader.SizeOfRawData;
				
			}else{
				dwSectionBase = (DWORD)imagestart + sectionHeader.VirtualAddress;
				dwSectionSize = sectionHeader.SizeOfRawData;
			}
			dwVirtualSectionSize = dwSectionSize;
			if(sectionHeader.Misc.VirtualSize > 0 && sectionHeader.Misc.VirtualSize < sectionHeader.SizeOfRawData)
			{
				dwVirtualSectionSize = sectionHeader.Misc.VirtualSize;
			}
			break;
		}
	}

	if(i == nSections)
	{
		*sectionlen = 0;
		*sectiondata = 0;
		*virtualsectionlen = 0;
	}else{
		*sectiondata = dwSectionBase;
		*sectionlen = dwSectionSize;
		*virtualsectionlen = dwVirtualSectionSize;
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
}




void AcHlp_RemoveHlpLoader(unsigned char* image, unsigned const char** data, int* imagelen)
{
	DWORD dwSectionSize, dwVirtSize;
	DWORD dwSectionBase;

//	unsigned char cipherkey[128];
//	int start;
	int i, z;

	AcHlp_GetSectionBaseLen(image, ".text", false, &dwSectionBase, &dwSectionSize, &dwVirtSize);

	if(dwSectionSize == 0)
	{
		MessageBoxA(NULL, "AcHlp_RemoveHlpLoader failed Section not found", "AcHlp_RemoveHlpLoader failed", MB_OK);
		exit(0);
	}

	*data = (unsigned char*)dwSectionBase;
	*imagelen = dwSectionSize;

	unsigned char* loc = (unsigned char*)dwSectionBase;
	//Search for the special instructions which are the markers
	for(i = 0; i < dwSectionSize; ++i)
	{
		if( ((DWORD*)&loc[i])[0] == 0xd289c089 && ((DWORD*)&loc[i])[1] == 0xdb89c989 && ((DWORD*)&loc[i])[2] == 0xff89c989 && ((DWORD*)&loc[i])[3] == 0xf689c989)
		{
			for(z = 0; z > -12; --z)
			{
				if(loc[i + z] == 0x55 && loc[i + z + 1] == 0x89 && loc[i + z + 2] == 0xe5)
				{
					loc[i + z] = 0xc3;
					loc[i + z +1] = 0xc3;
					loc[i + z +2] = 0xc3;
				}
			}
            memset(loc + i, 0x90, 16);

            for(z = 0;z < 1000; ++z, ++i)
            {
                if(((DWORD*)&loc[i])[0] == 0xd289c089)
                {
                    ((DWORD*)&loc[i])[0] = 0x90909090;
                    break;
                }
                loc[i] = 0x90;
                //clean function until lower marker
            }
            if(z == 1000)
            {
        		MessageBoxA(NULL, "AcHlp_RemoveHlpLoader failed end not found", "AcHlp_RemoveHlpLoader failed", MB_OK);
		        exit(0);
            }

		}
	}
}


void ACHlp_WriteToFile(unsigned const char *data, int len, const char* filename)
{
	char oldfilename[1024];
	snprintf(oldfilename, sizeof(oldfilename), "%s.todelete", filename);
	remove(oldfilename);
	rename(filename, oldfilename);

	FILE* fh = fopen(filename, "wb");
	if(!fh)
	{
		MessageBoxA(NULL, "ACHlp_WriteToFile failed  fopen(filename, \"wb\")", "ACHlp_WriteToFile failed", MB_OK);
		exit(0);
	}
	fwrite(data, 1, len, fh);
	fclose(fh);
	remove(oldfilename);
}


void AcHlp_Removal(const char* lpFilename)
{
	unsigned const char* textdata;
	int textlen;
	unsigned char* imagestart;

	int imagesize = ACHlp_LoadFile(&imagestart, lpFilename);
	if(imagesize == 0)
	{
        MessageBoxA(NULL, "Error loading shared library", "Error loading shared library", MB_OK);
		exit(-1);
	}

    AcHlp_RemoveHlpLoader(imagestart, &textdata, &textlen);


    ACHlp_WriteToFile(imagestart, imagesize, lpFilename);

}
