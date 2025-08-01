#include <windows.h> 

#define CALL 0xe8
#define MOV_EAX_ESI 0x89, 0xf0
#define MOV_ESI_EDX 0x89, 0xd6
#define MOV_EDX_ESI 0x89, 0xf2
#define MOV_EDX_ESP 0x89, 0xe2
#define MOV_EAX_EBX 0x89, 0xd8
#define MOV_EAX_EBP 0x89, 0xe8
#define NOP 0x90
#define PUSH_ESI 0x56
#define PUSH_EDI 0x57
#define PUSH_EAX 0x50
#define PUSH_EBX 0x53
#define PUSH_EDX 0x52
#define POP_EBX 0x5b
#define POP_ECX 0x59
#define POP_EDX 0x5a
#define RET 0xc3
#define CALL_ZERO 0xe8, 0x00, 0x00, 0x00, 0x00
#define ADD_ESP 0x83, 0xc4,
#define ADD_EDX_DWORD 0x81, 0xc2,
#define ADD_EDX 0x83, 0xc2,
#define ADD_EAX 0x83, 0xc0,

void SetCall(DWORD addr, void* destination);
void SetJump(DWORD addr, void* destination);


void Patch_WinMainEntryPoint();
qboolean Patch_MainModule(void(patch_func)(), void* arg);
void Patch_Exploits(void* arg);
