#include "q_shared.h"
#include "sys_patch.h"
#include <windows.h>


int entrypoint();

void Sys_PatchImageWithBlock(byte *block, int blocksize)
{

    int startadr;
    byte* startadrasbytes = (byte*)&startadr;

    startadrasbytes[0] = block[0];
    startadrasbytes[1] = block[1];
    startadrasbytes[2] = block[2];
    startadrasbytes[3] = block[3];

// printf("Block Start address is: %X\n", startadr);

    memcpy((void*)startadr, &block[4], blocksize - 4);

}


void SetCall(DWORD addr, void* destination){

	DWORD callwidth;
	byte* baddr = (byte*)addr;

	callwidth = (DWORD)( destination - (void*)baddr - 5);
	*baddr = 0xe8;
	baddr++;

	*(DWORD*)baddr = callwidth;
}

void SetJump(DWORD addr, void* destination){

	DWORD jmpwidth;
	byte* baddr = (byte*)addr;

	jmpwidth = (DWORD)( destination - (void*)baddr - 5);
	*baddr = 0xe9;
	baddr++;

	*(DWORD*)baddr = jmpwidth;
}

void Patch_WinMainEntryPoint(void* arg1){
	SetJump(0x67493c, entrypoint);
}

void Sys_OpenURLFake()
{

}

void __cdecl CG_RegisterItems();

int __regparm1 __MSG_ReadBitsCompress_Client(const byte* input, int readsize, byte* outputBuf);
int __regparm1 __MSG_ReadBitsCompress_Server(const byte* input, int readsize, byte* outputBuf);

void Patch_Exploits(void* arg)
{
	SetCall(0x43EDDD, CG_RegisterItems);
	SetJump(0x44AF69, CG_RegisterItems);
	SetJump(0x5769E0, Sys_OpenURLFake);
	SetCall(0x474727, __MSG_ReadBitsCompress_Client);
	SetCall(0x52D12D, __MSG_ReadBitsCompress_Server);
}

qboolean Patch_MainModule(void(patch_func)(), void* arg1){

	int i;
	DWORD dwSectionSize = 0;
	DWORD dwSectionBase;
	HMODULE hModule;
	hModule = GetModuleHandleA(NULL);
	if (! hModule)
		return qfalse;

	// get dos header
	IMAGE_DOS_HEADER dosHeader = *(IMAGE_DOS_HEADER*)(hModule);

	// get nt header
	IMAGE_NT_HEADERS ntHeader = *(IMAGE_NT_HEADERS*)((DWORD)hModule + dosHeader.e_lfanew);

	// iterate through section headers and search for ".text" section
	int nSections = ntHeader.FileHeader.NumberOfSections;
	for ( i = 0; i < nSections; i++)
	{
		IMAGE_SECTION_HEADER sectionHeader = *(IMAGE_SECTION_HEADER*) ((DWORD)hModule + dosHeader.e_lfanew + sizeof(IMAGE_NT_HEADERS) + i*sizeof(IMAGE_SECTION_HEADER));

		if (strcmp((char*)sectionHeader.Name,".text") == 0)
		{
			// the values you need
			dwSectionBase = (DWORD)hModule + sectionHeader.VirtualAddress;
			dwSectionSize = sectionHeader.Misc.VirtualSize;
			break;
		}
	}
	if(!dwSectionSize)
			return qfalse;

	// unprotect the text section
	DWORD oldProtectText;

	VirtualProtect((LPVOID)dwSectionBase, dwSectionSize, PAGE_READWRITE, &oldProtectText);
		patch_func(arg1);
	VirtualProtect((LPVOID)dwSectionBase, dwSectionSize, PAGE_EXECUTE_READ, &oldProtectText);
	return qtrue;
}
