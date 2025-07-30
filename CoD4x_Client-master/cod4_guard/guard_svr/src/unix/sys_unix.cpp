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


#include "../q_shared.h"
#include "../sys_main.h"
#include "../sys_thread.h"

#include <sys/resource.h>
#include <libgen.h>
#include <signal.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <execinfo.h>
#include <sys/time.h>
#include<pthread.h>

/*
==================
Sys_RandomBytes
==================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	FILE *fp;

	fp = fopen( "/dev/urandom", "r" );
	if( !fp )
		return qfalse;

	if( !fread( string, sizeof( byte ), len, fp ) )
	{
		fclose( fp );
		return qfalse;
	}

	fclose( fp );
	return qtrue;
}

unsigned int Sys_MillisecondsRaw( void )
{
	struct timeval tp;

	gettimeofday( &tp, NULL );

	return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}



/*
==================
Sys_Dirname
==================
*/
const char *Sys_Dirname( char *path )
{
	return dirname( path );
}


/*
==================
Sys_Cwd
==================
*/
char *Sys_Cwd( void )
{
	static char cwd[MAX_OSPATH];

	char *result = getcwd( cwd, sizeof( cwd ) - 1 );
	if( result != cwd )
		return NULL;

	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}



void Sys_InitCrashDumps(){

        // core dumps may be disallowed by parent of this process; change that

        struct rlimit core_limit;
        core_limit.rlim_cur = RLIM_INFINITY;
        core_limit.rlim_max = RLIM_INFINITY;

        if (setrlimit(RLIMIT_CORE, &core_limit) < 0)
            Com_PrintWarning("setrlimit: %s\nCore dumps may be truncated or non-existant\n", strerror(errno));

}

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


/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define MAX_FOUND_FILES 0x1000

int Sys_ListFilesWriteInfo( const char* basepath, const char* searchpath, char** list, int maxentries)
{
    DIR* findhandle;
    struct dirent* findinfo;
    int	nfiles;
    char search[1024];
    char filename[1024];
    char cbasepath[1024];
    nfiles = 0;

    Q_strncpyz(cbasepath, basepath, sizeof(cbasepath));

    int bl = strlen(cbasepath);
    if(bl > 0 && cbasepath[bl -1] == '/')
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
    }
    if(searchpath[0])
    {
	strcat(search, searchpath);
    }
    if(search[0] == 0)
    {
	search[0] = '.';
	search[1] = '\0';
    }
    findhandle = opendir(search);
    if (findhandle == NULL) {
	return nfiles;
    }

    while( (findinfo = readdir(findhandle)) != NULL ) {
	if(searchpath[0])
	{
	    Com_sprintf(filename, sizeof(filename), "%s/%s", searchpath, findinfo->d_name);
	}else{
	    Q_strncpyz(filename, findinfo->d_name, sizeof(filename));
	}
	struct stat fstat;

	if(stat(filename, &fstat) < 0)
	{
		continue;
	}
	if ( S_ISDIR(fstat.st_mode) )
	{
	    if(strcmp(findinfo->d_name, ".") == 0 || strcmp(findinfo->d_name, "..") == 0)
	    {
		continue; //Exclude . and .. dirs
	    }
	    char** reclist = NULL;
	    //Rekursion
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
    } 

    if(list)
    {
	list[ nfiles ] = 0;
    }
    closedir(findhandle);
    
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


qboolean Sys_DirectoryHasContent(const char *dir)
{
  DIR *hdir;
  struct dirent *hfiles;

  hdir = opendir(dir);
  if ( hdir )
  {
	hfiles = readdir(hdir);
    while ( hfiles )
    {
        if ( hfiles->d_reclen != 4 || hfiles->d_name[0] != '.' )
		{
	          closedir(hdir);
		  return qtrue;
		}
		hfiles = readdir(hdir);
    }
	closedir(hdir);
  }
  return qfalse;
}



/*
==================
Sys_FreeFileList
==================
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

/*
================
Sys_GetUsername
================
*/
const char* Sys_GetUsername()
{
        struct passwd *passwdstr = getpwuid(getuid());

        if(passwdstr == NULL)
            return "CoD-Admin";

        return passwdstr->pw_name;

}


/*
==================
Sys_Basename
==================
*/
const char *Sys_Basename( char *path )
{
	return basename( path );
}

/*
==================
Sys_Mkdir
==================
*/
bool Sys_Mkdir( const char *path )
{
	int result = mkdir( path, 0750 );

	if( result != 0 )
		return errno == EEXIST;

	return true;
}

qboolean Sys_SetPermissionsExec(const char* ospath)
{
        if(chmod(ospath, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)!=0){
		return qfalse;
        }
	return qtrue;
}


/*
==================
Sys_SleepSec
==================
*/

void Sys_SleepMSec(int msec)
{
		struct timespec ts;
		ts.tv_sec = msec / 1000;
		ts.tv_nsec = (msec % 1000) * 1000000;
		nanosleep(&ts, NULL);
}

void Sys_SleepSec(int seconds)
{
	struct timespec ts;
	ts.tv_sec = seconds;
	ts.tv_nsec = 0;
	nanosleep(&ts, NULL);
}

void Sys_SleepUSec(int usec)
{
	usleep(usec);
}

/*
==================
Sys_Backtrace
==================
*/

int Sys_Backtrace(void** buffer, int size)
{
    return backtrace(buffer, size);
}

void Sys_EventLoop()
{

}

void Sys_WaitForErrorConfirmation(const char* error)
{

}

void* currentLibHandle = NULL;

void* Sys_LoadLibrary(const char* dlfile)
{
	void* handle = dlopen(dlfile, RTLD_LAZY);
	currentLibHandle = handle;
	if(handle == NULL)
	{
		Com_PrintError("Sys_LoadLibrary error: %s\n", dlerror());
	}
	return handle;
}

void* Sys_GetProcedure(const char* lpProcName)
{
	if(currentLibHandle == NULL)
	{
		Com_Printf("Attempt to get ProcAddress from invalid or not loaded library");
		return NULL;
	}
	void* procedure = dlsym( currentLibHandle, lpProcName );
	return procedure;
}

void Sys_CloseLibrary(void* hModule)
{
	if(hModule == NULL)
	{
		Com_PrintError("Attempt to close not loaded library");
		return;
	}
	if(hModule == currentLibHandle)
	{
		currentLibHandle = NULL;
	}
	dlclose(hModule);
}

#define MIN_STACKSIZE 256*1024
qboolean Sys_CreateNewThread(void* (*ThreadMain)(void*), threadid_t *tid, void* arg)
{
	int err;
	pthread_attr_t tattr;

	err = pthread_attr_init(&tattr);
	if(err != 0)
	{
		Com_PrintError("pthread_attr_init(): Thread creation failed with the following error: %s\n", strerror(errno));
		return qfalse;
	}

	err = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	if(err != 0)
	{
		pthread_attr_destroy(&tattr);
		Com_PrintError("pthread_attr_setdetachstate(): Thread creation failed with the following error: %s\n", strerror(errno));
		return qfalse;
	}

	err = pthread_attr_setstacksize(&tattr, MIN_STACKSIZE);
	if(err != 0)
	{
		pthread_attr_destroy(&tattr);
		Com_PrintError("pthread_attr_setstacksize(): Thread creation failed with the following error: %s\n", strerror(errno));
		return qfalse;
	}

	err = pthread_create(tid, &tattr, ThreadMain, arg);
	if(err != 0)
	{
		pthread_attr_destroy(&tattr);
		Com_PrintError("pthread_create(): Thread creation failed with the following error: %s\n", strerror(errno));
		return qfalse;
	}
	pthread_attr_destroy(&tattr);
	return qtrue;
}


static pthread_mutex_t crit_sections[CRIT_SIZE];
threadid_t mainthread;

void Sys_InitializeCriticalSections( void )
{
	int i;
	pthread_mutexattr_t muxattr;

	pthread_mutexattr_init(&muxattr);
	pthread_mutexattr_settype(&muxattr, PTHREAD_MUTEX_RECURSIVE);

	for (i = 0; i < CRIT_SIZE; i++) {
		pthread_mutex_init( &crit_sections[i], &muxattr );

	}

	pthread_mutexattr_destroy(&muxattr);

}

void __cdecl Sys_EnterCriticalSectionInternal(int section)
{
	pthread_mutex_lock(&crit_sections[section]);
}

void __cdecl Sys_LeaveCriticalSectionInternal(int section)
{
	pthread_mutex_unlock(&crit_sections[section]);
}


qboolean __cdecl Sys_IsMainThread( void )
{
	return Sys_ThreadisSame(mainthread);
}

qboolean Sys_ThreadisSame(threadid_t threadid)
{
	threadid_t thread = pthread_self();

	return (qboolean)(pthread_equal(threadid, thread) != 0);

}

threadid_t Sys_GetCurrentThreadId( )
{
	threadid_t thread = pthread_self();

	return thread;

}

void Sys_ExitThread(int code)
{
	pthread_exit(&code);

}

void  __attribute__ ((noreturn)) Sys_ExitForOS( int exitCode )
{
	exit(exitCode);
}

int Sys_Chmod(const char* file, int mode)
{
    return chmod(file, mode);

}


void* Sys_RunNewProcess(void* arg)
{
	char cmdline[4096];

	Q_strncpyz(cmdline, (const char*)arg, sizeof(cmdline));
	free(arg);
	system(cmdline);
	return NULL;
}


void Sys_DoStartProcess( char *cmdline ) {

	threadid_t tid;

	void* mcmdline = (void*)strdup(cmdline);
	if(mcmdline == NULL)
	{
		return;
	}
	if(Sys_CreateNewThread(Sys_RunNewProcess, &tid, mcmdline) == qfalse)
	{
		free(mcmdline);
	}

}


int64_t FS_GetFileModifiedTime(const char* ospath)
{
	struct stat statr;
	if(stat(ospath, &statr) == 0)
	{
		return statr.st_mtime;
	}
	return 0;
}

int64_t FS_GetFileSize(const char* ospath)
{
	struct stat statr;
	if(stat(ospath, &statr) == 0)
	{
		return statr.st_size;
	}
	return -1;
}

