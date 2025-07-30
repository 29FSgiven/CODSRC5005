#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#define INLINE static __inline__ __attribute__((always_inline))


#define EXEOBFUS_STATIC_CODE_CRYPTER_INIT Sys_PreInitConsole
#define EXEOBFUS_CIPHERKEYHEADER "XEOBFUSCIPHERKEYHEAD"
#define EXEOBFUS_CIPHERKEYHEADERLEN 20
#define EXEOBFUS_CIPHERKEYLEN (EXEOBFUS_CIPHERKEYHEADERLEN - sizeof(short) -1)
#define EXEOBFUS_STATICKEY "nb89hsudgf78sddfssd"


INLINE const char *EXEOBFUS_STATIC_STRING_DECRYPT(volatile char *cipherstring, char *cleartext)
{
    char statickey[] = {EXEOBFUS_STATICKEY};
    volatile unsigned short len = (uint8_t)(cipherstring[EXEOBFUS_CIPHERKEYHEADERLEN - 2] ^ (cipherstring[0] ^ statickey[0])) |
                                  ((uint8_t)(cipherstring[EXEOBFUS_CIPHERKEYHEADERLEN - 1] ^ (cipherstring[1] ^ statickey[1])) << 8);

    for (int i = 0, c = 2; i < len; ++i, ++c)
    {
        if (c == EXEOBFUS_CIPHERKEYLEN)
            c = 0;

        cleartext[i] = cipherstring[c] ^ cipherstring[i + EXEOBFUS_CIPHERKEYHEADERLEN] ^ statickey[c];
    }

    return cleartext;
}


#define PROTECTSTRING(string,buf) EXEOBFUS_STATIC_STRING_DECRYPT(EXEOBFUS_CIPHERKEYHEADER string, buf)


INLINE bool IsJumpInstruction(void* adr)
{
	byte* inst = adr;
	if(inst[0] == 0xe9)
	{
		return true;
	}
	return false;
}

#define ZHOOKDETECT IsJumpInstruction


INLINE unsigned short crc16(unsigned char* data_p, unsigned char length){
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }
    return crc;
}

INLINE DWORD GetEIP( )
{
	DWORD eip;

	__asm__(
	".byte 0xe8\n"
	".long 0x00000000\n"
	"pop %%eax\n"
	:"=a" (eip)
	::);
	return eip;
}

INLINE void verifyError()
{
	__asm__
	(
		".byte 0x61\n"
		"xor %esp, %esp\n"
		".byte 0xe9\n"
		".long 0x0006284f\n"
	);
}


INLINE void verifyBlock()
{
	volatile int sum;
	volatile int	len;
	volatile DWORD eip;

	__asm__ volatile ("mov %%eax, %%eax\n"
			"mov %%edx, %%edx\n"
			"mov %%ecx, %%ecx\n"
			"mov %%ebx, %%ebx\n"
			"mov %%edi, %%edi\n"
			"mov %%esi, %%esi\n"
			".byte 0xe8\n"
			".long 0x00000000\n"
			"pop %%ecx\n"
			:"=a" (len), "=d" (sum), "=c" (eip)::);


	if(sum != crc16((BYTE*)eip, len))
	{
#ifdef FASTCOMPILE
		printf("Bad checksum. Expected %x got %x len %d\n", sum, crc16((BYTE*)eip, len), len);
#else
		verifyError();
#endif
	}

}




/*
INLINE DWORD verifyBlock(DWORD scan)
{
	int sum, len;


	if(scan == 0){
		verifyError();
	}

	__asm__("mov %%eax, %%eax\n"
			"mov %%edx, %%edx\n"
			"mov %%ecx, %%ecx\n"
			"mov %%ebx, %%ebx\n"
			"mov %%edi, %%edi\n"
			"mov %%esi, %%esi\n"
			:"=a" (len), "=d" (sum)::);

	if(sum != crc16((BYTE*)scan, len))
	{
		verifyError();
	}

	return scan + len;

}
*/

INLINE void verifyEnd()
{
	__asm__("mov %%eax, %%eax\n"
			"mov %%edx, %%edx\n"
			"mov %%ecx, %%ecx\n"
			"mov %%ebx, %%ebx\n"
			"mov %%edi, %%edi\n"
			"mov %%esi, %%esi\n"
			"mov %%eax, %%eax\n"
			::: "eax", "edx");

}

#ifndef _DEBUG

#define CODECRC() verifyBlock( )
#define CODECRCFINI() verifyEnd( )
#define CODEGARBAGEINIT() int garbageint1 = 0xff777711; int garbageint2 = 0x87178633; int garbageint3 = 500; int garbageint4 = 880; int garbageint5 = 880; char garbagebuf1[1024]; char garbagebuf2[1024]; char garbagebuf3[1024]

#define CODEGARBAGE1( ) garbageint5 = ZZ_ShortSwap(ZZ_LongSwap(garbageint2)); ZZ_isalpha(garbageint3);ZZ_islower( garbageint5 )
#define CODEGARBAGE2( ) garbageint3 = ZZ_isupper(ZZ_isprint( garbageint4 )); ZZ_isintegral( garbageint4 )
#define CODEGARBAGE3( ) if(ZZ_isanumber(PROTECTSTRING("7000 200 199 4", garbagebuf1))){ ZZ_CountChar(PROTECTSTRING("7000 200 199 4", garbagebuf1), garbageint4); }else{ ZZ_strichr( PROTECTSTRING("7000 200 199 4", garbagebuf1), '\n'); } ZZ_isprint( 'P' )
#define CODEGARBAGE4( ) ZZ_Compress( (char*)PROTECTSTRING("7000 200 199 4", garbagebuf1) ); ZZ_ExpandNewlines((char*) PROTECTSTRING("7000 200 199 4", garbagebuf1) )
#define CODEGARBAGE5( ) if(ZZ_stricmpn(PROTECTSTRING("open \"PhysicalDrive\7873af\\\"", garbagebuf2), garbagebuf1, garbageint4) == 0){ ZZ_strlwr(garbagebuf1); }else{ garbagebuf1[5] = 'T'; garbagebuf1[9] = 'q'; } ZZ_strchrrepl(garbagebuf1, '7', 'I')
#define CODEGARBAGE6( ) garbageint4 = 6; if(ZZ_isFloat(PROTECTSTRING("70000.6786852", garbagebuf2), garbageint4)){ sprintf(garbagebuf3, PROTECTSTRING("%d %s Check drive", garbagebuf1), garbageint1, garbagebuf2); } garbageint5 = ZZ_CountChar(garbagebuf3, 9)


#else

#define CODECRC()
#define CODECRCFINI()
#define CODEGARBAGEINIT()
#define CODEGARBAGE1( )
#define CODEGARBAGE2( )
#define CODEGARBAGE3( )
#define CODEGARBAGE4( )
#define CODEGARBAGE5( )
#define CODEGARBAGE6( )

#endif

INLINE short   ZZ_ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

INLINE int    ZZ_LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

INLINE int ZZ_isprint( int c )
{
	if ( c >= 0x20 && c <= 0x7E )
		return ( 1 );
	return ( 0 );
}

INLINE int ZZ_islower( int c )
{
	if (c >= 'a' && c <= 'z')
		return ( 1 );
	return ( 0 );
}

INLINE int ZZ_isupper( int c )
{
	if (c >= 'A' && c <= 'Z')
		return ( 1 );
	return ( 0 );
}

INLINE int ZZ_isalpha( int c )
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return ( 1 );
	return ( 0 );
}

INLINE bool ZZ_isanumber( const char *s )
{
	char *p;

	if( *s == '\0' )
		return false;

	strtod( s, &p );

	return *p == '\0';
}

INLINE bool ZZ_isintegral( float f )
{
	return (int)f == f;
}

INLINE bool ZZ_isprintstring( char* s ){
    char* a = s;
    while( *a ){
        if ( *a < 0x20 || *a > 0x7E ) return 0;
        a++;
    }
    return 1;
}

INLINE void ZZ_strncpyz( char *dest, const char *src, int destsize ) {

	if (!dest ) {
		verifyError();
	}
	if ( !src ) {
		verifyError();
	}
	if ( destsize < 1 ) {
		verifyError();
	}

	strncpy( dest, src, destsize-1 );
  dest[destsize-1] = 0;
}

INLINE int ZZ_stricmpn (const char *s1, const char *s2, int n) {
	int		c1, c2;

        if ( s1 == NULL ) {
           if ( s2 == NULL )
             return 0;
           else
             return -1;
        }
        else if ( s2==NULL )
          return 1;



	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0;		// strings are equal
}

INLINE int ZZ_strncmp (const char *s1, const char *s2, int n) {
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

INLINE char *ZZ_strlwr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = tolower(*s);
		s++;
	}
    return s1;
}

INLINE char *ZZ_strupr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = toupper(*s);
		s++;
	}
    return s1;
}

INLINE void ZZ_bstrcpy(char* dest, char* src){

    while(*src)
        *dest++ = *src++;

    *dest = 0;
}

INLINE void ZZ_strcat( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		verifyError();
	}
	ZZ_strncpyz( dest + l1, src, size - l1 );
}

INLINE void ZZ_strlcat( char *dest, size_t size, const char *src, int cpylimit) {

	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		verifyError();
	}

	if(cpylimit >= (size - l1) || cpylimit < 1){
		cpylimit = size - l1 -1;
	}

	memcpy( dest + l1, src, cpylimit);
	dest[l1 + cpylimit] = 0;
}

INLINE void ZZ_TrimCRLF(char* string)
{
	char* pos;
	int len;

	pos = strchr(string, '\n');
	if(pos)
	{
		*pos = '\0';
	}
	pos = strchr(string, '\r');
	if(pos)
	{
		*pos = '\0';
	}
	len = strlen(string);
	while(len > 0 && string[len -1] == ' ')
	{
		string[len -1] = '\0';
		len = strlen(string);
	}
}


INLINE int  ZZ_strichr( const char *s, char find)
{
  char sc;
  int i = 0;

    if (find >= 'a' && find <= 'z')
    {
      find -= ('a' - 'A');
    }

    while(true)
    {
        if ((sc = *s++) == 0)
          return -1;

        if(sc >= 'a' && sc <= 'z')
        {
          sc -= ('a' - 'A');
        }
        if(sc == find)
            return i;

        i++;
    }

    return -1;
}


INLINE int ZZ_CountChar(const char *string, char tocount)
{
	int count;

	for(count = 0; *string; string++)
	{
		if(*string == tocount)
			count++;
	}

	return count;
}


INLINE char	*ZZ_ExpandNewlines( char *in ) {
	static	char	string[1024];
	int		l;

	l = 0;
	while ( *in && l < sizeof(string) - 3 ) {
		if ( *in == '\n' ) {
			string[l++] = '\\';
			string[l++] = 'n';
		} else {
			string[l++] = *in;
		}
		in++;
	}
	string[l] = 0;

	return string;
}

INLINE void ZZ_strchrrepl(char *string, char torepl, char repl){
    for(;*string != 0x00;string++){
		if(*string == torepl){
			*string = repl;
		}
    }
}


INLINE bool ZZ_isNumeric(const char* string, int size){
    const char* ptr;
    int i;

    if(size > 0){ //If we have given a length compare the whole string

        for(i = 0, ptr = string; i < size; i++, ptr++){
            if(i == 0 && *ptr == '-') continue;
            if(*ptr < '0' || *ptr > '9') return false;
        }

    } else { //Search until the 1st space otherwise or null otherwise

        for(i = 0, ptr = string; *ptr != ' '; i++, ptr++){
            if(i == 0 && *ptr == '-') continue;
            if(!*ptr && i > 0 && ptr[-1] != '-') return true;
            if(*ptr < '0' || *ptr > '9') return false;
        }
    }

    return true;
}


INLINE bool ZZ_IsEqualUnitWSpace(char *cmp1, char *cmp2)
{

	while ( 1 )
	{
		if ( !(*cmp1) || !(*cmp2) )
			break;

		if ( *cmp1 == ' ' || *cmp2 == ' ' )
			break;

		if ( *cmp1 != *cmp2 )
			return false;

		cmp1++;
		cmp2++;
	}

	if ( *cmp1 && *cmp1 != ' ')
	{
		return false;
	}
	if ( *cmp2 && *cmp2 != ' ')
	{
		return false;
	}

	return 1;
}



INLINE bool ZZ_isFloat(const char* string, int size)
{
    const char* ptr;
    int i;
    bool dot = false;
    bool sign = false;
    bool whitespaceended = false;

    if(size == 0) //If we have given a length compare the whole string
        size = 0x10000;

    for(i = 0, ptr = string; i < size && *ptr != '\0' && *ptr != '\n'; i++, ptr++){

        if(*ptr == ' ')
        {
            if(whitespaceended == false)
                continue;
            else
                return true;
        }
        whitespaceended = true;

        if(*ptr == '-' && sign ==0)
        {
            sign = true;
            continue;
        }
        if(*ptr == '.' && dot == 0)
        {
            dot = true;
            continue;
        }
        if(*ptr < '0' || *ptr > '9') return false;
    }
    return true;
}

INLINE bool ZZ_isInteger(const char* string, int size)
{
    const char* ptr;
    int i;
    bool sign = false;
    bool whitespaceended = false;

    if(size == 0) //If we have given a length compare the whole string
        size = 0x10000;

    for(i = 0, ptr = string; i < size && *ptr != '\0' && *ptr != '\n' && *ptr != '\r'; i++, ptr++){

        if(*ptr == ' ')
        {
            if(whitespaceended == false)
                continue;
            else
                return true;
        }
        whitespaceended = true;

        if(*ptr == '-' && sign ==0)
        {
            sign = true;
            continue;
        }
        if(*ptr < '0' || *ptr > '9') return false;
    }
    return true;
}

INLINE bool ZZ_isVector(const char* string, int size, int dim)
{
    const char* ptr;
    int i;

    if(size == 0) //If we have given a length compare the whole string
        size = 0x10000;

    for(i = 0, ptr = string; i < size && *ptr != '\0' && *ptr != '\n' && dim > 0; i++, ptr++){

        if(*ptr == ' ')
        {
            continue;
        }
        dim = dim -1;

        if(ZZ_isFloat(ptr, size -i) == false)
            return false;

        while(*ptr != ' ' && *ptr != '\0' && *ptr != '\n' && i < size)
        {
            ptr++; i++;
        }
    }
    if(dim != 0)
        return false;

    return true;
}



INLINE bool ZZ_strToVect(const char* string, float *vect, int dim)
{
    const char* ptr;
    int i;

    for(ptr = string, i = 0; *ptr != '\0' && *ptr != '\n' && i < dim; ptr++){

        if(*ptr == ' ')
        {
            continue;
        }

        vect[i] = atof(ptr);

        i++;

        while(*ptr != ' ' && *ptr != '\0' && *ptr != '\n')
        {
            ptr++;
        }
    }
    if(i != dim)
        return false;

    return true;
}

INLINE int ZZ_Compress( char *data_p ) {
	char *datai, *datao;
	int c, size;
	bool ws = false;

	size = 0;
	datai = datao = data_p;
	if ( datai ) {
		while ( ( c = *datai ) != 0 ) {
			if ( c == 13 || c == 10 ) {
				*datao = c;
				datao++;
				ws = false;
				datai++;
				size++;
				// skip double slash comments
			} else if ( c == '/' && datai[1] == '/' ) {
				while ( *datai && *datai != '\n' ) {
					datai++;
				}
				ws = false;
				// skip /* */ comments
			} else if ( c == '/' && datai[1] == '*' ) {
				while ( *datai && ( *datai != '*' || datai[1] != '/' ) )
				{
					datai++;
				}
				if ( *datai ) {
					datai += 2;
				}
				ws = false;
			} else {
				if ( ws ) {
					*datao = ' ';
					datao++;
				}
				*datao = c;
				datao++;
				datai++;
				ws = false;
				size++;
			}
		}
	}
	*datao = 0;
	return size;
}
