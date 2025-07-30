/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm
    Copyright (C) 1999-2005 Id Software, Inc.

    This file is part of CoD4X18-Server source code.

    CoD4X18-Server source code is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    CoD4X18-Server source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
===========================================================================
*/

#ifndef __Q_SHARED_H__
#define __Q_SHARED_H__

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#define PORT_SERVER 24666
#define MAX_CONNECTEDCLIENTS 1024

#ifndef __stdcall
#define __stdcall __attribute__((stdcall))
#endif

#ifndef __cdecl
#define __cdecl __attribute__((cdecl))
#endif

#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#define DLL_LOCAL __attribute__ ((visibility ("hidden")))

#ifdef __linux

#define __optimize2 __attribute__ ((optimize("-O2")))
#define __optimize3 __attribute__ ((optimize("-O3"))) __attribute__ ((noinline))

#else

#define __optimize2
#define __optimize3

#endif

#define REGPARM(X)   __attribute__ ((regparm(X)))

#ifndef QDECL
#define QDECL
#endif

#ifndef _Noreturn
#define _Noreturn __attribute__((noreturn))
#endif

#ifndef __fastcall
#define __fastcall __attribute__((fastcall))
#endif

#ifndef __regparm1
#define __regparm1 __attribute__((regparm(1)))
#endif

#ifndef __regparm2
#define __regparm2 __attribute__((regparm(2)))
#endif

#ifndef __regparm3
#define __regparm3 __attribute__((regparm(3)))
#endif

typedef unsigned int long DWORD;
typedef unsigned short WORD;

#ifndef LOWORD
#define LOWORD(a) ((WORD)(a))
#endif

#ifndef HIWORD
#define HIWORD(a) ((WORD)(((DWORD)(a) >> 16) & 0xFFFF))
#endif

typedef unsigned char byte;
typedef enum {qfalse, qtrue}	qboolean;

#ifndef STDCALL
#define STDCALL __attribute__((stdcall))
#endif

#define	MAX_STRING_CHARS	1024

#ifndef Q_vsnprintf
int Q_vsnprintf(char *s0, size_t size, const char *fmt, va_list args);
#endif

#define MAX_OSPATH 256
#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024
#define	MAX_TOKEN_CHARS		1024

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_NAME_LENGTH		16	// max length of a client name
#define	MAX_QPATH		64	// max length of a quake game pathname

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		8192
#define	BIG_INFO_VALUE		8192

#define Com_Memset memset
#define Com_Memcpy memcpy


#define NET_WANT_READ -0x7000
#define NET_WANT_WRITE -0x7001
#define NET_CONNRESET -0x7002
#define NET_ERROR -0x7003

short   ShortSwap (short l);
short	ShortNoSwap (short l);
int    LongSwap (int l);
int	LongNoSwap (int l);


#ifdef _MSC_VER
int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

void Q_strncpyz( char *dest, const char *src, int destsize );
int Q_stricmpn (const char *s1, const char *s2, int n);
int Q_strncmp (const char *s1, const char *s2, int n);
int Q_stricmp (const char *s1, const char *s2);
char *Q_strlwr( char *s1 );
char *Q_strupr( char *s1 );
void Q_bstrcpy(char* dest, const char* src);
void Q_strcat( char *dest, int size, const char *src );
void Q_strlcat( char *dest, size_t size, const char *src, int cpylimit);
void Q_strnrepl( char *dest, size_t size, const char *src, const char* find, const char* replacement);
const char *Q_stristr( const char *s, const char *find);
int  Q_strichr( const char *s, char find);
int Q_PrintStrlen( const char *string );
char *Q_CleanStr( char *string );
int Q_CountChar(const char *string, char tocount);
int QDECL Com_sprintf(char *dest, int size, const char *fmt, ...);
void Q_strchrrepl(char *string, char torepl, char repl);
char* Q_BitConv(int val);
int Q_strLF2CRLF(const char* input, char* output, int outputlimit );
/* char	* QDECL va( char *format, ... ); */
char* QDECL va_replacement(char *dest, int size, const char *fmt, ...);
#define mvabuf char va_buffer[MAX_STRING_CHARS]
#define va(fmt,... ) va_replacement(va_buffer, sizeof(va_buffer), fmt, __VA_ARGS__)

void Com_Printf(const char* fmt, ...);
void Com_DPrintf(const char* fmt, ...);
void Com_PrintError(const char* fmt, ...);
void Com_PrintWarning(const char* fmt, ...);
_Noreturn void Com_Error(const char* fmt, ...);
uint64_t FS_FileHash(const char* ospath, uint8_t* hash, int outlength);
void Blakesum2String(uint8_t* checksum, char* hstring, int maxsize);
void FS_ReplaceSeparators( char *path );
void FS_TurnSeparatorsForward( char *path );
int64_t FS_GetFileModifiedTime(const char* ospath);
void FS_StripTrailingSeparator( char *path );
void FS_StripLeadingSeparator( char *path );
int64_t FS_GetFileSize(const char* ospath);
int64_t FS_FOpenFileReadOSPath( const char *filename, FILE **fp );
bool FS_FCloseFileOSPath( FILE* f );
int FS_ReadOSPath( void *buffer, int len, FILE* f );
bool FS_WriteOSPath( void *buffer, unsigned int len, FILE* f );
FILE* FS_FOpenFileWriteOSPath( const char *filename );
int ACHlp_LoadFile(unsigned char **data, const char* filename);

# define BLAKE2S_OUTBYTES 32

#ifndef _stricmp
int _stricmp(const char*, const char*);
#endif

void Sys_SleepSec(int seconds);

#endif
