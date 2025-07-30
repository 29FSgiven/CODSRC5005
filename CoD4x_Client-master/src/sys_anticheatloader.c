#include "q_shared.h"
#include "qcommon.h"
#include "win_sys.h"
#include "client.h"
#include "ui_shared.h"
#include "tomcrypt/tomcrypt.h"


#define xac_masterpem "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsGYuxLunXRIizYJd2dDp\
2MoTzG0IEKRpMUwHnNiSey94t8zUaDqc+sW8AK9DcVsmjzmph300DVgrkLo642Q/\
gYQbd8OJ3dwIY7w553ssspihdSGnNUdweIraVmOSnc2zFnDgBk6MW4Mum/kuFMAp\
zyjiZza7qcBOQu9csmWNEo/WpUC9vyJMdJesNWGeVbv2bj2gG2EKC/+Ylo6BGqqu\
b5k2u+l2sWzu+VWKK6WZtBRhkGnSVlTx34ZKwSAOr4S2yEZqpDCWlkEzbr2fB6Un\
U8u/I+d7fpVUWXBR+/Bcw4Br1DM2YCYhlitPIAdIS0oDhE7K0D2q7n3pTPyETWep\
kwIDAQAB"

qboolean Sys_GetAnticheatPath(wchar_t *outpath, int maxpathlen)
{
	wchar_t path[1024];

	if(GetModuleFileNameW(Sys_DllHandle(), path, sizeof(path) /2) == 0)
	{
		return qfalse;
	}
	wchar_t* end = wcsrchr(path, L'\\');
	if(end == NULL){
		return qfalse;
	}
	*end = 0;
	Com_sprintfUni(outpath, maxpathlen, L"%s\\iam_clint.dll", path);

	return qtrue;
}



int Sys_WriteAnticheatFile(byte* data, int len)
{
	wchar_t acdllpath[1024];

	if(!Sys_GetAnticheatPath(acdllpath, sizeof(acdllpath)))
	{
		return 0;
	}

	FILE* f = _wfopen(acdllpath, L"wb");
	if(f == NULL)
	{
		return 0;
	}
	int l = fwrite(data, 1, len, f);
	fclose(f);
	return l;

}


qboolean Sys_VerifyAnticheatFile(const char* cmphash)
{
	wchar_t acdllpath[1024];
	byte data[0x8000];
    const char	*hex = "0123456789abcdef";
	char finalsha65b[65];
    unsigned char hash[32];
	int i, r;
    hash_state ctx;


	if(!Sys_GetAnticheatPath(acdllpath, sizeof(acdllpath)))
	{
		return 0;
	}

	FILE* fd = _wfopen(acdllpath, L"rb");
	if(fd == NULL)
	{
		return qfalse;
	}
    blake2s_256_init(&ctx);

    while(1)
    {
        int s = fread(data, 1, sizeof(data), fd);
        if(s < 1)
        {
            break;
        }
        blake2s_process(&ctx, data, s);
    }
    fclose(fd);

    blake2s_done( &ctx, hash);

    for(i = 0, r=0; i < sizeof(hash); i++) {
        finalsha65b[r++] = hex[hash[i] >> 4];
        finalsha65b[r++] = hex[hash[i] & 0xf];
    }
    finalsha65b[64] = '\0';

	if(Q_stricmp(finalsha65b, cmphash) == 0)
	{
		return qtrue;
	}

	return qfalse;
}

int anticheatLoaderVersion = 0;
int anticheatdeinit = 0;
int anticheatpure = 0;
static char xacerrormessage[1024];


qboolean Sys_VerifyAnticheatSignature(unsigned char* data, int len, const char* pemcert)
{
    	char base64hash[4096];

		base64hash[0] = 0;
		int i;

        for(i = len -1; i > len - sizeof(base64hash); --i)
		{
			if(data[i] == '#' && strncmp((const char*)data +i, "#SIGNATURE-START#", 17) == 0)
			{
				int l = len - i - 17;
				if(l > sizeof(base64hash) -1)
				{
					return qfalse;
				}
				Com_Memcpy(base64hash, data +i +17, l);
				base64hash[l] = 0;
				break;
			}
		}

		if(base64hash[0] == 0)
		{
			return qfalse;
		}

		if(Sec_VerifyMemory(base64hash, data, i, pemcert) == qfalse)
		{
        	return qfalse;
		}
        return qtrue;
}


qboolean Sys_VerifyAnticheatMasterSig(unsigned char* data, int len)
{
	DWORD dwAttrib = GetFileAttributes("iam-developer.txt");

	if(dwAttrib == INVALID_FILE_ATTRIBUTES || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		Com_Error(ERR_DROP, "Signature verification is disabled\n. Allowed to bypass in developer mode only");
	}
	return true;

//	return Sys_VerifyAnticheatSignature(data, len, xac_masterpem);
}

static HMODULE hac = NULL;

void DB_PureInfoSetCallbacks(void (*DB_PureInfoDestroy)(), void (*DB_PureInfoBegin)(), void (*DB_PureInfoReadChunk)(long unsigned int));


typedef struct
{
	void (*Com_Error)(int code, const char* fmt, ...);
	void (*Menus_OpenByName)(const char* menuStr);
	void (*Menus_CloseByName)(const char *menuStr);
	void (*Menu_Open)(menuDef_t *menu);
	void (*Menu_Close)(menuDef_t *menu);
	void (*UI_CloseAllMenus)();
	void (*Com_Printf)(conChannel_t channel, const char* fmt, ...);
	void (*Cbuf_AddText)(const char* text);
	void (*Cmd_AddCommand)(const char* cmdname, void *);
	void (*NetSendData)(msg_t*);
	qboolean (*VerifyAnticheatSignature)(unsigned char* data, int filesize, const char* pem);
	void (**CL_Netchan_Encode)( byte *data, int cursize );
	void (**CL_Netchan_Decode)(byte *data, int length);
	void (*Deinitialize)(int mode);
	void (*DB_PureInfoSetCallbacks)(void (*DB_PureInfoDestroy)(), void (*DB_PureInfoBegin)(), void (*DB_PureInfoReadChunk)(long unsigned int));
	void (*SetErrorMessage)(const char* errormsg);
	void (*SetImpure)();
}ac_imports_t;

struct imagedata_t
{
	HMODULE cod4xcore;
	HMODULE msvcrt;
};

struct IXAC_EX
{
    int (*Initialize)();
	int (*EarlyInitialize)(); //after download is complete
    int (*ReturnVersion)();
    int (*Terminate)();
	int (*RunFrame)();
	void (*NetDataCallback)(msg_t*);
	void (*ConnectToServer)();
	int (*VerifyImage)();
};

struct IXAC_EX *ixac;

//Remove anticheat
//mode 1 = unload from memory
//mode 2 = unload and delete from disk
void Sys_DeinitAC(int mode)
{
	anticheatdeinit = mode;
}


void Sys_SetXACImpure()
{
	anticheatpure = 0; 
}

void Sys_SetXACErrorMessage(const char* errormessage)
{
	Q_strncpyz(xacerrormessage, errormessage, sizeof(xacerrormessage));
}

qboolean Sys_LoadAnticheatInternal(qboolean earlyinit, HMODULE hmod)
{
	wchar_t acdllpath[1024];
	struct IXAC_EX* (*farproc)(const char* version, int interfaceversion, ac_imports_t* imp, struct imagedata_t*);
	char versionstring[1024];
	bool restart;

	anticheatLoaderVersion = 0;

	if(!Sys_GetAnticheatPath(acdllpath, sizeof(acdllpath)))
	{
		return qfalse;
	}

    FILE* fd = _wfopen(acdllpath, L"rb");
    if(!fd)
    {
		return qfalse;
    }

    fseek(fd, 0L, SEEK_END);

    // calculating the size of the file
    int filesize = ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    if(filesize < 1)
    {
        fclose(fd);
		return qfalse;
    }

    void* data = malloc(filesize);
    if(data == NULL)
    {
        fclose(fd);
		return qfalse;
    }

    if(fread(data, 1, filesize, fd) != filesize)
    {
        fclose(fd);
        free(data);
		return qfalse;
    }
    fclose(fd);

    if(Sys_VerifyAnticheatMasterSig(data, filesize) == qfalse)
    {
		free(data);
		return qfalse;
    }
	free(data);


	ac_imports_t imp;
	imp.Com_Error = Com_Error;
	imp.Com_Printf = Com_Printf;
	imp.Cbuf_AddText = Cbuf_AddText;
	imp.Cmd_AddCommand = Cmd_AddCommand;
	imp.Menu_Open = Menu_Open;
	imp.Menus_OpenByName = Menus_OpenByName;
	imp.Menus_CloseByName = Menus_CloseByName;
	imp.Menu_Close = Menu_Close;
	imp.UI_CloseAllMenus = UI_CloseAllMenus;
	imp.NetSendData = CL_SendReliableClientCommand;
	imp.VerifyAnticheatSignature = Sys_VerifyAnticheatSignature;
	imp.CL_Netchan_Encode = &CL_Netchan_Encode;
	imp.CL_Netchan_Decode = &CL_Netchan_Decode;
	imp.Deinitialize = Sys_DeinitAC;
	imp.DB_PureInfoSetCallbacks = DB_PureInfoSetCallbacks;
	imp.SetErrorMessage = Sys_SetXACErrorMessage;
	imp.SetImpure = Sys_SetXACImpure;
	struct imagedata_t imagedata;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN, "msvcrt100.dll", &imagedata.msvcrt);
	imagedata.cod4xcore = hmod;

	hac = LoadLibraryExW(acdllpath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if(hac)
	{
		farproc = (void*)GetProcAddress(hac, "GetInterface");
		if(farproc)
		{
			Com_BuildVersionString(versionstring, sizeof(versionstring));

			restart = false;

			ixac = farproc( versionstring, 1, &imp, &imagedata);
			if(earlyinit)
			{
				restart = ixac->EarlyInitialize();

			}else{
				ixac->Initialize();
			}
			anticheatLoaderVersion = ixac->ReturnVersion();
			if(restart)
			{
				CL_RestartAndReconnect();
			}
			return qtrue;
		}
		FreeLibrary(hac);
	}
	return qfalse;

}

void Sys_LoadAnticheat(HMODULE hMod)
{
	Sys_LoadAnticheatInternal(qfalse, hMod);

	if(anticheatdeinit > 0)
	{
		Sys_UnloadAnticheat();
	}
	if(anticheatdeinit > 1)
	{
		Sys_RemoveAnticheatLoader();
	}
}

void Sys_LoadAnticheatAfterDownload()
{
	if(Sys_LoadAnticheatInternal(qtrue, NULL) == qfalse)
	{
		Com_Error(ERR_DROP, "Early Init of Anticheat failed. Try to restart game.");
	}
}

int Sys_GetAnticheatLoaderVersion()
{
	return anticheatLoaderVersion;
}

void Sys_UnloadAnticheat()
{
	if(ixac)
	{
		ixac->Terminate();

		if(FreeLibrary(hac) != 0)
		{
			hac = NULL;
			ixac = NULL;
		}
	}
}

void Sys_RunAnticheatFrame()
{
	if(ixac)
	{
		ixac->RunFrame();
	}
}

void Sys_RemoveAnticheatLoader()
{
	wchar_t acdllpath[1024];

	if(!Sys_GetAnticheatPath(acdllpath, sizeof(acdllpath)))
	{
		return;
	}
	DeleteFileW(acdllpath);
}

void Sys_InitAnticheatAfterDownload()
{
	Sys_LoadAnticheatAfterDownload();

	if(anticheatdeinit > 0)
	{
		Sys_UnloadAnticheat();
	}
	if(anticheatdeinit > 1)
	{
		Sys_RemoveAnticheatLoader();
	}

}

void Sys_RunAnticheatNetDataCallback(msg_t* msg)
{
	if(ixac)
	{
		ixac->NetDataCallback(msg);
	}
}

void Sys_ConnectToAnticheatServer()
{
	if(ixac)
	{
		ixac->ConnectToServer();
	}
}

void Sys_VerifyGameImage()
{
	if(ixac)
	{
		anticheatpure = ixac->VerifyImage();
	}
	if(anticheatdeinit > 0)
	{
		Sys_UnloadAnticheat();
	}
	if(anticheatdeinit > 1)
	{
		Sys_RemoveAnticheatLoader();
	}
}

int Sys_ImpureGameDetected()
{
	return anticheatpure;
}

const char* Sys_XACErrorMessage()
{
	return xacerrormessage;
}

void Sys_XACClearError()
{
	xacerrormessage[0] = 0;
}