/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm of the IceOps-Team
    Copyright (C) 1999-2005 Id Software, Inc.

    This file is part of CoD4X17a-Server source code.

    CoD4X17a-Server source code is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    CoD4X17a-Server source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
===========================================================================
*/

#include "q_shared.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <assert.h>
short   ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short	ShortNoSwap (short l)
{
	return l;
}

int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l)
{
	return l;
}

int Q_isprint( int c )
{
	if ( c >= 0x20 && c <= 0x7E )
		return ( 1 );
	return ( 0 );
}

int Q_islower( int c )
{
	if (c >= 'a' && c <= 'z')
		return ( 1 );
	return ( 0 );
}

int Q_isupper( int c )
{
	if (c >= 'A' && c <= 'Z')
		return ( 1 );
	return ( 0 );
}

int Q_isalpha( int c )
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return ( 1 );
	return ( 0 );
}

qboolean Q_isanumber( const char *s )
{
	char *p;

	if( *s == '\0' )
		return qfalse;

	strtod( s, &p );

	return *p == '\0';
}

qboolean Q_isintegral( float f )
{
	return (int)f == f;
}

qboolean Q_isprintstring( char* s ){
    char* a = s;
    while( *a ){
        if ( *a < 0x20 || *a > 0x7E ) return 0;
        a++;
    }
    return 1;
}

/*
This part makes qshared.c undepended in case no proper qcommon.h is included
*/

#ifndef __QCOMMON_STDIO_H__

#define Com_Printf printf
#define Com_PrintError printf
#define Com_PrintWarning printf
#define Com_DPrintf printf

#define ERR_FATAL 0
#define ERR_DROP 1
void Com_Error(int err, char* fmt,...)
{
	char buf[MAX_STRING_CHARS];

	va_list		argptr;

	va_start (argptr,fmt);
	Q_vsnprintf(buf, sizeof(buf), fmt, argptr);
	va_end (argptr);

	fputs(buf, stdout);
	exit(1);
}

#endif


#ifdef _MSC_VER
/*
=============
Q_vsnprintf

Special wrapper function for Microsoft's broken _vsnprintf() function.
MinGW comes with its own snprintf() which is not broken.
=============
*/

int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int retval;

	retval = _vsnprintf(str, size, format, ap);

	if(retval < 0 || retval == size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.

		str[size - 1] = '\0';
		return size;
	}

	return retval;
}
#endif

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize ) {

	if (!dest ) {
	    Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
	}
	if ( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}
	if ( destsize < 1 ) {
		Com_Error(ERR_FATAL,"Q_strncpyz: destsize < 1" );
	}

	strncpy( dest, src, destsize-1 );
  dest[destsize-1] = 0;
}

/*
=============
Q_strncpyzUni

Safe strncpy that ensures a trailing zero
=============
*/

void Q_strncpyzUni( wchar_t *dest, const wchar_t *src, int destsize ) {

	int numchars = destsize / sizeof(wchar_t);

	if (!dest ) {
	    Com_Error( ERR_FATAL, "Q_strncpyzUni: NULL dest" );
	}
	if ( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyUniz: NULL src" );
	}
	if ( numchars < 1 ) {
		Com_Error(ERR_FATAL,"Q_strncpyzUni: destsize < 1 wchar" );
	}

	wcsncpy( dest, src, numchars);
	dest[numchars -1] = 0;
}

int Q_stricmpn (const char *s1, const char *s2, int n) {
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

int Q_strncmp (const char *s1, const char *s2, int n) {
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

int Q_stricmp (const char *s1, const char *s2) {
	return (s1 && s2) ? Q_stricmpn (s1, s2, 99999) : -1;
}


char *Q_strlwr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = tolower(*s);
		s++;
	}
    return s1;
}

char *Q_strupr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = toupper(*s);
		s++;
	}
    return s1;
}

/*
=============
Q_bstrcpy

Same like strcpy but it does always a byte wise copying so source and destiantion can overlap
Use is to fix up a problem with Info_SetValueForKey() which happened on some newer machines
=============
*/


void Q_bstrcpy(char* dest, char* src){

    while(*src)
        *dest++ = *src++;

    *dest = 0;
}




// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}

// never goes past bounds or leaves without a terminating 0
void Q_strcatUni( wchar_t *dest, int size, const wchar_t *src ) {
	int		l1;
	int		numChar;

	numChar = size / sizeof(wchar_t);

	l1 = wcslen( dest );
	if ( l1 >= numChar ) {
		Com_Error( ERR_FATAL, "Q_strcatUni: already overflowed" );
	}
	Q_strncpyzUni( dest + l1, src, size - sizeof(wchar_t) * l1 );
}

/*
=============
Q_WIsAnsiString

Returns qtrue when this wchar string does not contain unicode characters
=============
*/

qboolean Q_WIsAnsiString(wchar_t* str)
{
	while(*str)
	{
		if((uint16_t)*str > 255)
		{
			return qfalse;
		}
		str++;
	}
	return qtrue;
}

/*
=============
Q_strlcat

Same like strcat but with an additional copylimit parameter
=============
*/
void Q_strlcat( char *dest, size_t size, const char *src, int cpylimit) {

	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		return;
//		Com_Error( ERR_FATAL, "Q_strlcat: already overflowed" );
	}

	if(cpylimit >= (size - l1) || cpylimit < 1){
		cpylimit = size - l1 -1;
	}

	memcpy( dest + l1, src, cpylimit);
	dest[l1 + cpylimit] = 0;
}

void Q_TrimCRLF(char* string)
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

void Q_TrimCRLFUni(wchar_t* string)
{
	wchar_t* pos;
	pos = wcschr(string, L'\n');
	if(pos)
	{
		*pos = L'\0';
	}
	pos = wcschr(string, L'\r');
	if(pos)
	{
		*pos = L'\0';
	}
}

/*
=============
Q_strrepl
=============
*/
void Q_strnrepl( char *dest, size_t size, const char *src, const char* find, const char* replacement) {

    char* new;
    *dest = 0;

    int findlen = strlen(find);

    while((new = strstr(src, find)) != NULL){
        Q_strlcat(dest, size, src, new - src);
        Q_strlcat(dest, size, replacement, 0);
        src = &new[findlen];
    }
    Q_strlcat(dest, size, src, 0);
}


/*
* Find the first occurrence of find in s.
*/
const char *Q_stristr( const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0)
  {
    if (c >= 'a' && c <= 'z')
    {
      c -= ('a' - 'A');
    }
    len = strlen(find);
    do
    {
      do
      {
        if ((sc = *s++) == 0)
          return NULL;
        if (sc >= 'a' && sc <= 'z')
        {
          sc -= ('a' - 'A');
        }
      } while (sc != c);
    } while (Q_stricmpn(s, find, len) != 0);
    s--;
  }
  return s;
}


int  Q_strichr( const char *s, char find)
{
  char sc;
  int i = 0;

    if (find >= 'a' && find <= 'z')
    {
      find -= ('a' - 'A');
    }

    while(qtrue)
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




int Q_PrintStrlen( const char *string ) {
	int			len;
	const char	*p;

	if( !string ) {
		return 0;
	}

	len = 0;
	p = string;
	while( *p ) {
		if( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr( char *string ) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			s++;
		}
		else if ( c >= 0x20 && c <= 0xFE ) {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

int Q_CountChar(const char *string, char tocount)
{
	int count;

	for(count = 0; *string; string++)
	{
		if(*string == tocount)
			count++;
	}

	return count;
}

int QDECL Com_sprintf(char *dest, int size, const char *fmt, ...)
{
	int		len;
	va_list		argptr;

	va_start (argptr,fmt);
	len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end (argptr);

	if(len >= size)
	{
		Com_Printf("Com_sprintf: Output length %d too short, require %d bytes.\n", size, len + 1);
	}
	return len;
}


int QDECL Com_sprintfUni(wchar_t *dest, size_t size, const wchar_t *fmt, ...)
{

	int			len, numchar;
	va_list		argptr;

	numchar = size / sizeof(wchar_t);

	va_start (argptr,fmt);
	len = vsnwprintf (dest, numchar, fmt, argptr );
	va_end (argptr);

	if(len < 0 || len >= numchar)
		Com_Printf("Com_sprintfUni: Output length %d too short\n", size);

	return len;
}

int Q_StrToWStr(wchar_t* dest, const char* src, size_t len)
{
	int i;

	if(len < 2)
	{
		return 0;
	}

	int numchar = len / sizeof(wchar_t);

	for(i = 0; i < numchar; ++i)
	{
		dest[i] = btowc(src[i]);

		if(src[i] == '\0')
		{
			break;
		}
	}
	dest[numchar -1] = L'\0';
	return i;
}
/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
============
*/
char	* QDECL va( char *format, ... ) {
	va_list		argptr;
	static char string[2][32000]; // in case va is called by nested functions
	static int	index = 0;
	char		*buf;

	buf = string[index & 1];
	index++;

	va_start (argptr, format);
	Q_vsnprintf (buf, sizeof(*string), format, argptr);
	va_end (argptr);

	return buf;
}

/*
============
Com_TruncateLongString

Assumes buffer is atleast TRUNCATE_LENGTH big
============
*/
void Com_TruncateLongString( char *buffer, const char *s )
{
	int length = strlen( s );

	if( length <= TRUNCATE_LENGTH )
		Q_strncpyz( buffer, s, TRUNCATE_LENGTH );
	else
	{
		Q_strncpyz( buffer, s, ( TRUNCATE_LENGTH / 2 ) - 3 );
		Q_strcat( buffer, TRUNCATE_LENGTH, " ... " );
		Q_strcat( buffer, TRUNCATE_LENGTH, s + length - ( TRUNCATE_LENGTH / 2 ) + 3 );
	}
}





/*
=====================================================================

  INFO STRINGS

=====================================================================
*/


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s ) {
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}


/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key ) {
	char	*start;
	char	pkey[MAX_INFO_KEY];
	char	value[MAX_INFO_VALUE];
	char	*o;

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Printf(  "Error: Info_RemoveKey: oversize infostring" );
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			Q_bstrcpy(start, s);	// remove this part - Bugfix using Q_bstrcpy() instaed strcpy()
			return;
		}

		if (!*s)
			return;
	}

}


/*
===================
BigInfo_RemoveKey
===================
*/
void BigInfo_RemoveKey( char *s, const char *key ) {
	char	*start;
	char	pkey[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	char	*o;

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Printf(  "Error: BigInfo_RemoveKey: oversize infostring" );
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			Q_bstrcpy(start, s);	// remove this part - Bugfix using Q_bstrcpy() instaed strcpy()
			return;
		}

		if (!*s)
			return;
	}

}


/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char *Info_ValueForKey( const char *s, const char *key ) {
	char	pkey[BIG_INFO_KEY];
	static	char value[2][BIG_INFO_VALUE];	// use two buffers so compares
						// work without stomping on each other
	static	int	valueindex = 0;
	char	*o;

	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Printf(  "Error: Info_ValueForKey: oversize infostring" );
		return;
	}

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}



/*
===============
Info_ValueForKey_tsInternal

Same as Info_ValueForKey() but thread safe
Buffer needs to be 2 * MAX_INFO_STRING
===============
*/
char *Info_ValueForKey_tsInternal( const char *s, const char *key, char* buffer)
{
	char	pkey[MAX_INFO_STRING];
	//static	char value[2][MAX_INFO_STRING];	// use two buffers so compares
						// work without stomping on each other
	int valueindex = 0;
	char* o;
	char* value = buffer;

	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Printf(  "Error: Info_ValueForKey_ts: oversize infostring" );
		return;
	}

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value + MAX_INFO_STRING * valueindex;

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp (key, pkey) )
			return value + MAX_INFO_STRING * valueindex;

		if (!*s)
			break;
		s++;
	}

	return "";
}

/*
===============
Info_ValueForKey_tsInternal

Same as Info_ValueForKey() but thread safe
Buffer needs to be MAX_INFO_STRING
===============
*/
char *BigInfo_ValueForKey_tsInternal( const char *s, const char *key, char* buffer)
{
	char	pkey[MAX_INFO_STRING];
	char* o;


	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Printf(CON_CHANNEL_ERROR, "Error: BigInfo_ValueForKey_tsInternal: oversize infostring" );
		return "";
	}

	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}

		if(!*s)
		{
			return "";
		}
		*o = 0;
		s++;

		o = buffer;

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp (key, pkey) )
			return buffer;

		if (!*s)
			break;
		s++;
	}

	return "";
}


/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char	newi[MAX_INFO_STRING];

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_PrintWarning("Unexpected error - Info_SetValueForKey: oversize infostring" );
	}

	if (strchr (key, '\\'))
	{
		Com_PrintWarning ("Can't use keys with a \\\n");
		Com_DPrintf("Bad key: %s value: %s\n", key, value);
		return;
	}

	if (strchr (value, '\\'))
	{
		Com_PrintWarning ("Can't use values with a \\\n");
		Com_DPrintf("Bad value: %s key: %s\n", value, key);
		return;
	}

	if (strchr (key, ';'))
	{
		Com_PrintWarning ("Can't use keys with a semicolon\n");
		Com_DPrintf("Bad key: %s value: %s\n", key, value);
		return;
	}

	if (strchr (value, ';'))
	{
		Com_PrintWarning ("Can't use values with a semicolon\n");
		Com_DPrintf("Bad value: %s key: %s\n", value, key);
		return;
	}

	if (strchr (key, '\"'))
	{
		Com_PrintWarning ("Can't use keys with a \"\n");
		Com_DPrintf("Bad key: %s value: %s\n", key, value);
		return;
	}
	if (strchr (value, '\"'))
	{
		Com_PrintWarning ("Can't use values with a \"\n");
		Com_DPrintf("Bad value: %s key: %s\n", value, key);
		return;
	}


	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > MAX_INFO_STRING)
	{
		Com_PrintWarning ("Info string length exceeded\n");
		return;
	}

	strcat (newi, s);
	strcpy (s, newi);
}


/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void BigInfo_SetValueForKey( char *s, const char *key, const char *value ) {
	char	newi[BIG_INFO_STRING];

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Printf(  "Error: Info_SetValueForKey: oversize infostring" );
	}

	if (strchr (key, '\\') || strchr (value, '\\'))
	{
		Com_Printf("Error: Can't use keys or values with a \\\n");
		return;
	}

	if (strchr (key, ';') || strchr (value, ';'))
	{
		Com_Printf("Error: Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr (key, '\"') || strchr (value, '\"'))
	{
		Com_Printf("Error: Can't use keys or values with a \"\n");
		return;
	}

	BigInfo_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > BIG_INFO_STRING)
	{
		Com_Printf( "Error: Info string length exceeded\n");
		return;
	}

	strcat (newi, s);
	strcpy (s, newi);
}

/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( const char **head, char *key, char *value ) {
	char *o;
	const char *s;

	s = *head;

	if ( *s == '\\' ) {
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while ( *s != '\\' ) {
		if ( !*s ) {
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while ( *s != '\\' && *s ) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}



/*
==================
FastInfo_SetValueForKey

Adds a key/value pair in very short time. Does not care for duplicates.
Can not be used on two different infostrings on same time. Does no error checking on keys.
Error checking on values requires too much time as well...
==================
*/
void FastInfo_SetValueForKey( char *s, const char *key, const char *value ) {

	static int curpos = 0;
	int keylen;
	int vallen;

	if(s[0] == '\0')
		curpos = 0;

	if ( curpos >= MAX_INFO_STRING ) {
		Com_Printf(  "Error: Info_SetValueForKey: oversize infostring" );
	}
/*	if (strchr (value, '\\'))
	{
		return;
	}
	if (strchr (value, ';'))
	{
		return;
	}
	if (strchr (value, '\"'))
	{
		return;
	}
*/
	keylen = strlen(key);
	vallen = strlen(value);

	if ( keylen + vallen + 2 + curpos >= MAX_INFO_STRING)
	{
		Com_Printf( "Error: Info string length exceeded\n");
		return;
	}
	s[curpos] = '\\';
	curpos ++;
	Com_Memcpy(&s[curpos], key, keylen);
	curpos += keylen;
	s[curpos] = '\\';
	curpos ++;
	Com_Memcpy(&s[curpos], value, vallen);
	curpos += vallen;
	s[curpos] = '\0';

}

/*
===============
SV_ExpandNewlines

Converts newlines to "\n" so a line prints nicer
===============
*/
char	*SV_ExpandNewlines( char *in ) {
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





/*
============
Q_strchrrepl
Parses the string for a given char and replace it with a replacement char
============
*/
void Q_strchrrepl(char *string, char torepl, char repl){
    for(;*string != 0x00;string++){
		if(*string == torepl){
			*string = repl;
		}
    }
}

/*
============
Q_strchrreplUni
Parses the string for a given char and replace it with a replacement char
============
*/
void Q_strchrreplUni(wchar_t *string, wchar_t torepl, wchar_t repl){
    for( ; *string != L'\0'; string++){
		if(*string == torepl){
			*string = repl;
		}
    }
}

//============================================================================

void Info_Print( const char *s ) {
	char	key[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			Com_Memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf ("%s ", key);

		if (!*s)
		{
			Com_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Com_Printf ("%s\n", value);
	}
}


/*****************************************************
*** Parsing files ***
*****************************************************/
static qboolean parse_inquotes = qfalse;
static char* parse_lastpos;

void Com_ParseReset(){

    parse_inquotes = qfalse;
    parse_lastpos = NULL;
}

char* Com_ParseGetToken(char* line){

    if(parse_lastpos == line){

        if(parse_inquotes){//In case we are inside quotes step until the end quote forward

            do{
                line++;
            }while(*line != '"' && *line != ';' && *line != '\n' && *line != '\0');

            parse_inquotes = qfalse;
        }

        while(*line != ' '){
            if(*line == '\0' || *line == '\n'){
                parse_inquotes = qfalse;
                parse_lastpos = NULL;
                return NULL;
            }
            line++;
        }


    }

    while(*line == ' ' || *line == ';'){
        if(*line == '\0' || *line == '\n'){
            parse_inquotes = qfalse;
            parse_lastpos = NULL;
            return NULL;
        }
        line++;
    }

    if(*line == '"'){ //Check if the next token is the beginning of a quoted string
        parse_inquotes = qtrue;
        line++;	//Move over the quotes character to the 1st real character
    }


    if(*line == '\0' || *line == '\n'){
        parse_inquotes = qfalse;
        parse_lastpos = NULL;
        return NULL;
    }

    parse_lastpos = line;
    return line;
}


int Com_ParseTokenLength(char* token){
    if(token == NULL) return 0;

    char* pos = token;
    int i = 0;
    if(parse_inquotes){//In case we are inside quotes
        while(*pos != '"' && *pos != ';' && *pos != '\n' && *pos != '\0'){
            pos++;
            i++;
        }

    }else{//Default case

        while(*pos != ' ' && *pos != ';' && *pos != '\n' && *pos != '\0'){
            pos++;
            i++;
        }
    }
    return i;
}



qboolean isNumeric(const char* string, int size){
    const char* ptr;
    int i;

    if(size > 0){ //If we have given a length compare the whole string

        for(i = 0, ptr = string; i < size; i++, ptr++){
            if(i == 0 && *ptr == '-') continue;
            if(*ptr < '0' || *ptr > '9') return qfalse;
        }

    } else { //Search until the 1st space otherwise or null otherwise

        for(i = 0, ptr = string; *ptr != ' '; i++, ptr++){
            if(i == 0 && *ptr == '-') continue;
            if(!*ptr && i > 0 && ptr[-1] != '-') return qtrue;
            if(*ptr < '0' || *ptr > '9') return qfalse;
        }
    }

    return qtrue;
}



/*
=====================================================================

 Functions to operate onto a stack in lifo mode

=====================================================================
*/

void stack_init(void *array[], size_t size){
    array[0] = (void*)((size_t)array+size );	//Moving the stackpointer in array[0] to top of stack
}

qboolean stack_push(void *array[], int size, void* pointer){
        void** base;

        if(array[0] == &array[1]) return qfalse;	//Stackoverflow
        array[0] -= sizeof(void*);

        base = *array;
        *base = pointer;
        return qtrue;
}

void* stack_pop(void *array[], int size){

    void** base;

    if(array[0] < (void*)((size_t)array+size )){
        base = *array;
        array[0] += sizeof(void*);
        return *base;
    }
    return NULL;	//Stack reached bottom
}

/*
=====================================================================

  Writing XML STRINGS

=====================================================================
*/

void XML_AppendToBuffer( xml_t *base, const char* s )
{
    int len = strlen(s);

    if(len + base->bufposition + 1 >= base->buffersize )
    {
        Com_Printf(  "Error: XML_AppendToBuffer: Overflow!\n" );
        return;
    }
    Com_Memcpy(base->buffer + base->bufposition, s, len);
    base->bufposition += len;
    base->buffer[base->bufposition] = '\0';
}


/*
 ==================
 XML_Init

 Changes or adds a key/value pair
 ==================
 */

void XML_Init( xml_t *base, char *s, int size, char* encoding) {

	Com_Memset(base,0,sizeof(xml_t));
	char version[1024];

	base->buffer = s;
	base->bufposition = 0;
	base->buffersize = size;
	base->encoding = encoding;
	stack_init(base->stack,sizeof(base->stack));
	if ( 256 > size ) {
		Com_Printf(  "Error: XML_Init: too small infostring" );
	}
	Com_sprintf(version, sizeof(version), "<?xml version=\"1.0\" encoding=\"%s\"?>\n\0", base->encoding);
	XML_AppendToBuffer( base, version );
}


/*
 ==================
 XML_Escape
 Changes or adds a key/value pair
 ==================
 */
void XML_Escape( char* buffer, size_t size, const char* string){
	char* basebuf = buffer;

	for(; (buffer - basebuf + 7) < size && *string != 0; string++){

	    switch(*string){

			case '<':
				strcpy(buffer, "&lt;");
				buffer += 4;
				break;
			case '>':
				strcpy(buffer, "&gt;");
				buffer += 4;
				break;
			case '&':
				strcpy(buffer, "&amp;");
				buffer += 5;
				break;
			case '"':
				strcpy(buffer, "&quot;");
				buffer += 6;
				break;
			case '\'':
				strcpy(buffer, "&apos;");
				buffer += 6;
				break;
			default:
				if((byte)*string >= ' '){
					*buffer = *string;
					buffer++;
				}
		}
	}
	*buffer = 0;
}



/*
 ==================
 XML_OpenTag

 Changes or adds a key/value pair
 ==================
 */
qboolean QDECL XML_OpenTag( xml_t *base, char* root, int count,... ) {

	char* key;
	char* value;
	char buffer[8192];
	char smallbuff[512];
	int i;

	buffer[0] = 0;
	if(base->parents*6 +1 >= sizeof(smallbuff))
	{
		Com_Printf("^3Warning: XML_OpenTag would overflow. Too many open tags\n");
		return qfalse;
	}
	Com_Memset(&smallbuff[1],' ',base->parents*6);
	smallbuff[0] = '\n';
	smallbuff[base->parents*6] = 0;
	XML_AppendToBuffer( base, smallbuff );
	Com_sprintf(buffer,sizeof(buffer),"<%s",root);

	if(!stack_push(base->stack,sizeof(base->stack), base->buffer + base->bufposition + 1)){
		Com_Printf("^3Warning: XML_OpenTag called without prior initialization\n");
		return qfalse;
	}

	XML_AppendToBuffer( base, buffer );
	va_list argptr;
	va_start(argptr, count);
	for(i=0;i < count;i++){
	    key = va_arg(argptr, char*);
	    value = va_arg(argptr, char*);
	    XML_Escape(smallbuff,sizeof(smallbuff),value);

		Com_sprintf(buffer,sizeof(buffer)," %s=\"%s\"",key,smallbuff);

	    XML_AppendToBuffer( base, buffer );
	}
	va_end(argptr);
	XML_AppendToBuffer( base, ">" );
	base->parents++;
	base->last = qtrue;
	return qtrue;
}

/*
 ==================
 XML_CloseTag

 Changes or adds a key/value pair
 ==================
 */
void XML_CloseTag(xml_t *base) {

	char buffer[256];
	char outbuffer[256];
	char preoffset[128];
	int i;
	char*	root;
	char*	stringptr = buffer;

	if(base->parents == 0)
	{
		Com_PrintError("XML_CloseTag: Invalid close attempt in XML. Attempt to close more elements than you have opened.\n");
		Com_Printf("Printing up to 960 recent characters of XML as debug:\n");
		if(base->bufposition > 960)
		{
			Com_Printf("%s\n", &base->buffer[base->bufposition -960]);
		}else{
			Com_Printf("%s\n", base->buffer);
		}
		Com_PrintError("You have errors in your XML code\n");
		return;
	}
	base->parents--;
	Com_Memset(&preoffset[1],' ',base->parents*6);
	preoffset[base->parents*6] = 0;
	preoffset[(base->parents*6)+1] = 0;

	buffer[0] = '\0';

	root = stack_pop(base->stack,sizeof(base->stack));
	for(i=0 ;*root != ' ' && *root != 0 && *root != '>' && i < sizeof(buffer); stringptr++, root++, i++) *stringptr = *root;
	*stringptr = 0;
	if(base->last){
		Com_sprintf(outbuffer,sizeof(outbuffer),"</%s>",buffer);
	}else{
		Com_sprintf(outbuffer,sizeof(outbuffer),"\n%s</%s>",&preoffset[1],buffer);
	}

	XML_AppendToBuffer( base, outbuffer );
	base->last = qfalse;
}

//====================================================================

/*
==================
Com_CharIsOneOfCharset
==================
*/
static qboolean Com_CharIsOneOfCharset( char c, char *set )
{
	int i;

	for( i = 0; i < strlen( set ); i++ )
	{
		if( set[ i ] == c )
			return qtrue;
	}

	return qfalse;
}

/*
==================
Com_SkipCharset
==================
*/
char *Com_SkipCharset( char *s, char *sep )
{
	char	*p = s;

	while( p )
	{
		if( Com_CharIsOneOfCharset( *p, sep ) )
			p++;
		else
			break;
	}

	return p;
}

/*
==================
Com_SkipTokens
==================
*/
char *Com_SkipTokens( char *s, int numTokens, char *sep )
{
	int		sepCount = 0;
	char	*p = s;

	while( sepCount < numTokens )
	{
		if( Com_CharIsOneOfCharset( *p++, sep ) )
		{
			sepCount++;
			while( Com_CharIsOneOfCharset( *p, sep ) )
				p++;
		}
		else if( *p == '\0' )
			break;
	}

	if( sepCount == numTokens )
		return p;
	else
		return s;
}



char* Q_BitConv(int val){

	int i, j;
	static char buf[40];
	memset(buf,'?',sizeof(buf));

	for(j = 0, i = 38; i >= 0; i--){

		if((i+1) % 5 == 0 && i != 38 && i != 0){
			buf[i] = ' ';
		}else if(val & 1 << j){
			buf[i] = '1';
			j++;
		}else{
			buf[i] = '0';
			j++;
		}
	}
	buf[39] = 0;
	return buf;
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char *path, int maxSize, const char *extension ) {
	char oldPath[MAX_QPATH];
	char    *src;

//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen( path ) - 1;

	while ( *src != '/' && src != path ) {
		if ( *src == '.' ) {
			return;                 // it has an extension
		}
		src--;
	}

	Q_strncpyz( oldPath, path, sizeof( oldPath ) );
	Com_sprintf( path, maxSize, "%s%s", oldPath, extension );
}

qboolean I_IsEqualUnitWSpace(char *cmp1, char *cmp2)
{

	while ( 1 )
	{
		if ( !(*cmp1) || !(*cmp2) )
			break;

		if ( *cmp1 == ' ' || *cmp2 == ' ' )
			break;

		if ( *cmp1 != *cmp2 )
			return qfalse;

		cmp1++;
		cmp2++;
	}

	if ( *cmp1 && *cmp1 != ' ')
	{
		return qfalse;
	}
	if ( *cmp2 && *cmp2 != ' ')
	{
		return qfalse;
	}

	return 1;
}

unsigned char I_CleanChar(unsigned char in)
{
  if(in == 146)
    return 39;

  return in;
}


/*
=====================================================================

 Functions to verify the variable type of string

=====================================================================
*/



qboolean isFloat(const char* string, int size)
{
    const char* ptr;
    int i;
    qboolean dot = qfalse;
    qboolean sign = qfalse;
    qboolean whitespaceended = qfalse;

    if(size == 0) //If we have given a length compare the whole string
        size = 0x10000;

    for(i = 0, ptr = string; i < size && *ptr != '\0' && *ptr != '\n'; i++, ptr++){

        if(*ptr == ' ')
        {
            if(whitespaceended == qfalse)
                continue;
            else
                return qtrue;
        }
        whitespaceended = qtrue;

        if(*ptr == '-' && sign ==0)
        {
            sign = qtrue;
            continue;
        }
        if(*ptr == '.' && dot == 0)
        {
            dot = qtrue;
            continue;
        }
        if(*ptr < '0' || *ptr > '9') return qfalse;
    }
    return qtrue;
}


qboolean isInteger(const char* string, int size)
{
    const char* ptr;
    int i;
    qboolean sign = qfalse;
    qboolean whitespaceended = qfalse;

    if(size == 0) //If we have given a length compare the whole string
        size = 0x10000;

    for(i = 0, ptr = string; i < size && *ptr != '\0' && *ptr != '\n' && *ptr != '\r'; i++, ptr++){

        if(*ptr == ' ')
        {
            if(whitespaceended == qfalse)
                continue;
            else
                return qtrue;
        }
        whitespaceended = qtrue;

        if(*ptr == '-' && sign ==0)
        {
            sign = qtrue;
            continue;
        }
        if(*ptr < '0' || *ptr > '9') return qfalse;
    }
    return qtrue;
}

qboolean isVector(const char* string, int size, int dim)
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

        if(isFloat(ptr, size -i) == qfalse)
            return qfalse;

        while(*ptr != ' ' && *ptr != '\0' && *ptr != '\n' && i < size)
        {
            ptr++; i++;
        }
    }
    if(dim != 0)
        return qfalse;

    return qtrue;
}


qboolean strToVect(const char* string, float *vect, int dim)
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
        return qfalse;

    return qtrue;
}

void CharToHexUni(wchar_t* string, char* hexstring, int size)
{
	int i;

	for(i = 0 ; string[i] != L'\0' && i < size - 5; i += 1)
	{
		sprintf(&hexstring[4*i], "%02x", ((char*)(&string[i]))[0]);
		sprintf(&hexstring[4*i+2], "%02x", ((char*)(&string[i]))[1]);
	}
	hexstring[4*i] = '\0';
}

void HexToCharUni(char* hexstring, wchar_t* string, int size)
{
	int i;
	char chrByte[3];

	for(i = 0 ; hexstring[2*i] != '\0' && hexstring[2*i+1] != '\0' && i < size -2; i++)
	{
		chrByte[0] = hexstring[2*i];
		chrByte[1] = hexstring[2*i+1];
		chrByte[2] = '\0';

		((char*)string)[i] = strtol(chrByte,NULL,16);
	}
	string[i] = '\0';
	string[i+1] = '\0';
}


const char* Com_GetFilenameSubString(const char* arg)
{
  char* stub;
  int i;

  stub = (char*)arg;

  for ( i = 0; arg[i]; ++i )
  {
    if ( arg[i] == '/' || arg[i] == '\\' )
	{
		stub = (char*)&arg[i];
	}
  }
  return stub;
}


int COM_Compress( char *data_p ) {
	char *datai, *datao;
	int c, size;
	qboolean ws = qfalse;

	size = 0;
	datai = datao = data_p;
	if ( datai ) {
		while ( ( c = *datai ) != 0 ) {
			if ( c == 13 || c == 10 ) {
				*datao = c;
				datao++;
				ws = qfalse;
				datai++;
				size++;
				// skip double slash comments
			} else if ( c == '/' && datai[1] == '/' ) {
				while ( *datai && *datai != '\n' ) {
					datai++;
				}
				ws = qfalse;
				// skip /* */ comments
			} else if ( c == '/' && datai[1] == '*' ) {
				while ( *datai && ( *datai != '*' || datai[1] != '/' ) )
				{
					datai++;
				}
				if ( *datai ) {
					datai += 2;
				}
				ws = qfalse;
			} else {
				if ( ws ) {
					*datao = ' ';
					datao++;
				}
				*datao = c;
				datao++;
				datai++;
				ws = qfalse;
				size++;
			}
		}
	}
	*datao = 0;
	return size;
}

/*
=====================================================================

 Functions to create / read / write a ringbuffer.
 Code used from: http://c.learncodethehardway.org/book/ex44.html
=====================================================================
*/
#if 0
RingBuffer *RingBuffer_create(int length)
{
    RingBuffer *buffer = malloc(sizeof(RingBuffer) + length);
    buffer->length  = length + 1;
    buffer->start = 0;
    buffer->end = 0;
    buffer->buffer = (byte*)&buffer[1];

    return buffer;
}

void RingBuffer_destroy(RingBuffer *buffer)
{
    if(buffer) {
        free(buffer);
    }
}

int RingBuffer_write(RingBuffer *buffer, char *data, unsigned int length)
{

	int wrap;

    check(length <= RingBuffer_available_space(buffer),
            "Not enough space: %d request, %d available",
            RingBuffer_available_data(buffer), length);

	wrap = buffer->length - buffer->end;
	memcpy(RingBuffer_ends_at(buffer), data, length);

    RingBuffer_commit_write(buffer, length);

    return length;
error:
    return -1;
}

int RingBuffer_read(RingBuffer *buffer, char *target, unsigned int amount)
{
    if(amount <= RingBuffer_available_data(buffer))
	{
		amount = RingBuffer_available_data(buffer);
	}
    memcpy(target, RingBuffer_starts_at(buffer), amount);

    RingBuffer_commit_read(buffer, amount);

    return amount;
}
#endif
