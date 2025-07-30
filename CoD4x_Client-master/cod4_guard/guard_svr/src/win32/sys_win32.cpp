/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm

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
#include "../q_shared.h"
#include "../sys_main.h"
#include "../sys_thread.h"

#include <windows.h>
#include <wincrypt.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <io.h>
#include <sys/stat.h>

void Sys_ShowErrorDialog(const char* functionName);

/*
================
Sys_RandomBytes
================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	HCRYPTPROV  prov;

	if( !CryptAcquireContext( &prov, NULL, NULL,
		PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )  {

		return qfalse;
	}

	if( !CryptGenRandom( prov, len, (BYTE *)string ) )  {
		CryptReleaseContext( prov, 0 );
		return qfalse;
	}
	CryptReleaseContext( prov, 0 );
	return qtrue;
}

/*
==================
Sys_Mkdir
==================
*/
bool Sys_Mkdir( const char *path )
{

	int result = _mkdir( path );

	if( result != 0 && errno != EEXIST)
		return false;

	return true;
}

qboolean Sys_SetPermissionsExec(const char* ospath)
{
	return qtrue;
}

/*
==================
Sys_SleepSec
==================
*/

void Sys_SleepMSec(int msec)
{
    Sleep(msec);
}

void Sys_SleepSec(int seconds)
{
    Sleep(1000*seconds);
}

void Sys_ShowErrorDialog(const char* functionName)
{
	HWND hwnd = NULL;
	char errMessageBuf[1024];
	char displayMessageBuf[1024];
	DWORD lastError = GetLastError();

	if(lastError != 0)
	{
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), errMessageBuf, sizeof(errMessageBuf) -1, NULL);
	}else{
		Q_strncpyz(errMessageBuf, "Unknown Error", sizeof(errMessageBuf));
	}

	Com_sprintf(displayMessageBuf, sizeof(displayMessageBuf), "Error in function: %s\nThe error is: %s", functionName, errMessageBuf);

	MessageBoxA(hwnd, displayMessageBuf, "System Error", MB_OK | MB_ICONERROR);
}

const char *Sys_DefaultHomePath( void ) {
	return NULL;
}



/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

/*
==============
strgtr
==============
*/

static qboolean strgtr(const char *s0, const char *s1)
{
	int l0, l1, i;

	l0 = strlen(s0);
	l1 = strlen(s1);

	if (l1<l0) {
		l0 = l1;
	}

	for(i=0;i<l0;i++) {
		if (s1[i] > s0[i]) {
			return qtrue;
		}
		if (s1[i] < s0[i]) {
			return qfalse;
		}
	}
	return qfalse;
}


int Sys_ListFilesWriteInfo( const char* basepath, const char* searchpath, char** list, int maxentries)
{
	intptr_t	findhandle;
	struct _finddata_t findinfo;
	int	nfiles;
	char search[1024];
	char filename[1024];
	char cbasepath[1024];
	nfiles = 0;

	Q_strncpyz(cbasepath, basepath, sizeof(cbasepath));

	int bl = strlen(cbasepath);
	if(bl > 0 && cbasepath[bl -1] == '\\')
	{
		cbasepath[bl -1] = '\0';
	}

	if(strlen(basepath) + strlen(searchpath) + 3 >= sizeof(search))
	{
		return 0;
	}

	search[0] = 0;

	if(cbasepath[0])
	{
		strcat(search, cbasepath);
		strcat(search, "\\");
	}
	if(searchpath[0])
	{
		strcat(search, searchpath);
		strcat(search, "\\");
	}
	strcat(search, "*");

	memset(&findinfo, 0, sizeof(findinfo));

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		return nfiles;
	}

	do {
		if(searchpath[0])
		{
			Com_sprintf(filename, sizeof(filename), "%s\\%s", searchpath, findinfo.name);
		}else{
			Q_strncpyz(filename, findinfo.name, sizeof(filename));
		}
		if ( findinfo.attrib & _A_SUBDIR )
		{
			if(strcmp(findinfo.name, ".") == 0 || strcmp(findinfo.name, "..") == 0)
			{
				continue; //Exclude . and .. dirs
			}
			char** reclist = NULL;
			//Rekusion
			if(list)
			{
				reclist = list + nfiles;
			}
			nfiles += Sys_ListFilesWriteInfo( basepath, filename, reclist, maxentries - nfiles);
		}else{
			if ( nfiles == maxentries - 1 && list) {
				break;
			}
			if(list)
			{
				list[ nfiles ] = CopyString( filename );
			}
			nfiles++;
		}
	} while ( _findnext (findhandle, &findinfo) != -1 );

	if(list)
	{
		list[ nfiles ] = 0;
	}
	_findclose (findhandle);
	
	return nfiles;
}


/*
==============
Sys_ListFiles
==============
*/
char **Sys_ListFiles( const char *directory, int *numfiles )
{
	int	nfiles;
	char	**listCopy;
	int	flag;
	int	i;

	int count = Sys_ListFilesWriteInfo( directory, "", NULL, 0);

	if(count <= 0)
	{
		*numfiles = 0;
		return NULL;
	}

	listCopy = (char**)Z_Malloc( ( count + 1 ) * sizeof( *listCopy ) );

	// search
	nfiles = Sys_ListFilesWriteInfo(directory, "", listCopy, count);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}
	listCopy[nfiles] = NULL;

	do {
		flag = 0;
		for(i=1; i<nfiles; i++) {
			if (strgtr(listCopy[i-1], listCopy[i])) {
				char *temp = listCopy[i];
				listCopy[i] = listCopy[i-1];
				listCopy[i-1] = temp;
				flag = 1;
			}
		}
	} while(flag);

	return listCopy;
}

/*
==============
Sys_FreeFileList
==============
*/
void Sys_FreeFileList( char **list )
{
	int i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		FreeString( list[i] );
	}

	Z_Free( list );
}


qboolean Sys_DirectoryHasContent(const char *dir)
{
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    char searchpath[MAX_OSPATH];

	if(strlen(dir) > MAX_OSPATH - 6 || dir[0] == '\0')
		return qfalse;

    Q_strncpyz(searchpath, dir, sizeof(searchpath));
	if( searchpath[strlen(searchpath) -1] ==  '\\' )
	{
		searchpath[strlen(searchpath) -1] = '\0';
	}
	Q_strcat(searchpath, sizeof(searchpath), "\\*");

    if((hFind = FindFirstFile(searchpath, &fdFile)) == INVALID_HANDLE_VALUE)
    {
        return qfalse;
    }

    do
    {
        if(_stricmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0)
        {
			FindClose(hFind);
			return qtrue;
        }
    }
    while(FindNextFile(hFind, &fdFile));

    FindClose(hFind);

    return qfalse;
}

uint32_t Sys_MillisecondsRaw()
{
	return timeGetTime();
}


/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}


static CRITICAL_SECTION crit_sections[CRIT_SIZE];
threadid_t mainthread;


void Sys_InitializeCriticalSections( void )
{
	int i;

	for (i = 0; i < CRIT_SIZE; i++) {
		InitializeCriticalSection( &crit_sections[i] );

	}

}

void __cdecl Sys_ThreadInit( void )
{
	mainthread = GetCurrentThreadId();
}


void __cdecl Sys_EnterCriticalSectionInternal(int section)
{
	EnterCriticalSection(&crit_sections[section]);
}

void __cdecl Sys_LeaveCriticalSectionInternal(int section)
{
	LeaveCriticalSection(&crit_sections[section]);
}


qboolean Sys_CreateNewThread(void* (*ThreadMain)(void*), threadid_t *tid, void* arg)
{
	char errMessageBuf[512];
	DWORD lastError;


	HANDLE thid = CreateThread(	NULL, // LPSECURITY_ATTRIBUTES lpsa,
								0, // DWORD cbStack,
								(LPTHREAD_START_ROUTINE)ThreadMain, // LPTHREAD_START_ROUTINE lpStartAddr,
								arg, // LPVOID lpvThreadParm,
								0, // DWORD fdwCreate,
								tid );

	if(thid == NULL)
	{
		lastError = GetLastError();

		if(lastError != 0)
		{
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR)errMessageBuf, sizeof(errMessageBuf) -1, NULL);
			Com_PrintError("Failed to start thread with error: %s\n", errMessageBuf);

		}else{
			Com_PrintError("Failed to start thread!\n");
		}
		return qfalse;
	}
	CloseHandle(thid);
	return qtrue;
}


qboolean __cdecl Sys_IsMainThread( void )
{
	return Sys_ThreadisSame(mainthread);
}

qboolean Sys_ThreadisSame(threadid_t threadid)
{
	threadid_t thread = GetCurrentThreadId();

	return (qboolean)(threadid == thread);

}

void Sys_ExitThread(int code)
{
	ExitThread( code );

}

threadid_t __cdecl Sys_GetCurrentThreadId( void )
{
		return GetCurrentThreadId();
}


/*
==================
Sys_Backtrace
==================
*/
int Sys_Backtrace(void** buffer, int size)
{
    return 0;
}

void  __attribute__ ((noreturn)) Sys_ExitForOS( int exitCode )
{
	ExitProcess( exitCode );
}

int Sys_Chmod(const char* filename, int mode)
{
    return _chmod(filename, mode);
}

void Sys_SleepUSec(int usec)
{
	Sleep((usec + 999) / 1000);
}


int64_t FS_GetFileModifiedTime(const char* ospath)
{
	struct _stati64 statr;
	if(_stati64(ospath, &statr) == 0)
	{
		return statr.st_mtime;
	}
	return 0;
}

int64_t FS_GetFileSize(const char* ospath)
{
	struct _stati64 statr;
	if(_stati64(ospath, &statr) == 0)
	{
		return statr.st_size;
	}
	return -1;
}

