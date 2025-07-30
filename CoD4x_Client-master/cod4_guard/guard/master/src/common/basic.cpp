#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "../blake/blake2.h"
#include "rsa_verify.h"



void XAC_CreateFileHash(const char* filepath, char* finalsha65b){

    unsigned int r, i;
    uint8_t hash[BLAKE2S_OUTBYTES];
    bool isgood = false;
    blake2s_state ctx;
    const char	*hex = "0123456789abcdef";
    char data[0x10000];
    finalsha65b[0] = 0;


    FILE* filehandle = fopen(filepath, "rb");
    if(filehandle == NULL)
    {
        return;
    }
    
    blake2s_init( &ctx, sizeof(hash) );
    while(true)
    {
        int s = fread(data, 1, sizeof(data), filehandle);
        if(s < 1)
        {
            break;
        }
        blake2s_update( &ctx, ( uint8_t * )data, s );   
	isgood = true;
    }
    fclose(filehandle);
    if(isgood == false)
    {
        return;
    }
    blake2s_final( &ctx, hash, sizeof(hash) );

    for(i = 0, r=0; i < sizeof(hash); i++) {
        finalsha65b[r++] = hex[hash[i] >> 4];
        finalsha65b[r++] = hex[hash[i] & 0xf];
    }
    finalsha65b[65] = 0x00;
}



void XAC_CreateHash(const char* in, int inlen, char* finalsha65b){

    unsigned int r, i;
    uint8_t hash[BLAKE2S_OUTBYTES];

    blake2s_state ctx;
    const char	*hex = "0123456789abcdef";
    finalsha65b[0] = 0;

    blake2s_init( &ctx, sizeof(hash) );
    blake2s_update( &ctx, ( uint8_t * )in, inlen );
    blake2s_final( &ctx, hash, sizeof(hash) );

    for(i = 0, r=0; i < sizeof(hash); i++) {
        finalsha65b[r++] = hex[hash[i] >> 4];
        finalsha65b[r++] = hex[hash[i] & 0xf];
    }
    finalsha65b[65] = 0x00;
}

int Com_sprintf(char *dest, int size, const char *fmt, ...)
{
	int		len;
	va_list		argptr;

	va_start (argptr,fmt);
	len = vsnprintf(dest, size, fmt, argptr);
	va_end (argptr);

	return len;
}

int ACHlp_LoadFile(unsigned char **data, const wchar_t* filename)
{
	FILE* fh = _wfopen(filename, L"rb");

	if(!fh)
	{
		*data = NULL;
		return 0;
	}

	// obtain file size:
	fseek (fh , 0 , SEEK_END);
	long unsigned lSize = ftell (fh);
	rewind (fh);

	// allocate memory to contain the whole file:
	unsigned char* buffer = (unsigned char*)malloc (sizeof(char) * (lSize +1));
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

bool Sys_VerifyAnticheatSignature(unsigned char* data, unsigned int len, const void* dercert, unsigned int certlen)
{
    	char base64hash[4096];

		base64hash[0] = 0;
		unsigned int i;

        for(i = len -1; i > len - sizeof(base64hash); --i)
		{
			if(data[i] == '#' && strncmp((const char*)data +i, "#SIGNATURE-START#", 17) == 0)
			{
				int l = len - i - 17;
				if(l > (signed)sizeof(base64hash) -1)
				{
					return false;
				}
				memcpy(base64hash, data +i +17, l);
				base64hash[l] = 0;
				break;
			}
		}

		if(base64hash[0] == 0)
		{
			return false;
		}

		return RSA_VerifyMemory(base64hash, dercert, certlen, data, i);

}