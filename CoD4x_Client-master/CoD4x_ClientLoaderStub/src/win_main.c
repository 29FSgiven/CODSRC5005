#include "q_shared.h"
#include "win_sys.h"
#include "sys_patch.h"
#include "sec_sign.h"
#include "zlib/zlib.h"
#include "sec_crypto.h"
#include <stdbool.h>
#include <windows.h>
#include <Wininet.h>
#include <WinBase.h>
#include <Shellapi.h>
#include <shlobj.h>
#include <aclapi.h>

void Sys_RunUpdater(  );
void ReplaceMainModule();
void ReplaceMiles32Module();
bool HookEntryPointForInstaller();
bool HookEntryPointForMSSInstaller();
qboolean LargeAddressspacePatchEnabled();
bool HookEntryPointForIW3Upd();
void UpdateIW3MPInternal();

bool xoxorPresent; //bypass

CRITICAL_SECTION sys_xCriticalSection;

void Sys_InitializeCriticalSection()
{
	InitializeCriticalSection(&sys_xCriticalSection);
}

void Sys_EnterCriticalSection()
{
	EnterCriticalSection(&sys_xCriticalSection);
}

void Sys_LeaveCriticalSection()
{
	LeaveCriticalSection(&sys_xCriticalSection);
}


uint32_t Sys_Milliseconds()
{
	return GetTickCount();

}

/*
==========
FS_ShiftStr
perform simple string shifting to avoid scanning from the exe
==========
*/
void FS_ShiftStr( const char *string, int shift, char *buf )
{
	int i,l;

	l = strlen( string );
	for ( i = 0; i < l; i++ ) {
		buf[i] = string[i] + shift;
	}
	buf[i] = '\0';
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

/*
====================
FS_ReplaceSeparatorsUni

Fix things up differently for win/unix/mac
====================
*/
void FS_ReplaceSeparatorsUni( wchar_t *path ) {
	wchar_t	*s;

	for ( s = path ; *s ; s++ ) {
		if ( *s == L'/' || *s == L'\\' ) {
			*s = PATH_SEPUNI;
		}
	}
}

/*
====================
FS_StripTrailingSeperator

Fix things up differently for win/unix/mac
====================
*/
void FS_StripTrailingSeperator( char *path ) {

	int i = 1;
	int len = strlen(path);

	while(len > i && path[len -i] == PATH_SEP)
	{
		path[len -i] = '\0';
		++i;
	}

}

/*
====================
FS_StripTrailingSeperatorUni

Fix things up differently for win/unix/mac
====================
*/
static void FS_StripTrailingSeperatorUni( wchar_t *path ) {

	int i = 1;
	int len = wcslen(path);

	while(len > i && path[len -i] == PATH_SEPUNI)
	{
		path[len -i] = L'\0';
		++i;
	}
}


void FS_BuildOSPathForThread(const char *base, const char *game, const char *qpath, char *fs_path, int fs_thread)
{
  char basename[MAX_OSPATH];
  char gamename[MAX_OSPATH];

  int len;

  if ( !game || !*game )
    game = "";

  Q_strncpyz(basename, base, sizeof(basename));
  Q_strncpyz(gamename, game, sizeof(gamename));

  len = strlen(basename);
  if(len > 0 && (basename[len -1] == '/' || basename[len -1] == '\\'))
  {
        basename[len -1] = '\0';
  }

  len = strlen(gamename);
  if(len > 0 && (gamename[len -1] == '/' || gamename[len -1] == '\\'))
  {
        gamename[len -1] = '\0';
  }
  if ( Com_sprintf(fs_path, MAX_OSPATH, "%s/%s/%s", basename, gamename, qpath) >= MAX_OSPATH )
  {
    if ( fs_thread )
    {
        fs_path[0] = 0;
        return;
    }
    preInitError("FS_BuildOSPath: os path length exceeded");
  }
  FS_ReplaceSeparators(fs_path);
  FS_StripTrailingSeperator(fs_path);

}

void FS_BuildOSPathForThreadUni(const wchar_t *base, const char *game, const char *qpath, wchar_t *fs_path, int fs_thread)
{
  wchar_t basename[MAX_OSPATH];
  wchar_t gamename[MAX_OSPATH];
  wchar_t qpathname[MAX_QPATH];
  int len;

  if ( !game || !*game )
    game = "";

  Q_strncpyzUni(basename, base, sizeof(basename));
  Q_StrToWStr(gamename, game, sizeof(gamename));
  Q_StrToWStr(qpathname, qpath, sizeof(qpathname));

  len = wcslen(basename);
  if(len > 0 && (basename[len -1] == L'/' || basename[len -1] == L'\\'))
  {
        basename[len -1] = L'\0';
  }

  len = wcslen(gamename);
  if(len > 0 && (gamename[len -1] == L'/' || gamename[len -1] == L'\\'))
  {
        gamename[len -1] = L'\0';
  }

  if( Com_sprintfUni(fs_path, sizeof(wchar_t) * MAX_OSPATH, L"%s/%s/%s", basename, gamename, qpathname) >= MAX_OSPATH )
  {
    if ( fs_thread )
    {
        fs_path[0] = 0;
        return;
    }
    preInitError("FS_BuildOSPath: os path length exceeded");
  }
  FS_ReplaceSeparatorsUni(fs_path);
  FS_StripTrailingSeperatorUni(fs_path);
}


/*
================
FS_filelengthOSPath

If this is called on a non-unique FILE (from a pak file),
it will return the size of the pak file, not the expected
size of the file.
================
*/
int FS_filelengthOSPath( FILE* h ) {
	int		pos;
	int		end;

	pos = ftell (h);
	fseek (h, 0, SEEK_END);
	end = ftell (h);
	fseek (h, pos, SEEK_SET);

	return end;
}

/*
===========
FS_FOpenFileReadOSPathUni
search for a file somewhere below the home path, base path or cd path
we search in that order, matching FS_SV_FOpenFileRead order
===========
*/
int FS_FOpenFileReadOSPathUni( const wchar_t *filename, FILE **fp ) {
	wchar_t ospath[MAX_OSPATH];
	FILE* fh;

	Q_strncpyzUni( ospath, filename, sizeof( ospath ) );

	fh = _wfopen( ospath, L"rb" );

	if ( !fh ){
		*fp = NULL;
		return -1;
	}

	*fp = fh;

	return FS_filelengthOSPath(fh);
}

/*
==============
FS_FCloseFileOSPath

==============
*/
qboolean FS_FCloseFileOSPath( FILE* f ) {

	if (f) {
	    fclose (f);
	    return qtrue;
	}
	return qfalse;
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
	byte	*buf;

	if ( !f ) {
		return 0;
	}

	buf = (byte *)buffer;

	remaining = len;
	while (remaining) {
		block = remaining;
		read = fread (buf, 1, block, f);
		if (read == 0)
		{
			return len-remaining;	//Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
		}

		if (read == -1) {
			preInitError("FS_ReadOSPath: -1 bytes read");
		}

		remaining -= read;
		buf += read;
	}
	return len;

}

/*
============
FS_ReadFileOSPathUni

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
int FS_ReadFileOSPathUni( const wchar_t *ospath, void **buffer ) {
	byte*	buf;
	int		len;
	FILE*   h;


	if ( !ospath || !ospath[0] ) {
		preInitError( "FS_ReadFileOSPathUni with empty name\n" );
	}

	buf = NULL;	// quiet compiler warning

	// look for it in the filesystem or pack files
	len = FS_FOpenFileReadOSPathUni( ospath, &h );
	if ( len == -1 ) {
		if ( buffer ) {
			*buffer = NULL;
		}
		return -1;
	}

	if ( !buffer ) {
		FS_FCloseFileOSPath( h );
		return len;
	}

	buf = malloc(len+1);
	if(buf == NULL)
	{
		preInitError( "FS_ReadFileOSPathUni got no memory\n" );
	}
	*buffer = buf;

	FS_ReadOSPath (buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFileOSPath( h );
	return len;
}

void FS_FreeFile(void* buf)
{
    free(buf);
}


//================================================================

static char exeFilename[ MAX_STRING_CHARS ] = { 0 };
static char dllFilename[ MAX_STRING_CHARS ] = { 0 };
/*
=================
Sys_SetExeFile
=================
*/
void Sys_SetExeFile(const char *filepath)
{
	Q_strncpyz(exeFilename, filepath, sizeof(exeFilename));
}

/*
=================
Sys_SetDllFile
=================
*/
void Sys_SetDllFile(const char *filepath)
{
	Q_strncpyz(dllFilename, filepath, sizeof(dllFilename));
}


/*
=================
Sys_ExeFile
=================
*/
const char* Sys_ExeFile( void )
{
	return exeFilename;
}

/*
=================
Sys_DllFile
=================
*/
const char* Sys_DllFile( void )
{
	return dllFilename;
}

char sys_restartCmdLine[4096];

void Sys_SetRestartParams(const char* params)
{
	Q_strncpyz(sys_restartCmdLine, params, sizeof(sys_restartCmdLine));
}

qboolean AutoupdateRequiresElevatedPermissions = qfalse;

void Sys_RestartProcessOnExit()
{
	void* HWND = NULL;
	char displayMessageBuf[1024];
	const char *exefile;
	const char* method;
    SHELLEXECUTEINFO sei;
	int pid;

	if ( sys_restartCmdLine[0] == '\0' )
	{
		/* No restart (normal exit) */
		return;
	}


	exefile = Sys_ExeFile();
	if(strlen(exefile) < 9)
	{
		/* Can not restart (normal exit) */
		return;
	}

	if(AutoupdateRequiresElevatedPermissions)
	{
		method = "runas";
		MessageBoxA(HWND, "Note: CoD4X launcher needs to update file iw3mp.exe and will require extended permissions" , "Call of Duty 4 - Launcher", MB_OK | MB_ICONEXCLAMATION);
	}else{
		method = "open";

	}

	memset(&sei, 0, sizeof(sei));
	sei.cbSize = sizeof(sei);
    sei.lpVerb = method;
    sei.lpFile = exefile;
	sei.lpParameters = sys_restartCmdLine;
    sei.hwnd = HWND;
    sei.nShow = SW_NORMAL;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;

	if (!ShellExecuteExA(&sei))
	{
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						0, GetLastError(), 0x400, displayMessageBuf, sizeof(displayMessageBuf), 0);

		MessageBoxA(HWND, va("ShellExec of commandline: %s %s has failed.\nError: %s\n" , exefile, sys_restartCmdLine, displayMessageBuf), "Call of Duty - Error", MB_OK | MB_ICONERROR);
		return;
	}
	if(sei.hProcess == NULL)
	{
		return;
	}
	pid = GetProcessId(sei.hProcess);
    AllowSetForegroundWindow(pid);
}

#ifndef KF_FLAG_CREATE
	#define KF_FLAG_CREATE 0x00008000
#endif



const GUID FOLDERID_LocalAppData = {0xF1B32785, 0x6FBA, 0x4FCF, {0x9D, 0x55, 0x7B, 0x8E, 0x7F, 0x15, 0x70, 0x91}};


HMODULE LoadSystemLibraryA(char* libraryname)
{
	char system32dir[1024];
	char modulepath[1024];
	int len;

	len = GetSystemDirectoryA(system32dir, sizeof(system32dir));

	if (len < 1 || len >= sizeof(system32dir))
	{
		return LoadLibraryA(libraryname);
	}

	Com_sprintf(modulepath, sizeof(modulepath), "%s\\%s", system32dir, libraryname);
	return LoadLibraryA(libraryname);

}

char sys_cwd[1024];

void Sys_Cwd(char* path, int len)
{
	Q_strncpyz(path, sys_cwd, len);
}

void Sys_SetupCwd(  )
{
	char* cut;

	Q_strncpyz(sys_cwd, Sys_ExeFile(), sizeof(sys_cwd));
	cut = strrchr(sys_cwd, PATH_SEP);
	if(cut != NULL)
	{
		*cut = '\0';
		SetCurrentDirectory(sys_cwd);
	}
}

wchar_t* Sys_GetAppDataDir(wchar_t *basepath, int size)
{



	HMODULE hModuleShl = LoadSystemLibraryA("shell32.dll");
	HMODULE hModuleOle = LoadSystemLibraryA("ole32.dll");
	HRESULT (WINAPI *farproc)(const GUID *rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
	HRESULT (__cdecl *farprocxp)(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath);
	void (__cdecl *CoTaskMemFree_t)( void* mem );
	wchar_t xpPath[MAX_OSPATH];
	LPWSTR wszPath = NULL;


	/* Get the AppData folder */
	basepath[0] = L'\0';

	if(hModuleShl)
	{
		farproc = (void*)GetProcAddress(hModuleShl, "SHGetKnownFolderPath");
		farprocxp = (void*)GetProcAddress(hModuleShl, "SHGetFolderPathW");
		if(farproc)
		{

			HRESULT hr = farproc ( &FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &wszPath );
			if ( SUCCEEDED(hr) )
			{
				Q_strncpyzUni(basepath, wszPath, size);
				Q_strcatUni(basepath, size, L"\\CallofDuty4MW");
			}
			if(hModuleOle)
			{
				CoTaskMemFree_t = (void*)GetProcAddress(hModuleShl, "CoTaskMemFree");
				if(CoTaskMemFree_t)
				{
					CoTaskMemFree_t( wszPath );
				}
			}


		}else if(farprocxp){

			HRESULT hr = farprocxp(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, (LPTSTR)xpPath);
			if ( SUCCEEDED(hr) )
			{
				Q_strncpyzUni(basepath, xpPath, size);
				Q_strcatUni(basepath, size, L"\\CallofDuty4MW");
			}

		}else{
			FreeLibrary(hModuleShl);
			return NULL;
		}
		FreeLibrary(hModuleShl);

	}else{
		return NULL;
	}

	if(hModuleOle)
	{
		FreeLibrary(hModuleOle);
	}

	return basepath;
}


wchar_t fs_savepathDir[MAX_OSPATH];

void FS_SetupSavePath()
{
	char homepath[MAX_OSPATH];


	if(Sys_GetAppDataDir(fs_savepathDir, sizeof(fs_savepathDir)) == NULL)
	{
		Sys_Cwd(homepath, sizeof(homepath));
		Q_StrToWStr(fs_savepathDir, homepath, sizeof(fs_savepathDir));
	}
}

wchar_t* FS_GetSavePath()
{
	return fs_savepathDir;
}



DWORD *threadHandles = (DWORD *)0x14E89A8; //array of 12

void Sys_GetLastErrorAsString(char* errbuf, int len)
{
	int lastError = GetLastError();
	if(lastError != 0)
	{
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR)errbuf, len -1, NULL);
	}else{
		Q_strncpyz(errbuf, "Unknown Error", len);
	}
}



void preInitError(const char* error){
	MessageBoxA( 0, error, "Call of Duty 4 - Modern Warfare - Fatal Error", 0x10 );
	ExitProcess(1);
}





char* mss32importnames[] = {
	"AIL_debug_printf",
	"AIL_sprintf",
	"DLSClose",
	"DLSCompactMemory",
	"DLSGetInfo",
	"DLSLoadFile",
	"DLSLoadMemFile",
	"DLSMSSOpen",
	"DLSSetAttribute",
	"DLSUnloadAll",
	"DLSUnloadFile",
	"RIB_alloc_provider_handle",
	"RIB_enumerate_interface",
	"RIB_error",
	"RIB_find_file_provider",
	"RIB_free_provider_handle",
	"RIB_free_provider_library",
	"RIB_load_provider_library",
	"RIB_register_interface",
	"RIB_request_interface",
	"RIB_request_interface_entry",
	"RIB_type_string",
	"RIB_unregister_interface",
	"_AIL_3D_distance_factor@4",
	"_AIL_3D_doppler_factor@4",
	"_AIL_3D_rolloff_factor@4",
	"_AIL_DLS_close@8",
	"_AIL_DLS_compact@4",
	"_AIL_DLS_get_info@12",
	"_AIL_DLS_load_file@12",
	"_AIL_DLS_load_memory@12",
	"_AIL_DLS_open@28",
	"_AIL_DLS_sample_handle@4",
	"_AIL_DLS_unload@8",
	"_AIL_HWND@0",
	"_AIL_MIDI_handle_reacquire@4",
	"_AIL_MIDI_handle_release@4",
	"_AIL_MIDI_to_XMI@20",
	"_AIL_MMX_available@0",
	"_AIL_WAV_file_write@20",
	"_AIL_WAV_info@8",
	"_AIL_XMIDI_master_volume@4",
	"_AIL_active_sample_count@4",
	"_AIL_active_sequence_count@4",
	"_AIL_allocate_sample_handle@4",
	"_AIL_allocate_sequence_handle@4",
	"_AIL_auto_service_stream@8",
	"_AIL_background@0",
	"_AIL_background_CPU_percent@0",
	"_AIL_branch_index@8",
	"_AIL_calculate_3D_channel_levels@56",
	"_AIL_channel_notes@8",
	"_AIL_close_XMIDI_driver@4",
	"_AIL_close_digital_driver@4",
	"_AIL_close_filter@4",
	"_AIL_close_input@4",
	"_AIL_close_stream@4",
	"_AIL_compress_ADPCM@12",
	"_AIL_compress_ASI@20",
	"_AIL_compress_DLS@20",
	"_AIL_controller_value@12",
	"_AIL_create_wave_synthesizer@16",
	"_AIL_decompress_ADPCM@12",
	"_AIL_decompress_ASI@24",
	"_AIL_delay@4",
	"_AIL_destroy_wave_synthesizer@4",
	"_AIL_digital_CPU_percent@4",
	"_AIL_digital_configuration@16",
	"_AIL_digital_driver_processor@8",
	"_AIL_digital_handle_reacquire@4",
	"_AIL_digital_handle_release@4",
	"_AIL_digital_latency@4",
	"_AIL_digital_master_reverb@16",
	"_AIL_digital_master_reverb_levels@12",
	"_AIL_digital_master_volume_level@4",
	"_AIL_digital_output_filter@4",
	"_AIL_end_sample@4",
	"_AIL_end_sequence@4",
	"_AIL_enumerate_MP3_frames@4",
	"_AIL_enumerate_filter_properties@12",
	"_AIL_enumerate_filter_sample_properties@12",
	"_AIL_enumerate_filters@12",
	"_AIL_enumerate_output_filter_driver_properties@12",
	"_AIL_enumerate_output_filter_sample_properties@12",
	"_AIL_enumerate_sample_stage_properties@16",
	"_AIL_extract_DLS@28",
	"_AIL_file_error@0",
	"_AIL_file_read@8",
	"_AIL_file_size@4",
	"_AIL_file_type@8",
	"_AIL_file_type_named@12",
	"_AIL_file_write@12",
	"_AIL_filter_DLS_with_XMI@24",
	"_AIL_filter_property@20",
	"_AIL_find_DLS@24",
	"_AIL_find_filter@8",
	"_AIL_ftoa@4",
	"_AIL_get_DirectSound_info@12",
	"_AIL_get_input_info@4",
	"_AIL_get_preference@4",
	"_AIL_get_timer_highest_delay@0",
	"_AIL_init_sample@12",
	"_AIL_init_sequence@12",
	"_AIL_inspect_MP3@12",
	"_AIL_last_error@0",
	"_AIL_list_DLS@20",
	"_AIL_list_MIDI@20",
	"_AIL_listener_3D_orientation@28",
	"_AIL_listener_3D_position@16",
	"_AIL_listener_3D_velocity@16",
	"_AIL_listener_relative_receiver_array@8",
	"_AIL_load_sample_attributes@8",
	"_AIL_load_sample_buffer@16",
	"_AIL_lock@0",
	"_AIL_lock_channel@4",
	"_AIL_lock_mutex@0",
	"_AIL_map_sequence_channel@12",
	"_AIL_mem_alloc_lock@4",
	"_AIL_mem_free_lock@4",
	"_AIL_mem_use_free@4",
	"_AIL_mem_use_malloc@4",
	"_AIL_merge_DLS_with_XMI@16",
	"_AIL_midiOutClose@4",
	"_AIL_midiOutOpen@12",
	"_AIL_minimum_sample_buffer_size@12",
	"_AIL_ms_count@0",
	"_AIL_open_XMIDI_driver@4",
	"_AIL_open_digital_driver@16",
	"_AIL_open_filter@8",
	"_AIL_open_input@4",
	"_AIL_open_stream@12",
	"_AIL_output_filter_driver_property@20",
	"_AIL_pause_stream@8",
	"_AIL_platform_property@20",
	"_AIL_primary_digital_driver@4",
	"_AIL_process_digital_audio@24",
	"_AIL_quick_copy@4",
	"_AIL_quick_halt@4",
	"_AIL_quick_handles@12",
	"_AIL_quick_load@4",
	"_AIL_quick_load_and_play@12",
	"_AIL_quick_load_mem@8",
	"_AIL_quick_load_named_mem@12",
	"_AIL_quick_ms_length@4",
	"_AIL_quick_ms_position@4",
	"_AIL_quick_play@8",
	"_AIL_quick_set_low_pass_cut_off@8",
	"_AIL_quick_set_ms_position@8",
	"_AIL_quick_set_reverb_levels@12",
	"_AIL_quick_set_speed@8",
	"_AIL_quick_set_volume@12",
	"_AIL_quick_shutdown@0",
	"_AIL_quick_startup@20",
	"_AIL_quick_status@4",
	"_AIL_quick_type@4",
	"_AIL_quick_unload@4",
	"_AIL_redbook_close@4",
	"_AIL_redbook_eject@4",
	"_AIL_redbook_id@4",
	"_AIL_redbook_open@4",
	"_AIL_redbook_open_drive@4",
	"_AIL_redbook_pause@4",
	"_AIL_redbook_play@12",
	"_AIL_redbook_position@4",
	"_AIL_redbook_resume@4",
	"_AIL_redbook_retract@4",
	"_AIL_redbook_set_volume_level@8",
	"_AIL_redbook_status@4",
	"_AIL_redbook_stop@4",
	"_AIL_redbook_track@4",
	"_AIL_redbook_track_info@16",
	"_AIL_redbook_tracks@4",
	"_AIL_redbook_volume_level@4",
	"_AIL_register_EOB_callback@8",
	"_AIL_register_EOS_callback@8",
	"_AIL_register_ICA_array@8",
	"_AIL_register_SOB_callback@8",
	"_AIL_register_beat_callback@8",
	"_AIL_register_event_callback@8",
	"_AIL_register_falloff_function_callback@8",
	"_AIL_register_prefix_callback@8",
	"_AIL_register_sequence_callback@8",
	"_AIL_register_stream_callback@8",
	"_AIL_register_timbre_callback@8",
	"_AIL_register_timer@4",
	"_AIL_register_trace_callback@8",
	"_AIL_register_trigger_callback@8",
	"_AIL_release_all_timers@0",
	"_AIL_release_channel@8",
	"_AIL_release_sample_handle@4",
	"_AIL_release_sequence_handle@4",
	"_AIL_release_timer_handle@4",
	"_AIL_request_EOB_ASI_reset@12",
	"_AIL_resume_sample@4",
	"_AIL_resume_sequence@4",
	"_AIL_room_type@4",
	"_AIL_sample_3D_cone@16",
	"_AIL_sample_3D_distances@16",
	"_AIL_sample_3D_orientation@28",
	"_AIL_sample_3D_position@16",
	"_AIL_sample_3D_velocity@16",
	"_AIL_sample_51_volume_levels@28",
	"_AIL_sample_51_volume_pan@24",
	"_AIL_sample_buffer_info@20",
	"_AIL_sample_buffer_ready@4",
	"_AIL_sample_channel_levels@8",
	"_AIL_sample_exclusion@4",
	"_AIL_sample_granularity@4",
	"_AIL_sample_loop_block@12",
	"_AIL_sample_loop_count@4",
	"_AIL_sample_low_pass_cut_off@4",
	"_AIL_sample_ms_position@12",
	"_AIL_sample_obstruction@4",
	"_AIL_sample_occlusion@4",
	"_AIL_sample_playback_rate@4",
	"_AIL_sample_position@4",
	"_AIL_sample_processor@8",
	"_AIL_sample_reverb_levels@12",
	"_AIL_sample_stage_property@24",
	"_AIL_sample_status@4",
	"_AIL_sample_user_data@8",
	"_AIL_sample_volume_levels@12",
	"_AIL_sample_volume_pan@12",
	"_AIL_save_sample_attributes@8",
	"_AIL_send_channel_voice_message@20",
	"_AIL_send_sysex_message@8",
	"_AIL_sequence_loop_count@4",
	"_AIL_sequence_ms_position@12",
	"_AIL_sequence_position@12",
	"_AIL_sequence_status@4",
	"_AIL_sequence_tempo@4",
	"_AIL_sequence_user_data@8",
	"_AIL_sequence_volume@4",
	"_AIL_serve@0",
	"_AIL_service_stream@8",
	"_AIL_set_3D_distance_factor@8",
	"_AIL_set_3D_doppler_factor@8",
	"_AIL_set_3D_rolloff_factor@8",
	"_AIL_set_DirectSound_HWND@8",
	"_AIL_set_XMIDI_master_volume@8",
	"_AIL_set_digital_driver_processor@12",
	"_AIL_set_digital_master_reverb@16",
	"_AIL_set_digital_master_reverb_levels@12",
	"_AIL_set_digital_master_volume_level@8",
	"_AIL_set_error@4",
	"_AIL_set_file_async_callbacks@20",
	"_AIL_set_file_callbacks@16",
	"_AIL_set_input_state@8",
	"_AIL_set_listener_3D_orientation@28",
	"_AIL_set_listener_3D_position@16",
	"_AIL_set_listener_3D_velocity@20",
	"_AIL_set_listener_3D_velocity_vector@16",
	"_AIL_set_listener_relative_receiver_array@12",
	"_AIL_set_named_sample_file@20",
	"_AIL_set_preference@8",
	"_AIL_set_redist_directory@4",
	"_AIL_set_room_type@8",
	"_AIL_set_sample_3D_cone@16",
	"_AIL_set_sample_3D_distances@16",
	"_AIL_set_sample_3D_orientation@28",
	"_AIL_set_sample_3D_position@16",
	"_AIL_set_sample_3D_velocity@20",
	"_AIL_set_sample_3D_velocity_vector@16",
	"_AIL_set_sample_51_volume_levels@28",
	"_AIL_set_sample_51_volume_pan@24",
	"_AIL_set_sample_address@12",
	"_AIL_set_sample_adpcm_block_size@8",
	"_AIL_set_sample_channel_levels@12",
	"_AIL_set_sample_exclusion@8",
	"_AIL_set_sample_file@12",
	"_AIL_set_sample_info@8",
	"_AIL_set_sample_loop_block@12",
	"_AIL_set_sample_loop_count@8",
	"_AIL_set_sample_low_pass_cut_off@8",
	"_AIL_set_sample_ms_position@8",
	"_AIL_set_sample_obstruction@8",
	"_AIL_set_sample_occlusion@8",
	"_AIL_set_sample_playback_rate@8",
	"_AIL_set_sample_position@8",
	"_AIL_set_sample_processor@12",
	"_AIL_set_sample_reverb_levels@12",
	"_AIL_set_sample_user_data@12",
	"_AIL_set_sample_volume_levels@12",
	"_AIL_set_sample_volume_pan@12",
	"_AIL_set_sequence_loop_count@8",
	"_AIL_set_sequence_ms_position@8",
	"_AIL_set_sequence_tempo@12",
	"_AIL_set_sequence_user_data@12",
	"_AIL_set_sequence_volume@12",
	"_AIL_set_speaker_configuration@16",
	"_AIL_set_speaker_reverb_levels@16",
	"_AIL_set_stream_loop_block@12",
	"_AIL_set_stream_loop_count@8",
	"_AIL_set_stream_ms_position@8",
	"_AIL_set_stream_position@8",
	"_AIL_set_stream_user_data@12",
	"_AIL_set_timer_divisor@8",
	"_AIL_set_timer_frequency@8",
	"_AIL_set_timer_period@8",
	"_AIL_set_timer_user@8",
	"_AIL_shutdown@0",
	"_AIL_size_processed_digital_audio@16",
	"_AIL_speaker_configuration@20",
	"_AIL_speaker_reverb_levels@16",
	"_AIL_start_all_timers@0",
	"_AIL_start_sample@4",
	"_AIL_start_sequence@4",
	"_AIL_start_stream@4",
	"_AIL_start_timer@4",
	"_AIL_startup@0",
	"_AIL_stop_all_timers@0",
	"_AIL_stop_sample@4",
	"_AIL_stop_sequence@4",
	"_AIL_stop_timer@4",
	"_AIL_stream_info@20",
	"_AIL_stream_loop_count@4",
	"_AIL_stream_ms_position@12",
	"_AIL_stream_position@4",
	"_AIL_stream_sample_handle@4",
	"_AIL_stream_status@4",
	"_AIL_stream_user_data@8",
	"_AIL_true_sequence_channel@8",
	"_AIL_unlock@0",
	"_AIL_unlock_mutex@0",
	"_AIL_update_listener_3D_position@8",
	"_AIL_update_sample_3D_position@8",
	"_AIL_us_count@0",
	"_DLSMSSGetCPU@4",
	"_MIX_RIB_MAIN@8",
	"_MSSDisableThreadLibraryCalls@4",
	"_RIB_enumerate_providers@12",
	"_RIB_find_file_dec_provider@20",
	"_RIB_find_files_provider@20",
	"_RIB_find_provider@12",
	"_RIB_load_application_providers@4",
	"_RIB_load_static_provider_library@8",
	"_RIB_provider_system_data@8",
	"_RIB_provider_user_data@8",
	"_RIB_set_provider_system_data@12",
	"_RIB_set_provider_user_data@12"
};

void* mss32importprocs[AIL_TOP_COUNT];
static qboolean sys_tempinstall;

qboolean Sys_LoadModules(HINSTANCE hInstance){

	HINSTANCE base;
	FARPROC proc;
	int i, copylen;
	char moduledir[1024];
	char mss32path[1024];
	char miles32path[1024];
	static qboolean loaded = qfalse;
	char* find;

	if(loaded == qtrue)
		return qtrue;

	copylen = GetModuleFileNameA(hInstance, moduledir, sizeof(moduledir));

	if (copylen < 1 || strrchr(moduledir, '\\') == NULL)
	{
		Q_strncpyz(miles32path, "miles32.dll", sizeof(miles32path));
		Q_strncpyz(mss32path, "mss32.dll", sizeof(mss32path));
	} else {
		find = strrchr(moduledir, '\\');
		*find = '\0';
		Com_sprintf(miles32path, sizeof(miles32path), "%s\\miles32.dll", moduledir);
		Com_sprintf(mss32path, sizeof(mss32path), "%s\\mss32.dll", moduledir);
	}


	if (sys_tempinstall)
		base = LoadLibraryA(mss32path);
	else
		base = LoadLibraryA(miles32path);

	if(!base){
		MessageBoxA(NULL, "Error loading module miles32.dll\nAttempting to fix...", "mss32.dll not found", MB_OK);
		return qfalse;
	}

	for(i = 0; i < AIL_TOP_COUNT; i++)
	{
		proc = GetProcAddress(base, mss32importnames[i]);
		if(!proc)
		{
			preInitError(va("No entry point for procedure: %s\n", mss32importnames[i]));
			return qfalse;
		}else
			mss32importprocs[i] = proc;
	}
	return qtrue;
}


/* Needs 256 wchar_t characters */
/*
const wchar_t* Sys_DllPath(wchar_t* path)
{
	FS_BuildOSPathForThreadUni(FS_GetSavePath(), "bin", "", path, 0);
	return path;
}

void Sys_SetupCrashReporter()
{
	CR_INSTALL_INFOA info;
	wchar_t crashrptdllpath[1024];
	wchar_t dllpath[1024];
	wchar_t displayMessageBuf[4096];
	wchar_t errorMessageBuf[4096];
	int WINAPI (*crInstallAImp)(PCR_INSTALL_INFOA pInfo);
	int WINAPI (*crGetLastErrorMsgAImp)(LPSTR pszBuffer, UINT uBuffSize);

	Sys_DllPath(dllpath);
	Com_sprintfUni(crashrptdllpath, sizeof(crashrptdllpath), L"%s\\crashrpt1403.dll", dllpath);
	HMODULE hCrashRpt = LoadLibraryExW(crashrptdllpath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if(hCrashRpt)
	{
		crInstallAImp = GetProcAddress(hCrashRpt, "crInstallA");
		crGetLastErrorMsgAImp = GetProcAddress(hCrashRpt, "crGetLastErrorMsgA");
	}else{
		crInstallAImp = NULL;
		crGetLastErrorMsgAImp = NULL;
	}
}
*/

void Sys_ExitCoD4SPToMpConnect(const char* commandLine)
{
	char lpFilename[MAX_STRING_CHARS];
	char lpCommandLine[8192];
	STARTUPINFOA siStartupInfo;
    PROCESS_INFORMATION piProcessInfo;

	Q_strncpyz(lpCommandLine, commandLine, sizeof(lpCommandLine));

	HMODULE hModule = GetModuleHandleA(NULL);
	char* find;

	if(hModule == NULL)
	{
		return;
	}

	if(GetModuleFileNameA( hModule, lpFilename, sizeof(lpFilename)) == 0)
	{
		return;
	}

	find = strstr(lpFilename, "iw3sp.exe");

	if(find == NULL)
	{
		return;
	}
	strcpy(find, "iw3mp.exe");

	find = strstr(lpCommandLine, "iw3sp.exe");

	if(find == NULL)
	{
		return;
	}
	memcpy(find, "iw3mp.exe", 9);

    memset(&siStartupInfo, 0, sizeof(siStartupInfo));
    memset(&piProcessInfo, 0, sizeof(piProcessInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);

	if(CreateProcessA(lpFilename, lpCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &siStartupInfo, &piProcessInfo) == 0)
	{
		return;
	}
	ExitProcess(0);

//    doexit(0, 0, 0);
}

void __iw3mp_security_init_cookie();
int __iw3mp_tmainCRTStartup();
/* We have a too small stacksize on mainthread.
This trick will replace the mainthread with a new mainthread which has an large enough stack */

int Sys_SortVersionDirs(const void* cmp1, const void* cmp2)
{
	const char* stri1 = *(const char**)cmp1;
	const char* stri2 = *(const char**)cmp2;

	return Q_stricmp(stri1, stri2);
}

HMODULE Sys_LoadCoD4XModule(const char* version)
{
	int i, j;
	wchar_t dir[1024];
	wchar_t dllfile[1024];
	wchar_t dlldir[1024];
	char* list[1024];
	char* filteredList[1024];

	FS_SetupSavePath();
	FS_BuildOSPathForThreadUni( FS_GetSavePath(), "bin", "", dir, 0);

	int count = Sys_ListDirectories(dir, list, sizeof(list) / sizeof(list[0]));

	for(i = 0, j = 0; i < count; ++i)
	{
		if(Q_stricmpn(list[i], "cod4x_", 6) == 0)
		{
			filteredList[j] = list[i];
			++j;
		}
	}
	filteredList[j] = NULL;

	qsort(filteredList, j, sizeof(filteredList[0]), Sys_SortVersionDirs);

	if(j < 1)
	{
		Sys_FreeFileList(list);
		return NULL;
	}

	char filename[1024];

	if(version && version[0])
	{
		for(i = 0; i < j; ++i)
		{
			if(!Q_stricmp(filteredList[i] + 6, version))
			{
				Com_sprintf(filename, sizeof(filename), "%s.dll", filteredList[i]);
				FS_BuildOSPathForThreadUni( dir, filteredList[i], filename, dllfile, 0);
				FS_BuildOSPathForThreadUni( dir, filteredList[i], "", dlldir, 0);
				break;
			}
		}
		if(i == j)
		{
			return NULL;
		}
	}else{
		Com_sprintf(filename, sizeof(filename), "%s.dll", filteredList[j -1]);
		FS_BuildOSPathForThreadUni( dir, filteredList[j -1], filename, dllfile, 0);
		FS_BuildOSPathForThreadUni( dir, filteredList[j -1], "", dlldir, 0);
	}

	Sys_FreeFileList(list);
	SetDllDirectoryW( dlldir );
	HMODULE hModule = LoadLibraryW( dllfile );
	if(hModule == NULL)
	{
		char errormsg[128];
		sprintf(errormsg, "Error code %d\n", GetLastError());
		MessageBoxA(NULL, errormsg, "LoadLibrary failed", MB_OK);
	}
	return hModule;
}


int entrypoint()
{
	static qboolean mainThreadCreated;

	HINSTANCE hInstance = (HINSTANCE)0x400000;
	HINSTANCE hPrevInstance = (HINSTANCE)0x0;
	char cmdLine[1024], parsecmdLine[1024];
	char v[16];

	if(!mainThreadCreated)
	{
		mainThreadCreated = qtrue;

		if (CreateThread(NULL, //Choose default security
							1024*1024*12, //stack size
							(LPTHREAD_START_ROUTINE)&entrypoint, //Routine to execute
							NULL, //Thread parameter
							0, //Immediately run the thread
							NULL //Thread Id
		) == NULL)
		{
			return -1;
		}
		return 0;
	}

	char lpFilename[MAX_STRING_CHARS];
	int copylen;

	copylen = GetModuleFileNameA(hInstance, lpFilename, sizeof(lpFilename));
	if(copylen >= (sizeof(lpFilename) -1))
	{
		Sys_SetExeFile( "" );
	}else{
		Sys_SetExeFile( lpFilename );
	}

	Sys_SetupCwd( );

	Q_strncpyz(cmdLine, GetCommandLineA(), sizeof(cmdLine));
	Q_strncpyz(parsecmdLine, cmdLine, sizeof(parsecmdLine));


	Com_ParseCommandLine(parsecmdLine);

	Com_StartupVariableValue( "installupdatefiles", v, sizeof(v) );
	if(v[0])
	{
		Sec_Init();
		Sys_RunUpdater();
		exit( 0);
	}


	Com_StartupVariableValue( "legacymode", v, sizeof(v) );
	if(!atoi(v))
	{
		int CALLBACK (*lpWinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) = NULL;

		HMODULE hModule = Sys_LoadCoD4XModule(Com_StartupVariableValue("protocolversion", v ,sizeof(v)));

		if(hModule)
		{
			lpWinMain = GetProcAddress(hModule, "WinMain@16");
		}
		int r = -1;
		if(lpWinMain)
		{
			r = lpWinMain(hInstance, hPrevInstance, cmdLine, 0);
			exit(r);
		}else{
			MessageBoxA(NULL, "Failed to load CoD4X because file cod4x_xxx.dll or entryoint was not found. Attempting to load CoD4 v1.7", "Error loading CoD4X", MB_OK);
		}
	}
	/* Original CoD4 function getting called without any other patching but exploit patching */
	Patch_MainModule(Patch_Exploits, NULL);

	__iw3mp_security_init_cookie();
	return __iw3mp_tmainCRTStartup();
}


qboolean Patch_isiw3mp()
{
	char filename[1024];

	if(GetModuleFileNameA(NULL, filename, sizeof(filename)) < 6)
	{
		return qfalse;
	}

	char* pos = strrchr(filename, '\\');
	if(!pos)
	{
		return qfalse;
	}

	*pos = 0;
	++pos;

	DWORD cmp1 = *((DWORD*)(0x6748ce));
	DWORD cmp2 = *((DWORD*)(0x6748d2));


	if(cmp1 == 0x48c28bff && cmp2 == 0x74481774)
		return qfalse; //offical 1.0

	if(cmp1 == 0xe80875ff && cmp2 == 0xfffffeef)
		return qfalse; //offical 1.1

	if(cmp1 == 0x89c53300 && cmp2 == 0x458bfc45)
		return qfalse; //offical 1.2

	if(cmp1 == 0x8c0f01fe && cmp2 == 0xffffff79)
		return qfalse; //offical 1.3

	if(cmp1 == 0x6a000072 && cmp2 == 0x1075ff01)
		return qfalse; //offical 1.4

	if(cmp1 == 0xc08510c4 && cmp2 == 0x7d8b0874)
		return qfalse; //offical 1.5

	if(cmp1 == 0xebe4458b && cmp2 == 0x40c03313)
		return qfalse; //offical 1.6

	if(cmp1 == 0xebe4458b && cmp2 == 0x40c03313)
		return qfalse; //offical 1.6

	if(cmp1 == 0xe9ffc883 && cmp2 == 0x552)
		return qtrue; //offical 1.8  steam

	if(cmp1 == 0xf02c7de8 && cmp2 == 0xe44589ff)
		return qtrue; //offical 1.7

	if(Q_stricmpn(pos, "iw3mp", 5) == 0)
	{
		return qtrue;
	}


	return qfalse;
}

qboolean Patch_isiw3sp()
{
	DWORD cmp1 = *((DWORD*)(0x410000));
	DWORD cmp2 = *((DWORD*)(0x410004));
	if(cmp1 == 0xbd2e1853 && cmp2 == 0x9972c157)
	{
		return qtrue;
	}

	return qfalse;
}




bool IsMainModulePure()
{
	unsigned long textchecksum;
	unsigned long rdatachecksum;

	byte* module = (byte*)0x401000;
	byte* moduletextend = (byte*)0x690429;
	byte* rdata = (byte*)0x691520;
	byte* rdataend = (byte*)0x71b000;

	textchecksum = adler32(0, module, moduletextend - module);

	if(textchecksum != 0xda4368f5)
	{
		return false;
	}

	rdatachecksum = adler32(0, rdata, rdataend - rdata);

	if(rdatachecksum != 0xedbbc11)
	{
		return false;
	}

	return true;
}

void Sys_Quit()
{
  	Sys_RestartProcessOnExit( );
	ExitProcess(0);
}

void ImpureUpdateFixFail()
{
	MessageBoxA(NULL, "Impure file iw3mp.exe detected.\nFetching the pure version has failed.\nPut original iw3mp.exe of CoD4 version 1.7 into folder or try again.", "Impure iw3mp.exe", MB_OK);
	Sys_Quit();
}

void IW3MPUpdateFail()
{
	MessageBoxA(NULL, "Couldn't upgrade iw3mp.exe", "Unable to upgrade iw3mp.exe", MB_OK);
	Sys_Quit();
}


BOOL WINAPI __declspec(dllexport) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
	char lpFilename[MAX_STRING_CHARS];
	int copylen;
	const char* commandLine;

	if(dwReason == DLL_PROCESS_ATTACH && Patch_isiw3mp())
	{
		commandLine = GetCommandLineA();
		if(strstr(commandLine, "+steamdummy"))
		{
			Sleep(1000);
			ExitProcess(0);
		}
		copylen = GetModuleFileName(hInstance, lpFilename, sizeof(lpFilename));
		if(copylen >= (sizeof(lpFilename) -1))
		{
			Sys_SetDllFile( "" );
		}else{
			Sys_SetDllFile( lpFilename );
		}

		static qboolean initialized = qfalse;

		xoxorPresent = GetModuleHandleA("iw3x.dll") != NULL ? true : false;

		if(xoxorPresent)
		{
			if(Sys_LoadModules(hInstance) == qfalse)
			{
				preInitError("No miles32.dll found. Start iw3mp.exe to fix\n");
				return FALSE;
			}
			return TRUE;
		}


		if(initialized == qfalse)
		{
			if(IsMainModulePure() == false)
			{
				HookEntryPointForInstaller();
				return TRUE;
			}
			if(!LargeAddressspacePatchEnabled())
			{
				HookEntryPointForIW3Upd(); //Update it for 4 GByte address space
				return TRUE;
			}

			if(*(int*)0x690429 == 0)
			{
				/* Normal startup from permanent install */
				Patch_MainModule(Patch_WinMainEntryPoint, NULL);
			}else{
				/* Startup from temp install */
				sys_tempinstall = qtrue;
				Patch_MainModule(Patch_WinMainEntryPoint, NULL);
			}
			initialized = qtrue;
			if(Sys_LoadModules(hInstance) == qfalse){//mss32 imports
				HookEntryPointForMSSInstaller();
				return TRUE;
			}
		}

	}else if(dwReason == DLL_PROCESS_ATTACH){
		if(Patch_isiw3sp())
		{
			commandLine = GetCommandLineA();
			if(strstr(commandLine, " +connect"))
			{
				Sys_ExitCoD4SPToMpConnect(commandLine);
			}

		}
		if(Sys_LoadModules(hInstance) == qfalse)
		{
			preInitError("No miles32.dll found. Start iw3mp.exe to fix\n");
			return FALSE;
		}

	}
	return TRUE;
}


// "updates" shifted from -7
#define AUTOUPDATE_DIR "ni]Zm^l"
#define AUTOUPDATE_DIR_SHIFT 7


qboolean InternetGetContent(char* url, void** buf)
{
	static HMODULE hInternetModule;

	typedef HINTERNET ( WINAPI *TYPE_InternetOpen ) (LPCTSTR , DWORD ,LPCTSTR,LPCTSTR ,DWORD );
	typedef HINTERNET ( WINAPI *TYPE_InternetOpenUrl) ( HINTERNET, LPCTSTR , LPCTSTR , DWORD , DWORD , DWORD);
	typedef BOOL (WINAPI *TYPE_InternetCloseHandle) ( HINTERNET );
	typedef BOOL ( WINAPI * TYPE_InternetReadFile ) (HINTERNET ,LPVOID ,DWORD ,LPDWORD);
	typedef BOOL ( WINAPI * TYPE_InternetSetOption ) (HINTERNET ,DWORD dwOption ,LPVOID lpBuffer ,DWORD dwBufferLength);

	*buf = NULL;

	if(hInternetModule == NULL)
	{
		hInternetModule = LoadSystemLibraryA("wininet.dll");
	}
	if(hInternetModule == NULL)
	{
		return 0;
	}

	TYPE_InternetOpen _InternetOpen = ( TYPE_InternetOpen ) GetProcAddress( hInternetModule, "InternetOpenA" );
	TYPE_InternetOpenUrl _InternetOpenUrl = (TYPE_InternetOpenUrl ) GetProcAddress ( hInternetModule, "InternetOpenUrlA");
	TYPE_InternetCloseHandle _InternetCloseHandle = (TYPE_InternetCloseHandle) GetProcAddress (hInternetModule,"InternetCloseHandle");
	TYPE_InternetReadFile _InternetReadFile = (TYPE_InternetReadFile) GetProcAddress(hInternetModule,"InternetReadFile");
	TYPE_InternetSetOption _InternetSetOption = (TYPE_InternetSetOption) GetProcAddress(hInternetModule,"InternetSetOptionA");

	if(_InternetOpen == NULL || _InternetOpenUrl == NULL || _InternetCloseHandle == NULL || _InternetReadFile == NULL || _InternetSetOption == NULL)
	{
		return 0;
	}


	HINTERNET hInet = _InternetOpen("CoD4X-Updater", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if(hInet == NULL)
	{
		return 0;
	}

	int timeout = 15000;

	_InternetSetOption(hInet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
	_InternetSetOption(hInet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
	_InternetSetOption(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));

	DWORD flags = INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE;
	HINTERNET hUrl = _InternetOpenUrl(hInet, url, NULL, 0, flags, 0);
	if(hUrl == NULL)
	{
		_InternetCloseHandle(hInet);
		return 0;
	}

	int max_size = 1024*1024*4;
	DWORD size = 0;
	void* buffer = malloc(max_size);
	_InternetReadFile(hUrl, buffer, max_size, &size);

	_InternetCloseHandle(hUrl);
	_InternetCloseHandle(hInet);

	if(size < 1)
	{
		free(buffer);
		return 0;
	}
	*buf = buffer;
	return size;
}

int DownloadFile(char* url, wchar_t* to)
{
	void* buf;
	int size = InternetGetContent(url, &buf);

	if(size < 1)
	{
		return 0;
	}

	FILE* f = _wfopen(to, L"wb");
	int l = fwrite(buf, sizeof(char), size, f);
	fclose(f);
	free(buf);
	if(l != size)
	{
		return 0;
	}
	return l;
}


DWORD FindEntryPointAddress( )
{
    BY_HANDLE_FILE_INFORMATION bhfi;
    HANDLE hMapping;
    char *lpBase;

	char exeFile[1024];

	if(GetModuleFileNameA(NULL, exeFile, sizeof(exeFile)) < 6)
	{
		return 0;
	}

    HANDLE hFile = CreateFileA(exeFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

    if (!GetFileInformationByHandle(hFile, &bhfi))
	{
		return 0;
	}

    hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, bhfi.nFileSizeHigh, bhfi.nFileSizeLow, NULL);
    if (!hMapping)
		return 0;

    lpBase = (char *)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, bhfi.nFileSizeLow);
    if (!lpBase)
		return 0;

    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)lpBase;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
        UnmapViewOfFile((LPCVOID)lpBase);
		return 0;
	}
    PIMAGE_NT_HEADERS32 ntHeader = (PIMAGE_NT_HEADERS32)(lpBase + dosHeader->e_lfanew);
    if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
	{
        UnmapViewOfFile((LPCVOID)lpBase);
		return 0;
	}
    DWORD pEntryPoint = ntHeader->OptionalHeader.ImageBase + ntHeader->OptionalHeader.AddressOfEntryPoint;

    UnmapViewOfFile((LPCVOID)lpBase);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    return pEntryPoint;
}

void WriteIW3UpdEntryPoint(void* entrypointaddr)
{
	SetJump((DWORD)entrypointaddr, UpdateIW3MPInternal);
}


void WriteInstallerEntryPoint(void* entrypointaddr)
{
	SetJump((DWORD)entrypointaddr, ReplaceMainModule);
}

void WriteMSSInstallerEntryPoint(void* entrypointaddr)
{
	SetJump((DWORD)entrypointaddr, ReplaceMiles32Module);
}


bool HookEntryPointForInstaller()
{
	DWORD entrypoint = FindEntryPointAddress( );
	if(entrypoint == 0)
	{
		return false;
	}

	Patch_MainModule(WriteInstallerEntryPoint, (void*)entrypoint);
	return true;

}

bool HookEntryPointForIW3Upd()
{
	DWORD entrypoint = FindEntryPointAddress( );
	if(entrypoint == 0)
	{
		return false;
	}
	Patch_MainModule(WriteIW3UpdEntryPoint, (void*)entrypoint);
	return true;
}




bool HookEntryPointForMSSInstaller()
{
	DWORD entrypoint = FindEntryPointAddress( );
	if(entrypoint == 0)
	{
		return false;
	}

	Patch_MainModule(WriteMSSInstallerEntryPoint, (void*)entrypoint);
	return true;

}

qboolean LargeAddressspacePatchEnabled()  //When it fails it will not bother to patch at all
{
	char filename[1024];

	if(GetModuleFileNameA(NULL, filename, sizeof(filename)) < 6)
	{
		return qtrue;
	}
	FILE* f = fopen(filename, "rb");
	if(f == NULL)
	{
		return qtrue;
	}
	if(fseek(f, 286, SEEK_SET) != 0){
		fclose(f);
		return qtrue;
	}

	uint16_t flags;

	size_t s = fread( &flags, 2, 1, f );

	if(s != 1 || flags & IMAGE_FILE_LARGE_ADDRESS_AWARE)
	{
		fclose(f);
		return qtrue;
	}
	fclose(f);
	return qfalse;
}



void ReplaceMiles32ModuleInternal()
{
	FS_SetupSavePath();

	char filename[1024];
	char moduledir[1024];
	char testfile[1024];
	wchar_t ospath[1024];
	char *miles32;

	if(GetModuleFileNameA(NULL, filename, sizeof(filename)) < 6)
	{
		ImpureUpdateFixFail();
		return;
	}

	Sys_SetExeFile(filename);

	Q_strncpyz(moduledir, filename, sizeof(moduledir));

	char* find = strrchr(moduledir, '\\');
	if (find == NULL)
	{
		ImpureUpdateFixFail();
		return;
	}
	*find = '\0';
	Com_sprintf(testfile, sizeof(testfile), "%s\\testfile.tmp", moduledir);
	Com_sprintf(filename, sizeof(filename), "%s\\miles32.dll", moduledir);

	//Disable folder virtualization
    HANDLE Token;
	DWORD disable = 0;

    if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token)){
        SetTokenInformation(Token, (TOKEN_INFORMATION_CLASS)24, &disable, sizeof(disable));
		CloseHandle(Token);
    }

	FILE* f = fopen(testfile, "wb");
	if(f == NULL)
	{
		AutoupdateRequiresElevatedPermissions = qtrue;
		Sys_SetRestartParams("+");
		Sys_Quit();
	}
	fclose(f);
	DeleteFileA(testfile);

	Sec_Init();

	FS_BuildOSPathForThreadUni( FS_GetSavePath(), "bin", "miles32.dll", ospath, 0);

	/* Verifying miles32.dll */

	int len = FS_ReadFileOSPathUni(ospath, (void**)&miles32);
	if(len < 1)
	{
		int size = DownloadFile("http://cod4update.cod4x.me/clupdate/get.php?f=mss", ospath);

		if(size != 434688 || (len = FS_ReadFileOSPathUni(ospath, (void**)&miles32)) < 1)
		{
			ImpureUpdateFixFail();
			return;
		}
	}

	char hash[64] = { 0 };
	long unsigned int outSize = sizeof(hash);

	if(Sec_HashMemory(SEC_HASH_TIGER, miles32, len, hash, &outSize, qfalse) == qfalse || Q_stricmp(hash, "c48e2e74f5e4025637061c063552ba0bd00a04031383c930"))
	{
		/*
		FILE* qf = fopen("msssum.txt", "wt");
		fwrite(hash, 1, sizeof(hash), qf);
		fclose(qf);
		MessageBoxA(NULL, hash, "msshash", MB_OK);
		*/
		FS_FreeFile(miles32);
		DeleteFileW(ospath);
		ImpureUpdateFixFail();
		return;
	}


	DeleteFileA(filename);
	//miles32.dll shall be gone here

	f = fopen(filename, "wb");
	if(f == NULL)
	{
		FS_FreeFile(miles32);
		ImpureUpdateFixFail();
		return;
	}

	int wcount = fwrite(miles32, sizeof(char), len, f);

	FS_FreeFile(miles32);
	fclose(f);

	if(wcount != len)
	{
		ImpureUpdateFixFail();
		return;
	}
}


void ReplaceMainModuleInternal()
{
	FS_SetupSavePath();

	char filename[1024];
	char moduledir[1024];
	char testfile[1024];
	char impurefile[1024];
	wchar_t ospath[1024];
	char *iw3mp;

	if(GetModuleFileNameA(NULL, filename, sizeof(filename)) < 6)
	{
		ImpureUpdateFixFail();
		return;
	}

	Sys_SetExeFile(filename);

	Q_strncpyz(moduledir, filename, sizeof(moduledir));

	char* find = strrchr(moduledir, '\\');
	if (find == NULL)
	{
		ImpureUpdateFixFail();
		return;
	}
	*find = '\0';
	Com_sprintf(testfile, sizeof(testfile), "%s\\testfile.tmp", moduledir);
	Com_sprintf(impurefile, sizeof(impurefile), "%s\\iw3mp.exe.%x.impure", moduledir, Sys_Milliseconds());


	//Disable folder virtualization
    HANDLE Token;
	DWORD disable = 0;

    if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token)){
        SetTokenInformation(Token, (TOKEN_INFORMATION_CLASS)24, &disable, sizeof(disable));
		CloseHandle(Token);
    }

	FILE* f = fopen(testfile, "wb");
	if(f == NULL)
	{
		AutoupdateRequiresElevatedPermissions = qtrue;
		Sys_SetRestartParams("+");
		Sys_Quit();
	}
	fclose(f);
	DeleteFileA(testfile);

	Sec_Init();



	CreateDirectoryW(FS_GetSavePath(), NULL);
	FS_BuildOSPathForThreadUni( FS_GetSavePath(), NULL, "bin", ospath, 0);
	CreateDirectoryW(ospath, NULL);

	FS_BuildOSPathForThreadUni( FS_GetSavePath(), "bin", "iw3mp.exe", ospath, 0);

	/* Verifying iw3mp.exe */

	int len = FS_ReadFileOSPathUni(ospath, (void**)&iw3mp);
	if(len < 1)
	{
		int size = DownloadFile("http://cod4update.cod4x.me/clupdate/get.php?f=core", ospath);

		if(size != 3330048 || (len = FS_ReadFileOSPathUni(ospath, (void**)&iw3mp)) < 1)
		{
			ImpureUpdateFixFail();
			return;
		}
	}

	char hash[64] = { 0 };
	long unsigned int outSize = sizeof(hash);

	if(Sec_HashMemory(SEC_HASH_TIGER, iw3mp, len, hash, &outSize, qfalse) == qfalse || Q_stricmp(hash, "6e1d8ab68083711d7dc1c95dbda263881911ed081393436b"))
	{
		FS_FreeFile(iw3mp);
		DeleteFileW(ospath);
		ImpureUpdateFixFail();
		return;
	}


	if(rename(filename, impurefile) != 0)
	{
		FS_FreeFile(iw3mp);
		ImpureUpdateFixFail();
		return;
	}
	//iw3mp.exe shall be gone here

	f = fopen(filename, "wb");
	if(f == NULL)
	{
		FS_FreeFile(iw3mp);
		rename(impurefile, filename);
		ImpureUpdateFixFail();
		return;
	}

	*((uint16_t*)(&iw3mp[286])) |= IMAGE_FILE_LARGE_ADDRESS_AWARE;

	int wcount = fwrite(iw3mp, sizeof(char), len, f);

	FS_FreeFile(iw3mp);
	fclose(f);

	if(wcount != len)
	{
		rename(impurefile, filename);
		ImpureUpdateFixFail();
		return;
	}
}

void ReplaceMainModule()
{
	ReplaceMainModuleInternal();
	MessageBoxA(NULL, "Call of Duty 4 X Launcher will close now. Restart CoD4 if needed.", "Notice", MB_OK);
	ExitProcess(0);
}

void ReplaceMiles32Module()
{
	ReplaceMiles32ModuleInternal();
	MessageBoxA(NULL, "Call of Duty 4 X Launcher will close now. Restart CoD4 if needed.", "Notice", MB_OK);
	ExitProcess(0);
}

void UpdateIW3MPInternal()
{

	FS_SetupSavePath();

	wchar_t filename[1024];
	char filenamea[1024];

	wchar_t moduledir[1024];
	wchar_t testfile[1024];
	wchar_t impurefile[1024];
	wchar_t ospath[1024];
	char *iw3mp;

	if(GetModuleFileNameW(NULL, filename, sizeof(filename)) < 6)
	{
		IW3MPUpdateFail();
		return;
	}
	if(GetModuleFileNameA(NULL, filenamea, sizeof(filenamea)) < 6)
	{
		IW3MPUpdateFail();
		return;
	}

	Sys_SetExeFile(filenamea);

	Q_strncpyzUni(moduledir, filename, sizeof(moduledir));

	wchar_t* find = wcsrchr(moduledir, '\\');
	if (find == NULL)
	{
		IW3MPUpdateFail();
		return;
	}
	*find = '\0';
	Com_sprintfUni(testfile, sizeof(testfile), L"%s\\testfile.tmp", moduledir);
	Com_sprintfUni(impurefile, sizeof(impurefile), L"%s\\iw3mp.exe.%x.org", moduledir, Sys_Milliseconds());


	//Disable folder virtualization
  	HANDLE Token;
	DWORD disable = 0;

    if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token)){
        SetTokenInformation(Token, (TOKEN_INFORMATION_CLASS)24, &disable, sizeof(disable));
		CloseHandle(Token);
    }

	FILE* f = _wfopen(testfile, L"wb");
	if(f == NULL)
	{
		AutoupdateRequiresElevatedPermissions = qtrue;
		Sys_SetRestartParams("+");
		Sys_Quit();
	}
	fclose(f);
	DeleteFileW(testfile);

	CreateDirectoryW(FS_GetSavePath(), NULL);

	FS_BuildOSPathForThreadUni( FS_GetSavePath(), NULL, "bin", ospath, 0);

	CreateDirectoryW(ospath, NULL);
	FS_BuildOSPathForThreadUni( FS_GetSavePath(), "bin", "iw3mp.exe", ospath, 0);

	/* Verifying iw3mp.exe */

	int len = FS_ReadFileOSPathUni(ospath, (void**)&iw3mp);
	if(len != 3330048)
	{
		CopyFileW(filename, ospath, false);
		len = FS_ReadFileOSPathUni(ospath, (void**)&iw3mp);
		if(len != 3330048)
		{
			int size = DownloadFile("http://cod4update.cod4x.me/clupdate/get.php?f=core", ospath);

			if(size != 3330048 || (len = FS_ReadFileOSPathUni(ospath, (void**)&iw3mp)) < 1)
			{
				ImpureUpdateFixFail();
				return;
			}
		}
	}

	Sec_Init();

	char hash[64] = { 0 };
	long unsigned int outSize = sizeof(hash);

	if(Sec_HashMemory(SEC_HASH_TIGER, iw3mp, len, hash, &outSize, qfalse) == qfalse || Q_stricmp(hash, "6e1d8ab68083711d7dc1c95dbda263881911ed081393436b"))
	{
		FS_FreeFile(iw3mp);
		DeleteFileW(ospath);
		IW3MPUpdateFail();
		return;
	}



	//Grepping security information of file binkw32.dll
	PACL pDACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	
	GetNamedSecurityInfoW(filename, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pDACL, NULL, &pSD);

	if(_wrename(filename, impurefile) != 0)
	{
		FS_FreeFile(iw3mp);
		IW3MPUpdateFail();
		return;
	}
	//iw3mp.exe shall be gone here

	f = _wfopen(filename, L"wb");
	if(f == NULL)
	{
		FS_FreeFile(iw3mp);
		_wrename(impurefile, filename);
		IW3MPUpdateFail();
		return;
	}

	*((uint16_t*)(&iw3mp[286])) |= IMAGE_FILE_LARGE_ADDRESS_AWARE;

	int wcount = fwrite(iw3mp, sizeof(char), len, f);

	FS_FreeFile(iw3mp);
	fclose(f);


	//Apply security information from binkw32.dll to new created file
	if(pSD != NULL)
	{
		SetNamedSecurityInfoW(filename, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL);
       	LocalFree((HLOCAL) pSD);
	}

	if(wcount != len)
	{
		_wrename(impurefile, filename);
		IW3MPUpdateFail();
		return;
	}

	MessageBoxA(NULL, "iw3mp got updated. Please start CoD4MW again", "Update complete", MB_OK);

}









#if 0

/* This function determins if we need elevated permissions, and sets the restart commandline arguments */
void Sys_RunUpdater(  )
{
	wchar_t ospath[MAX_OSPATH];
	void* hWnd = NULL;
	char displayMessageBuf[1024];
	char dir[MAX_STRING_CHARS];
	wchar_t updatepath[MAX_OSPATH];
	wchar_t keyfilepath[MAX_OSPATH];
	char dllpathtruncate[4*MAX_OSPATH];
	char fs_write_pathclean[4*MAX_OSPATH];
	char fs_write_path[4*MAX_OSPATH];
	char errmessage[4*1024];
	const char *dllpath;
	char* search;
	char* keyfile;
	byte* libfile;
	char version[128];
	HMODULE hModule;
	int liblen;
	int keyfilelen;

	typedef struct{
		HINSTANCE reflib_library;           // Handle to refresh DLL
		qboolean reflib_active;
		HWND hWnd;							//0xcc1b6fc
		HINSTANCE hInstance;				//0xcc1b700
		qboolean activeApp;
		qboolean isMinimized;				//0xcc1b708
		OSVERSIONINFOA osversion;
		// when we get a windows message, we store the time off so keyboard processing
		// can know the exact time of an event
		unsigned sysMsgTime;				//0xcc1b710
	} WinVars_t;

	WinVars_t g_wv;

	g_wv.reflib_library = (HINSTANCE)0x0;
	g_wv.reflib_active = qfalse;
	g_wv.hWnd = 0;
	g_wv.hInstance = (HINSTANCE)0x400000;
	g_wv.activeApp = qfalse;
	g_wv.isMinimized = qfalse;
	memset(&g_wv.osversion, 0, sizeof(g_wv.osversion));
	g_wv.sysMsgTime = 0;

	BOOL __cdecl (*runupdate)(HMODULE, const char* fs_write_base, const wchar_t* updatepath, const char* version, int numargs, WinVars_t*);


	FS_SetupSavePath();

	dllpath = Sys_DllFile( );

	if(strlen(dllpath) < 9)
	{
		preInitError("Autoupdate failed because of invalid HMODULE path. Manual installation is required\n");
		return;
	}

	Q_strncpyz(dllpathtruncate, dllpath, sizeof(dllpathtruncate));

	search = strrchr(dllpathtruncate, PATH_SEP);
	if(search)
	{
		*search = '\0';
	}

	Sys_Cwd(fs_write_path, sizeof(fs_write_path));

	Q_strncpyz(fs_write_pathclean, fs_write_path, sizeof(fs_write_pathclean));

	FS_ReplaceSeparators( fs_write_pathclean );
	FS_StripTrailingSeperator( fs_write_pathclean );

	FS_ReplaceSeparators( dllpathtruncate );
	FS_StripTrailingSeperator( dllpathtruncate );

	if(Q_stricmp(dllpathtruncate, fs_write_pathclean))
	{
		Com_sprintf(errmessage, sizeof(errmessage), "Autoupdate: Dll path (%s) and install path (%s) mismatch.\nAutoupdate failed.\n", dllpathtruncate, fs_write_pathclean);

		MessageBoxA(hWnd, errmessage, "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
		return;
	}

	FS_ShiftStr( AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT, dir );

	FS_BuildOSPathForThreadUni( FS_GetSavePath(), dir, "cod4update.dl_", ospath, 0);
	FS_BuildOSPathForThreadUni( FS_GetSavePath(), dir, "cod4update.key", keyfilepath, 0);
	FS_BuildOSPathForThreadUni( FS_GetSavePath(), dir, "", updatepath, 0);

	/* Verifying cod4update.dl_ */

	liblen = FS_ReadFileOSPathUni(ospath, (void**)&libfile);
	if(liblen < 1)
	{
		MessageBoxA(hWnd, "Unable to read updater library", "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
		return;
	}

	keyfilelen = FS_ReadFileOSPathUni(keyfilepath, (void**)&keyfile);
	if(keyfilelen < 1)
	{
		FS_FreeFile(libfile);
		MessageBoxA(hWnd, "Unable to read key file", "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
		return;
	}

	if(Sec_VerifyMemory(keyfile, libfile, liblen) == qfalse)
	{
		FS_FreeFile(libfile);
		FS_FreeFile(keyfile);
		MessageBoxA(hWnd, "Unable to verify file. Checksum mismatch.", "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
		return;
	}
	FS_FreeFile(libfile);
	FS_FreeFile(keyfile);

	hModule = LoadLibraryW( ospath );
	if( hModule == NULL )
	{
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						0, GetLastError(), 0x400, displayMessageBuf, sizeof(displayMessageBuf), 0);

		wchar_t messagecnt[MAX_STRING_CHARS];
		wchar_t werrmessage[MAX_STRING_CHARS];

		Q_StrToWStr(werrmessage, displayMessageBuf, sizeof(werrmessage));
		Com_sprintfUni(messagecnt, sizeof(messagecnt), L"Loading of the autoupdate module %s has failed.\nError: %s\n", ospath, displayMessageBuf);

		MessageBoxW(hWnd, messagecnt, L"Call of Duty Update Failed", MB_OK | MB_ICONERROR);
		return;
	}

	runupdate = (void*)GetProcAddress( hModule, "CoD4UpdateMain");
	if( runupdate == NULL )
	{
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						0, GetLastError(), 0x400, displayMessageBuf, sizeof(displayMessageBuf), 0);

		MessageBoxA(hWnd, va("Couldn't find the procedure entrypoint for %s\nError: %s\n", "CoD4UpdateMain" ,displayMessageBuf), "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
		return;
	}

	if(runupdate( hModule, fs_write_pathclean, updatepath, "0.0", 1, &g_wv))
	{
		FreeLibrary( hModule );
		Sys_SetRestartParams( "+nosplash" );
	}

	FreeLibrary( hModule );
	Sys_Quit();
}
#endif

void Sys_RunUpdater(  )
{
	wchar_t ospath[MAX_OSPATH];
	void* hWnd = NULL;
	char displayMessageBuf[1024];
	char dir[MAX_STRING_CHARS];
	wchar_t updatepath[MAX_OSPATH];
	char dllpathtruncate[4*MAX_OSPATH];
	char fs_write_pathclean[4*MAX_OSPATH];
	char fs_write_path[4*MAX_OSPATH];
	char errmessage[4*1024];
	const char *dllpath;
	char* search;
	byte* libfile;
	char version[128];
	HMODULE hModule;
	int liblen;

	typedef struct{
		HINSTANCE reflib_library;           // Handle to refresh DLL
		qboolean reflib_active;
		HWND hWnd;							//0xcc1b6fc
		HINSTANCE hInstance;				//0xcc1b700
		qboolean activeApp;
		qboolean isMinimized;				//0xcc1b708
		OSVERSIONINFOA osversion;
		// when we get a windows message, we store the time off so keyboard processing
		// can know the exact time of an event
		unsigned sysMsgTime;				//0xcc1b710
	} WinVars_t;

	WinVars_t g_wv;

	g_wv.reflib_library = (HINSTANCE)0x0;
	g_wv.reflib_active = qfalse;
	g_wv.hWnd = 0;
	g_wv.hInstance = (HINSTANCE)0x400000;
	g_wv.activeApp = qfalse;
	g_wv.isMinimized = qfalse;
	memset(&g_wv.osversion, 0, sizeof(g_wv.osversion));
	g_wv.sysMsgTime = 0;

	BOOL __cdecl (*runupdate)(HMODULE, const char* fs_write_base, const wchar_t* updatepath, const char* version, int numargs, WinVars_t*);


	FS_SetupSavePath();

	dllpath = Sys_DllFile( );

	if(strlen(dllpath) < 9)
	{
		preInitError("Autoupdate failed because of invalid HMODULE path. Manual installation is required\n");
		return;
	}

	Q_strncpyz(dllpathtruncate, dllpath, sizeof(dllpathtruncate));

	search = strrchr(dllpathtruncate, PATH_SEP);
	if(search)
	{
		*search = '\0';
	}

	Sys_Cwd(fs_write_path, sizeof(fs_write_path));

	Q_strncpyz(fs_write_pathclean, fs_write_path, sizeof(fs_write_pathclean));

	FS_ReplaceSeparators( fs_write_pathclean );
	FS_StripTrailingSeperator( fs_write_pathclean );

	FS_ReplaceSeparators( dllpathtruncate );
	FS_StripTrailingSeperator( dllpathtruncate );

	if(Q_stricmp(dllpathtruncate, fs_write_pathclean))
	{
		Com_sprintf(errmessage, sizeof(errmessage), "Autoupdate: Dll path (%s) and install path (%s) mismatch.\nAutoupdate failed.\n", dllpathtruncate, fs_write_pathclean);

		MessageBoxA(hWnd, errmessage, "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
		return;
	}

	FS_ShiftStr( AUTOUPDATE_DIR, AUTOUPDATE_DIR_SHIFT, dir );

	char infobuf[BIG_INFO_STRING];
	char* updateinfodata;
	int size = InternetGetContent(UPDATE_SERVER_NAME "?mode=0", (void**)&updateinfodata);

	if(size <= 0)
	{
		MessageBoxA(hWnd, "Unable to read update info from remote updateserver. Try again later.", "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
		return;
	}

	char updateHash[1024]; //RSA encrypted base64 hash
	char updateFile[1024];
	char updateVersionStr[1024];


	Q_strncpyz(updateVersionStr, BigInfo_ValueForKey_tsInternal(updateinfodata, "version", infobuf), sizeof(updateVersionStr));
	Q_strncpyz(updateHash, BigInfo_ValueForKey_tsInternal(updateinfodata, "hash", infobuf), sizeof(updateHash));
	Q_strncpyz(updateFile, BigInfo_ValueForKey_tsInternal(updateinfodata, "url", infobuf), sizeof(updateFile));

	free(updateinfodata);

	FS_BuildOSPathForThreadUni( FS_GetSavePath(), dir, "cod4update.dl_", ospath, 0);
	FS_BuildOSPathForThreadUni( FS_GetSavePath(), dir, "", updatepath, 0);

	/* Verifying cod4update.dl_ */

	liblen = FS_ReadFileOSPathUni(ospath, (void**)&libfile);
	if(liblen < 1 || Sec_VerifyMemory(updateHash, libfile, liblen) == qfalse)
	{

		size = DownloadFile(updateFile, ospath);
		if(size < 1)
		{
			MessageBoxA(hWnd, "Unable to read updater library from remote host!", "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
			return;
		}
		liblen = FS_ReadFileOSPathUni(ospath, (void**)&libfile);
		if(liblen < 1)
		{
			MessageBoxA(hWnd, "Unable to read updater library from local drive!", "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
			return;
		}

		if(Sec_VerifyMemory(updateHash, libfile, liblen) == qfalse)
		{
			FS_FreeFile(libfile);
			MessageBoxA(hWnd, "Unable to read updater library. Checksum mismatch - File corrupted.\nNote: \"ESET\" is known to corrupt downloaded files if running.", "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
			return;
		}
	}

	FS_FreeFile(libfile);

	hModule = LoadLibraryW( ospath );
	if( hModule == NULL )
	{
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						0, GetLastError(), 0x400, displayMessageBuf, sizeof(displayMessageBuf), 0);

		wchar_t messagecnt[MAX_STRING_CHARS];
		wchar_t werrmessage[MAX_STRING_CHARS];

		Q_StrToWStr(werrmessage, displayMessageBuf, sizeof(werrmessage));
		Com_sprintfUni(messagecnt, sizeof(messagecnt), L"Loading of the autoupdate module %s has failed.\nError: %s\n", ospath, displayMessageBuf);

		MessageBoxW(hWnd, messagecnt, L"Call of Duty Update Failed", MB_OK | MB_ICONERROR);
		return;
	}

	runupdate = (void*)GetProcAddress( hModule, "CoD4UpdateMain");
	if( runupdate == NULL )
	{
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						0, GetLastError(), 0x400, displayMessageBuf, sizeof(displayMessageBuf), 0);

		MessageBoxA(hWnd, va("Couldn't find the procedure entrypoint for %s\nError: %s\n", "CoD4UpdateMain" ,displayMessageBuf), "Call of Duty Update Failed", MB_OK | MB_ICONERROR);
        FreeLibrary(hModule);
		return;
	}

	if(runupdate( hModule, fs_write_pathclean, updatepath, "0.0", 1, &g_wv))
		Sys_SetRestartParams( "+nosplash" );

	FreeLibrary( hModule );
	Sys_Quit();
}



int Sys_ListDirectories(const wchar_t* dir, char** list, int limit)
{
  int count, i;
  HANDLE dhandle;
  struct _WIN32_FIND_DATAW FindFileData;
  char *ansifilename;
  wchar_t searchdir[MAX_OSPATH];
  char errorString[1024];

  list[0] = NULL;

  FS_BuildOSPathForThreadUni( dir, "*", "", searchdir, 0 );

  dhandle = FindFirstFileW(searchdir, &FindFileData);
  if ( dhandle == (HANDLE)-1 )
  {
	Sys_GetLastErrorAsString(errorString, sizeof(errorString));
	Com_DPrintf("Sys_ListDirectories: %s\n", errorString);
    return 0;
  }
  count = 0;

  do
  {

    if ( !FindFileData.cFileName[0])
    {
		continue;
		}
		if(FindFileData.cFileName[0] == L'.' && (FindFileData.cFileName[1] == L'.' || !FindFileData.cFileName[1]))
		{
			continue;
		}

		/* is directory ? */
	  if ( FindFileData.dwFileAttributes & 0x10)
		{

			if(Q_WIsAnsiString(FindFileData.cFileName) == qfalse)
			{

				//We won't support non ANSI chars
				continue;
			}

			list[count] = malloc(wcslen(FindFileData.cFileName) +1);

			if(list[count] == NULL)
			{
				break;
			}

			ansifilename = list[count];

			/* String copy as ANSI string */
			for(i = 0; FindFileData.cFileName[i]; ++i)
			{
				ansifilename[i] = FindFileData.cFileName[i];
			}
			ansifilename[i] = '\0';
			count++;
		}

  }
  while ( FindNextFileW(dhandle, &FindFileData) != 0 && count < limit -1);

  FindClose(dhandle);

  list[count] = NULL;

  return count;
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
		free( list[i] );
	}

}


//I know this will not work
#define ASSET_TYPE_WEAPON_COUNT 144 

const char* __CL_GetConfigstring(int index)
{
	gameState_t* gs = (gameState_t*)(0xC5F930 + 0x2fbc);

	int offset = gs->stringOffsets[index];
	return gs->stringData + offset;
}

void __CG_RegisterWeapon(int index);

void __cdecl CG_RegisterItems()
{
  int i;
  signed int v4;
  int v5;
  char items[1024];

  Q_strncpyz(items,  __CL_GetConfigstring(2314), sizeof(items));

  for(i = 1; i < ASSET_TYPE_WEAPON_COUNT; ++i)
  {
    v4 = items[i / 4];

    if ( v4 > '9' )
    {
        v5 = v4 - 'W';
    }else{
        v5 = v4 - '0';
    }


    if ( (1 << (i & 3)) & v5 )
    {
		__CG_RegisterWeapon(i);
    }
  }

}



#define HMAX 256 /* Maximum symbol */
#define INTERNAL_NODE ( HMAX + 1 )

typedef struct nodetype {
	struct  nodetype *left, *right, *parent; /* tree structure */
//	struct  nodetype *next, *prev; /* doubly-linked list */
//	struct  nodetype **head; /* highest ranked node in block */
	int weight;
	int symbol; //0x10
//	struct  nodetype *next, *prev; /* doubly-linked list */
//	struct  nodetype **head; /* highest ranked node in block */

} node_t; //Length: 20

static int bloc;

/* Receive one bit from the input file (buffered) */
static int get_bit( const byte *fin ) {
	int t;

	t = ( fin[( bloc >> 3 )] >> ( bloc & 7 ) ) & 0x1;
	bloc++;
	return t;
}

static void __Huff_offsetReceive( node_t *node, int *ch, const byte *fin, int *offset ) {

	bloc = *offset;
	while ( node && node->symbol == INTERNAL_NODE ) {

		if ( get_bit( fin ) ) {
			node = node->right;

		} else {
			node = node->left;

		}
	}
	if ( !node ) {
		*ch = 0;
//		Com_PrintError("Illegal tree!\n");
		return;

	}
	*ch = node->symbol;
	*offset = bloc;
}


int MSG_ReadBitsCompress(const byte *from, int fromSizeBytes, byte *to, int toSizeBytes){

    int get;
	int bits;
	int bit;
	int i;
	byte *data;

    bits = 8 * fromSizeBytes;
    i = 0;
    data = to;
    bit = 0;
	bloc = 0;
    while ( bit < bits && i < toSizeBytes)
    {
	    __Huff_offsetReceive(*(node_t**)0x14B8C88, &get, from, &bit);
        *data = (byte)get;
		++data;
		++i;
    }
    return data - to;

}

int __regparm1 __MSG_ReadBitsCompress_Client(const byte* input, byte* outputBuf, int readsize)
{

	return MSG_ReadBitsCompress(input, readsize, outputBuf, 0x20000);

}

int __regparm1 __MSG_ReadBitsCompress_Server(const byte* input, byte* outputBuf, int readsize)
{
	return MSG_ReadBitsCompress(input, readsize, outputBuf, 0x800);
}
