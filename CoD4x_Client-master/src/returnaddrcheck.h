

static inline void Sec_CrashGameIfNecessary()
{
#ifndef COD4XDEV
	if(	*(uint16_t*)0x9562a9 == 0)
	{
		return;
	}
	
	if(((*(uint16_t*)0x9562a9) ^ 0x6781) == 1)
	{
		__asm__
		(
			"addl $0x800, %esp\n"
			"pushl $0x40C6E0\n"
			"ret\n"
		);
	}

	if(	*(uint16_t*)0x9562a9 != 0)
	{
		(*(uint16_t*)0x9562a9) = (((*(uint16_t*)0x9562a9)  ^ 0x6781) -1) ^ 0x6781;
	}
#endif // COD4XDEV
}

static inline void Sec_SetToCrashGame()
{
#ifndef COD4XDEV
	if(	*(uint16_t*)0x9562a9 != 0)
	{
		return;
	}
	MessageBoxA(NULL, "Wait crash", "Error", MB_OK);

	(*(uint16_t*)0x9562a9) = (rand() & 0x1f) ^ 0x6781;
#endif // COD4XDEV
}

static inline void Sec_VerifyReturnAddress(DWORD* whitelist)
{
#ifndef COD4XDEV
	int i;
	DWORD retAddr =  (DWORD)__builtin_return_address (0);
	
	for(i = 0; whitelist[i]; ++i)
	{
		if(whitelist[i] == retAddr)
		{
			return;
		}
	}
	Sec_SetToCrashGame();
#endif // COD4XDEV
}

