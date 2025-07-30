#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <new>
#include <sys/types.h>
#include <sys/stat.h>

#include "q_shared.h"
#include "sys_main.h"
#include "tomcrypt/tomcrypt.h"


#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

void Com_Printf(const char* fmt, ...)
{
    char line[8192];
    va_list ap;
    va_start(ap, fmt);

    vsnprintf(line, sizeof(line), fmt, ap);

    va_end(ap);

    printf("%s", line);

}

void Com_DPrintf(const char* fmt, ...)
{
    char line[8192];
    va_list ap;
    va_start(ap, fmt);

    vsnprintf(line, sizeof(line), fmt, ap);

    va_end(ap);

    printf("%s", line);

}

void Com_PrintError(const char* fmt, ...)
{
    char line[8192];
    va_list ap;
    va_start(ap, fmt);

    vsnprintf(line, sizeof(line), fmt, ap);

    va_end(ap);

    printf("Error: %s", line);

}

void Com_PrintWarning(const char* fmt, ...)
{
    char line[8192];
    va_list ap;
    va_start(ap, fmt);

    vsnprintf(line, sizeof(line), fmt, ap);

    va_end(ap);

    printf("Warning: %s", line);

}


int Com_sprintf(char *dest, int size, const char *fmt, ...)
{
	int		len;
	va_list		argptr;

	va_start (argptr,fmt);
	len = vsnprintf(dest, size, fmt, argptr);
	va_end (argptr);

	if(len >= size)
		Com_Printf("Com_sprintf: Output length %d too short, require %d uint8_ts.\n", size, len + 1);
	
	return len;
}

/*
=============
Q_strncpyz
 
Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize ) {

	if (!dest ) {
	    Com_PrintError("Q_strncpyz: NULL dest" );
        return;
	}
	if ( !src ) {
		Com_PrintError("Q_strncpyz: NULL src" );
        return;        
	}
	if ( destsize < 1 ) {
		Com_PrintError("Q_strncpyz: destsize < 1" );
        return;        
	}

	strncpy( dest, src, destsize-1 );
  dest[destsize-1] = 0;
}

// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_PrintError( "Q_strcat: already overflowed" );
        return;
	}
	Q_strncpyz( dest + l1, src, size - l1 );
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

char* CopyString(const char* s)
{
	char* ms = new char[strlen(s) +1];
	strcpy(ms, s);
	return ms;
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

#ifndef _stricmp
int _stricmp(const char* s1, const char* s2)
{
	return Q_stricmpn (s1, s2, 0x7fffffff);
}
#endif


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void FS_CreatePath (char *OSPath)
{
	char	*ofs;

	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
		Com_Printf( "WARNING: refusing to create relative path \"%s\"\n", OSPath );
		return;
	}

	for (ofs = OSPath+1 ; *ofs ; ofs++) {
		if (*ofs == PATH_SEP) {	
			// create the directory
			*ofs = 0;
			Sys_Mkdir (OSPath);
			*ofs = PATH_SEP;
		}
	}
	return;
}


/*
====================
FS_ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
void FS_ReplaceSeparators( char *path ) {
	char	*s;

	for ( s = path ; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = PATH_SEP;
		}
	}
}

void FS_TurnSeparatorsForward( char *path ) {
	char	*s;

	for ( s = path ; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = '/';
		}
	}
}

/*
====================
FS_StripTrailingSeparator

Fix things up differently for win/unix/mac
====================
*/
void FS_StripTrailingSeparator( char *path )
{

	while(true)
	{
		int len = strlen(path);
		if(len < 1)
		{
			return;
		}
		if(path[len -1] == '\\' || path[len -1] == '/')
		{
			path[len -1] = '\0';
		}else{
			break;
		}
	}
}

void FS_StripLeadingSeparator( char *path )
{

	char* op = path;

	while(*path == '\\' || *path == '/' || (*path <= ' ' && *path))
	{
		++path;
	}
	if(op != path)
	{
		memmove(op, path, strlen(path) +1);
	}
}

void FS_BuildOSPath(const char *base, const char *path1, const char *path2, char *fs_path, int maxlen)
{
  char basename[1024];
  char p1name[1024];

  int len;

  if ( !path1 || !*path1 )
    path1 = "";

  Q_strncpyz(basename, base, sizeof(basename));
  Q_strncpyz(p1name, path1, sizeof(p1name));

  len = strlen(basename);
  if(len > 0 && (basename[len -1] == '/' || basename[len -1] == '\\'))
  {
        basename[len -1] = '\0';
  }

  len = strlen(p1name);
  if(len > 0 && (p1name[len -1] == '/' || p1name[len -1] == '\\'))
  {
        p1name[len -1] = '\0';
  }
  if ( Com_sprintf(fs_path, maxlen, "%s/%s/%s", basename, p1name, path2) >= maxlen )
  {
    Com_Error("FS_BuildOSPath: os path maxlen exceeded");
  }
  FS_ReplaceSeparators(fs_path);
  FS_StripTrailingSeparator(fs_path);

}


/*
===========
FS_FOpenFileWriteOSPath
===========
*/
FILE* FS_FOpenFileWriteOSPath( const char *filename ) {
	char ospath[1024];
	FILE* fh;
	
	Q_strncpyz( ospath, filename, sizeof( ospath ) );

	FS_ReplaceSeparators(ospath);
	FS_StripTrailingSeparator(ospath);

	FS_CreatePath(ospath);


	fh = fopen( ospath, "wb" );

	return fh;
}

/*
===========
FS_FOpenFileReadOSPath
===========
*/
int64_t FS_FOpenFileReadOSPath( const char *filename, FILE **fp ) {
	char ospath[1024];
	FILE* fh;
	
	Q_strncpyz( ospath, filename, sizeof( ospath ) );

	FS_ReplaceSeparators(ospath);
	FS_StripTrailingSeparator(ospath);

	fh = fopen( ospath, "rb" );

	if ( !fh ){
		*fp = NULL;
		return -1;
	}

	*fp = fh;

	return FS_GetFileSize(ospath);
}

/*
==============
FS_FCloseFileOSPath
==============
*/
bool FS_FCloseFileOSPath( FILE* f ) {

	if (f) {
	    fclose (f);
	    return true;
	}
	return false;
}


/*
=================
FS_ReadOSPath

Properly handles partial reads
=================
*/
int FS_ReadOSPath( void *buffer, int len, FILE* f ) {
	int		block, remaining;
	int		read;
	uint8_t	*buf;

	if ( !f ) {
		return 0;
	}

	buf = (uint8_t *)buffer;

	remaining = len;
	while (remaining) {
		block = remaining;
		read = fread (buf, 1, block, f);
		if (read == 0)
		{
			return len-remaining;	//Com_Error ("FS_Read: 0 uint8_ts read");
		}

		if (read == -1) {
			Com_Error ("FS_ReadOSPath: -1 uint8_ts read");
		}

		remaining -= read;
		buf += read;
	}
	return len;

}


bool FS_WriteOSPath( void *buffer, unsigned int len, FILE* f )
{

  if ( fwrite( buffer, 1, len, f ) != len )
  {
	Com_PrintError("Short write in FS_WriteOSPath()\n");
	return false;
  }
  return true;
}

/* returns Blake2s hash and file length 
hash need to be size of BLAKE2S_OUTBYTES
*/

uint64_t FS_FileHash(const char* ospath, uint8_t* hash, int outlength)
{
	uint8_t	buf[0x20000];
	int		readlen;
	uint64_t totallen;
	FILE*   h; 
	hash_state S;
	  
	if ( !ospath || !ospath[0] ) {
		Com_Error("FS_FileHash with empty name\n" );
	}
	
	if ( !hash || outlength < BLAKE2S_OUTBYTES ) {
		Com_Error("FS_FileHash with invalid argument\n" );
	}	
		
	// look for it in the filesystem or pack files
	int64_t len = FS_FOpenFileReadOSPath( ospath, &h );
	if ( len == -1 ) {
		return -1;
	}

  if( blake2s_init( &S, BLAKE2S_OUTBYTES, NULL, 0 ) != CRYPT_OK )
	{
		Com_Error("blake2s_init() has failed");
	}
	readlen = 0;
	totallen = 0;
	do
	{
		totallen += (uint64_t)readlen;
		readlen = FS_ReadOSPath (buf, sizeof(buf), h);
		blake2s_process( &S, ( uint8_t * )buf, readlen );
	}while(readlen > 0);

	blake2s_done( &S, hash );
	
	FS_FCloseFileOSPath( h );
	
	return totallen;
}

void Blakesum2String(uint8_t* checksum, char* hstring, int maxsize)
{
	int i;

    if(2* BLAKE2S_OUTBYTES +1 > maxsize)
	{
		hstring[0] = 0;
		return;
	}
	for(i = 0; i < BLAKE2S_OUTBYTES; ++i)
	{
		sprintf(hstring + 2*i, "%02X", checksum[i]);
	}
}


_Noreturn void Com_Error(const char* fmt, ...)
{
    char line[8192];
    va_list ap;
    va_start(ap, fmt);

    vsnprintf(line, sizeof(line), fmt, ap);

    va_end(ap);

    printf("System Error: %s\n", line);
	exit(-1);
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
	unsigned char* buffer = malloc (sizeof(char) * (lSize +1));
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
