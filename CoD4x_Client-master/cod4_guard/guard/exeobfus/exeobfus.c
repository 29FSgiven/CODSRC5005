


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

#include "exeobfus.h"
#include "../../../src/libudis86/extern.h"

#include <time.h>
#include <winbase.h>

#define EXEOBFUS_CIPHER_STATIC_STRINGS Sys_CreateWindow


INLINE void EXEOBFUS_GETSECTIONBASELEN(unsigned char* imagestart, char* sectionname, bool virtual, DWORD* sectiondata, DWORD* sectionlen)
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
			break;
		}
	}

	if(i == nSections)
	{
		*sectionlen = 0;
		*sectiondata = 0;
	}else{
		*sectiondata = dwSectionBase;
		*sectionlen = dwSectionSize;
	}
}

INLINE int EXEOBFUS_READ_FROM_FILE(unsigned char **data, char* filename)
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


INLINE void EXEOBFUS_WRITE_SECTION_TO_FILE(unsigned const char *data, int len, char* filename ,char* sectionname)
{

	DWORD dwSectionSize;
	DWORD dwSectionBase;
	int lSize;
	unsigned char* buffer;


	lSize = EXEOBFUS_READ_FROM_FILE(&buffer, filename);
	if(lSize == 0)
	{
		exit(3);
	}
	//1st find the rdata section...
	EXEOBFUS_GETSECTIONBASELEN(buffer, sectionname, false, &dwSectionBase, &dwSectionSize);

	if(dwSectionSize <= 0 || dwSectionSize != len)
	{
		exit(4);
	}

	memcpy( (void*)dwSectionBase, data, len);
	remove("todelete");
	rename(filename, "todelete");

	FILE* fh = fopen(filename, "wb");
	if(!fh)
	{
		MessageBoxA(NULL, "EXEOBFUS_CIPHER_STATIC_STRINGS failed  fopen(filename, \"wb\")", "EXEOBFUS_CIPHER_STATIC_STRINGS failed", MB_OK);
		exit(0);
	}
	fwrite(buffer, 1, lSize, fh);
	fclose(fh);
	remove("todelete");
}



INLINE void EXEOBFUS_CIPHER_STATIC_STRINGS(unsigned char* image, unsigned const char** data, int* imagelen)
{
	//1st find the rdata section...
	// get dos header
	DWORD dwSectionSize;
	DWORD dwSectionBase;

	unsigned char cipherkey[128];
	int start;
	int i, k, len, c;


	EXEOBFUS_GETSECTIONBASELEN(image, ".rdata", false, &dwSectionBase, &dwSectionSize);


	if(dwSectionSize == 0)
	{
		MessageBoxA(NULL, "EXEOBFUS_CIPHER_STATIC_STRINGS failed Section not found", "EXEOBFUS_CIPHER_STATIC_STRINGS failed", MB_OK);
		exit(0);
	}

	char* loc = (char*)dwSectionBase;
	*data = (unsigned char*)dwSectionBase;
	*imagelen = dwSectionSize;

	srand (time(NULL));
	//Search for crypto strings
	for(i = 0; i < dwSectionSize; ++i)
	{
		if(loc[i] == EXEOBFUS_CIPHERKEYHEADER[0] && strncmp(&loc[i], EXEOBFUS_CIPHERKEYHEADER, EXEOBFUS_CIPHERKEYHEADERLEN) == 0)
		{
			start = i;
			//If a crypto string is found... Create a random key with EXEOBFUS_CIPHERKEYLEN
			for(k = 0; k < EXEOBFUS_CIPHERKEYLEN; ++k)
			{
				cipherkey[k] = rand();
			}
			memcpy(&loc[i], cipherkey, EXEOBFUS_CIPHERKEYLEN);

			i += EXEOBFUS_CIPHERKEYLEN;

			len = strlen(&loc[i +3]) +1;
			loc[i] = '\0';
			++i;

			*((unsigned short*)&loc[i]) = len;

			//printf("Crypt: %s\n", &loc[i +2]);

			for(k = 0, c = 0; k < len +2; ++k, ++c)
			{
				if(c >= EXEOBFUS_CIPHERKEYLEN)
				{
					c = 0;
				}
				loc[ i + k ] ^= loc[start + c];

			}

			for(k = 0; k < EXEOBFUS_CIPHERKEYLEN; ++k)
			{
				loc[ start + k ] ^= EXEOBFUS_STATICKEY[k];
			}

		}

	}

}

void EXEOBFUS_WRITE_CRC_CHECKSUMS(unsigned char* image, unsigned const char** data, int* imagelen);

void EXEOBFUS_STATIC_CODE_CRYPTER_INIT(char* lpFilename)
{
	unsigned const char* rdata;
	int rdatalen;
	unsigned const char* textdata;
	int textlen;
	unsigned char* imagestart;

	int imagesize = EXEOBFUS_READ_FROM_FILE(&imagestart, lpFilename);
	if(imagesize == 0)
	{
		exit(-1);
	}
	printf("Image offset: %x\n", imagestart);
	
	printf("EXEOBFUS_CIPHER_STATIC_STRINGS ...");
	EXEOBFUS_CIPHER_STATIC_STRINGS(imagestart, &rdata, &rdatalen);
	printf("done\n");
	
	printf("EXEOBFUS_WRITE_CRC_CHECKSUMS ...");
	EXEOBFUS_WRITE_CRC_CHECKSUMS(imagestart, &textdata, &textlen);
	printf("done\n");

	printf("EXEOBFUS_WRITE_SECTION_TO_FILE ...");
	EXEOBFUS_WRITE_SECTION_TO_FILE(rdata, rdatalen, lpFilename, ".rdata");
	printf("done\n");
	
	printf("EXEOBFUS_WRITE_SECTION_TO_FILE ...");
	EXEOBFUS_WRITE_SECTION_TO_FILE(textdata, textlen, lpFilename, ".text");
	printf("done\n");

	return;
}
/*
INLINE void EXEOBFUS_NUKE_CRPYTOR(char* filename)
{
	DWORD dwSectionSize;
	DWORD dwSectionBase;
	unsigned char* imagestart;
	int i;
	unsigned int start = 0;
	unsigned int end = 0;

	int imagesize = EXEOBFUS_READ_FROM_FILE(&imagestart, filename);
	if(imagesize <= 0)
	{
		exit(6);
	}
	EXEOBFUS_GETSECTIONBASELEN(imagestart, ".text", false, &dwSectionBase, &dwSectionSize);

	if(dwSectionSize <= 0)
	{
		exit(7);
	}

	char* loc = (char*)dwSectionBase;

	for(i = 0; i < dwSectionSize; ++i)
	{
		if( ((DWORD*)&loc[i])[0] == 0xd231c031 && ((DWORD*)&loc[i])[1] == 0xd231c031)
		{
			MessageBoxA(NULL, "Nuke start found", "Nuke start found", MB_OK);
			if(start == 0)
			{
				start = i;
			}else{
				end = i;
				break;
			}
			i += 8;

		}

	}
	if(!start || !end)
	{
		exit(8);
	}
	memset(&loc[start], 0xc3, end - start);

	EXEOBFUS_WRITE_SECTION_TO_FILE((unsigned const char*)dwSectionBase, dwSectionSize, filename , ".text");


}
*/


int main ( int argc, char *argv[])
{
	char* filename = argv[1];
	if(argc != 2)
	{
		printf("Usage: exeobfus.exe filename\n");
		return -1;
	}

	EXEOBFUS_STATIC_CODE_CRYPTER_INIT(filename);
	return 0;
}

int EXEOBFUS_WRITE_CRC_CHECKSUMS_FOR_SINGLE_FUNCTION(unsigned char* image, unsigned char* loc, int maxlen);

int GetFunctionLength(uint32_t entrypoint/* amount of bytes ahead from .text section start */, uint32_t textstart, int textsize);


void EXEOBFUS_WRITE_CRC_CHECKSUMS(unsigned char* image, unsigned const char** data, int* imagelen)
{
	DWORD dwSectionSize;
	DWORD dwSectionBase;

//	unsigned char cipherkey[128];
//	int start;
	int i;

	EXEOBFUS_GETSECTIONBASELEN(image, ".text", false, &dwSectionBase, &dwSectionSize);

	if(dwSectionSize == 0)
	{
		MessageBoxA(NULL, "EXEOBFUS_WRITE_CRC_CHECKSUMS failed Section not found", "EXEOBFUS_WRITE_CRC_CHECKSUMS failed", MB_OK);
		exit(0);
	}

	*data = (unsigned char*)dwSectionBase;
	*imagelen = dwSectionSize;

	unsigned char* loc = (unsigned char*)dwSectionBase;
	//Search for the special instructions which are the markers
	for(i = 0; i < dwSectionSize; ++i)
	{
		if( ((DWORD*)&loc[i])[0] == 0xd289c089 && ((DWORD*)&loc[i])[1] == 0xdb89c989 && ((DWORD*)&loc[i])[2] == 0xf689ff89)
		{
			printf("\nFound function which needs checksum at 0x%x...\n", &loc[i] - image);
			int funclen = GetFunctionLength(i, dwSectionBase, dwSectionSize);
			printf("Function length is: %d\n", funclen);

			i += EXEOBFUS_WRITE_CRC_CHECKSUMS_FOR_SINGLE_FUNCTION(image, &loc[i], funclen);

//			i += EXEOBFUS_WRITE_CRC_CHECKSUMS_FOR_SINGLE_FUNCTION(image, &loc[i], dwSectionSize - i);

		}
	}
	/*
	if(i == dwSectionSize)
		MessageBoxA(NULL, "EXEOBFUS_WRITE_CRC_CHECKSUMS NOTHING FOUND", "EXEOBFUS_WRITE_CRC_CHECKSUMS NOTHING FOUND", MB_OK);
	*/



}

int IsJumpLength(ud_t *ud_obj, enum ud_mnemonic_code mnemonic, int instructionLength)
{

	if (mnemonic == UD_Ijmp || mnemonic == UD_Ija || mnemonic == UD_Ijae || mnemonic == UD_Ijb || mnemonic == UD_Ijbe ||
		 mnemonic == UD_Ijcxz || mnemonic == UD_Ijecxz || mnemonic == UD_Ijg || mnemonic == UD_Ijge || mnemonic == UD_Ijl ||
		 mnemonic == UD_Ijle || mnemonic == UD_Ijno || mnemonic == UD_Ijnp || mnemonic == UD_Ijns || mnemonic == UD_Ijnz ||
		 mnemonic == UD_Ijo || mnemonic == UD_Ijp || mnemonic == UD_Ijrcxz || mnemonic == UD_Ijs || mnemonic == UD_Ijz)
	{ //Is a jump instruction.
		const struct ud_operand* op = ud_insn_opr(ud_obj, 0);
		if(op->type == UD_OP_JIMM)
		{ //But does it also point to the instruction we want?
			int jmpd;
			if(instructionLength == 2)
			{
				jmpd = op->lval.sbyte;
			}else if(instructionLength == 3){
				jmpd = op->lval.sword;
			}else{
				jmpd = op->lval.sdword;
			}
			return jmpd;
		}
	}
	return 0;
}

/* Length:
	 1. Scan for RET or JMP
	 2. if RET or JMP is hit scan again from function start for any jmp, jnz, jz, jc, jnc, jp, jnp... which reference the address after the RET
	 3. if scan is successful continue at 1.
 */
bool FindReferenceToProgCounter(ud_t *ud_obj, uint32_t entrypoint)
{
	uint32_t oldpc = ud_obj->inp_buf_index;
	enum ud_mnemonic_code mnemonic;
	int instructionLength;

	ud_obj->inp_buf_index = entrypoint;

	while (!ud_input_end(ud_obj) && ud_obj->inp_buf_index < oldpc && (instructionLength = ud_disassemble(ud_obj)))
	{

		mnemonic = ud_insn_mnemonic(ud_obj);

//		printf("%x   -   %s\n", ud_obj->inp_buf_index, ud_insn_asm(ud_obj));

		if(mnemonic == UD_Iint3 || mnemonic == UD_Iinvalid)
		{
			return false;
		}

		if (mnemonic == UD_Ijmp || mnemonic == UD_Ija || mnemonic == UD_Ijae || mnemonic == UD_Ijb || mnemonic == UD_Ijbe ||
			 mnemonic == UD_Ijcxz || mnemonic == UD_Ijecxz || mnemonic == UD_Ijg || mnemonic == UD_Ijge || mnemonic == UD_Ijl ||
			 mnemonic == UD_Ijle || mnemonic == UD_Ijno || mnemonic == UD_Ijnp || mnemonic == UD_Ijns || mnemonic == UD_Ijnz ||
			 mnemonic == UD_Ijo || mnemonic == UD_Ijp || mnemonic == UD_Ijrcxz || mnemonic == UD_Ijs || mnemonic == UD_Ijz)
		{ //Is a jump instruction.
			const struct ud_operand* op = ud_insn_opr(ud_obj, 0);
			if(op->type == UD_OP_JIMM)
			{ //But does it also point to the instruction we want?
				int jmpd;
				if(instructionLength == 2)
				{
					jmpd = op->lval.sbyte;
				}else if(instructionLength == 3){
					jmpd = op->lval.sword;
				}else{
					jmpd = op->lval.sdword;
				}
				if(ud_obj->inp_buf_index + jmpd >= oldpc && ud_obj->inp_buf_index + jmpd < oldpc + 32)
				{
//					printf("Found match\n");
					//reference found
					//restore the program counter
					ud_obj->inp_buf_index = oldpc;
					return true;
				}
			}
		}
	}
	return false;
}

//Can't analyse switch statements or any else [jmp register] opcodes at the moment
// Super Slow
int GetFunctionLength(uint32_t entrypoint/* amount of bytes ahead from .text section start */, uint32_t textstart, int textsize)
{
	printf("Base: %p Offset: %d Textsize: %d\n", textstart, entrypoint, textsize);
	ud_t ud_obj;
	enum ud_mnemonic_code mnemonic;
	int pc;
	int instructionLength;

	if(textstart == 0 || entrypoint < 0 || entrypoint >= textsize)
	{
		return 0;
	}

	ud_init(&ud_obj);

	ud_set_mode(&ud_obj, 32);
	ud_set_vendor(&ud_obj, UD_VENDOR_ANY);
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);
	ud_set_input_buffer(&ud_obj, (uint8_t*)textstart, textsize);
	ud_set_pc(&ud_obj, entrypoint);
	//ud_obj.inp_buf_index = entrypoint;
	ud_obj.inp_buf_index = entrypoint;

	while (pc = ud_obj.inp_buf_index, !ud_input_end(&ud_obj) && (instructionLength = ud_disassemble(&ud_obj)))
	{

		mnemonic = ud_insn_mnemonic(&ud_obj);

//		printf("%x - %s\n", pc, ud_insn_asm(&ud_obj));

		if(mnemonic == UD_Iint3)
		{
			printf("Int3 hit\n");
			return 0;
		}

		if(mnemonic == UD_Iinvalid)
		{
			printf("Invalid hit\n");
			return 0;
		}

		if (mnemonic == UD_Iret || mnemonic == UD_Iretf)
		{
//			printf("%x %s\n", pc, ud_insn_asm(&ud_obj));

			if(FindReferenceToProgCounter(&ud_obj, entrypoint) == false)
			{
				return ud_obj.inp_buf_index - entrypoint;
			}
		}else{
			int jmpd = IsJumpLength(&ud_obj, mnemonic, instructionLength);
			if(jmpd != 0)
			{
//				printf("%x %s\n", pc, ud_insn_asm(&ud_obj));
				if(jmpd > 0)
				{
					if(jmpd + ud_obj.inp_buf_index > textsize)
					{
						return 0;
					}
					ud_obj.inp_buf_index += jmpd;
//					printf("Follow jump forward by %d\n", jmpd);
				}else{
					if(mnemonic == UD_Ijmp)
					{
//						printf("Found backward jmp\n");
						if(FindReferenceToProgCounter(&ud_obj, entrypoint) == false)
						{
							return ud_obj.inp_buf_index - entrypoint;
						}
					}
				}
			}
		}
	}
	printf("End of while() loop\n");
	return 0;
}




int EXEOBFUS_WRITE_CRC_CHECKSUMS_FOR_SINGLE_FUNCTION(unsigned char* image, unsigned char* loc, int maxlen)
{
	int oldloc = 0;
	int end = maxlen;
	int i, len, sum;

	for(i = end, oldloc = end; i >= 0; --i)
	{
		if( ((DWORD*)&loc[i])[0] == 0xd289c089 && ((DWORD*)&loc[i])[1] == 0xdb89c989 && ((DWORD*)&loc[i])[2] == 0xf689ff89 && ((WORD*)&loc[i])[6] != 0xc089)
		{

			len = oldloc - (i + 17);
			oldloc = i;

			sum = crc16((unsigned char*)&loc[i + 17], len);
			printf("CRC16 for len %d = %x Offset = %x\n", len, sum, &loc[i] - image );

			*((WORD*)&loc[i]) = 0xc0c7;
			*((DWORD*)&loc[i +2]) = len;
			*((WORD*)&loc[i +6]) = 0xc2c7;
			*((DWORD*)&loc[i +8]) = sum;

		}

		if( ((DWORD*)&loc[i])[0] == 0x90424090 && ((DWORD*)&loc[i])[1] == 0x31904143 && ((DWORD*)&loc[i])[2] == 0x434090f6)
		{
			printf("Applying crashcode at %p\n", &loc[i] - image );

			*((WORD*)&loc[i]) = 0xc481;
			*((WORD*)&loc[i +2]) = rand() % 20000;
			*((DWORD*)&loc[i +4]) = 0xe9610000;
			*((DWORD*)&loc[i +8]) = rand() % 200000;

		}

	}

	return end;
}

/*

INLINE DWORD DoBlockChecking(DWORD scan)
{
	int sum, i;


	if(scan == 0){
		return 0;
	}

	BYTE* pos = (BYTE*)scan;

	for(i = 0; i < 5000; ++i)
	{
		if( ((DWORD*)&pos[i])[0] == 0x89c089c9)
		{
			break;
		}
	}

	scan += i;

	sum = crc16(pos, i);



	return scan;

}
*/
