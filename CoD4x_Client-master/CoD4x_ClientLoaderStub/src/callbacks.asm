
%macro bin_import 2

	SECTION .text
		global %1
		%1: jmp dword [o%1]

	SECTION .rodata
		o%1 dd %2
%endmacro


bin_import __iw3mp_security_init_cookie, 0x67f189
bin_import __iw3mp_tmainCRTStartup, 0x67475c
bin_import __CG_RegisterWeapon, 0x454320
