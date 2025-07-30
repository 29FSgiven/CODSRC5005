#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include "zutil.h"
#include "configuration.h"
#include "checksumengine.h"
#include "../../exeobfus/exeobfus.h"
#include "sharedconfig.h"
#include "common/basic.h"
#include "elfhlp.h"
#include "common/rsa_verify.h"
#include "common/core.h"

char build_string[256];


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

static struct IXAC_EX iXAC_EX;

const unsigned char rsa_public_key[] = {0x30,0x82,0x1,0x22,0x30,0xd,0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x1,0x1,0x5,0x0,0x3,0x82,0x1,0xf,0x0,0x30,0x82,0x1,0xa,0x2,0x82,0x1,0x1,0x0,0xd5,0x4a,0x51,0x67,0x39,0x55,0xc1,0x31,0x8d,0x76,0x18,0x68,0x66,0x43,0xb0,0x59,0x1a,0xac,0xfc,0x7d,0x54,0x50,0x19,0xb1,0x5f,0x42,0x4a,0x2c,0x59,0x33,0x44,0xbc,0x2e,0x9b,0x71,0x91,0xf3,0xc4,0x4c,0xc6,0x53,0x97,0x99,0x60,0x3b,0x69,0xbe,0xa8,0x9b,0x46,0x18,0x9f,0x2d,0x6a,0x4b,0x78,0x87,0x3e,0x9f,0x64,0xed,0x39,0xdd,0x7a,0xe5,0x20,0xbb,0xe4,0x95,0xe,0x8c,0x9d,0x4e,0x93,0xa9,0xbe,0x39,0x93,0xf5,0xeb,0x18,0xbf,0xa7,0x5b,0x72,0xb0,0x6d,0xeb,0x3d,0xf1,0xed,0x2f,0xf8,0x1,0x64,0xc2,0xf3,0x4d,0x7d,0x81,0x49,0x27,0x2b,0x21,0x2d,0xc9,0x4b,0xde,0xc6,0x37,0x6c,0x9f,0x4d,0x34,0xfc,0xdd,0xd3,0x93,0x3f,0xb7,0xc2,0x5,0xb0,0x9,0xb2,0x2b,0xfb,0x59,0x84,0x64,0xae,0xd3,0xa3,0x2f,0xbe,0x6c,0x57,0xa2,0x8c,0xaf,0xc8,0xe5,0x47,0x3a,0xea,0x61,0x1d,0x32,0xd3,0x5,0x50,0x5d,0xc2,0x93,0x7d,0x4,0x93,0x16,0x4e,0xe1,0xa8,0x12,0xbe,0xe3,0x9b,0x4c,0xc8,0x13,0x45,0xc2,0x85,0x72,0xbb,0xc2,0xb9,0x27,0xbf,0x4,0x4a,0x3c,0x68,0x3d,0x69,0xbe,0x8a,0x5e,0x9f,0xe6,0x5b,0xbd,0x6d,0xaf,0x24,0x57,0x68,0x72,0x34,0x25,0x5,0x3b,0x11,0x9c,0x13,0xc4,0xf4,0x1a,0x28,0x1d,0x16,0x46,0xea,0x85,0x86,0xd,0x6b,0x8e,0xd4,0x94,0x4a,0xae,0xea,0xf8,0x25,0x75,0xd6,0xb6,0xfe,0x90,0x83,0x84,0xb1,0x3c,0xaa,0x4a,0xa2,0x9d,0x7,0x17,0xb8,0x98,0xc2,0x83,0x60,0xa5,0x92,0x89,0x27,0x8c,0x55,0xac,0xd5,0x86,0xf9,0x8e,0x8a,0xa5,0x2,0x3,0x1,0x0,0x1};

ac_imports_t imp;

void iprintf(const char* fmt, ...)
{
    char buffer[1024];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, va);
    va_end(va);

    imp.Com_Printf(0, "%s", buffer);
}

int LoadAndExecuteFiledata(unsigned char* filedata, unsigned int filesize)
{
    unsigned char* image = ELFHlp_LoadAndAllocImage(filedata, filesize);

    if(image == NULL)
    {
        return 0;
    }
    
    const char* (*entrypoint)(void (*)(const char*, ...), unsigned char*) = (const char* (*)(void (*)(const char*, ...), unsigned char*))ELFHlp_GetProcAddress(image, "XAC_StartupSlave");

    if(entrypoint == NULL)
    {
        ELFHlp_FreeImage(image);
        return 0;
    }

    const char* ret = entrypoint(iprintf, image);


    iprintf("Result=%s\n", ret);

    
    return 1;

}



class IDownload
{
    public:
        IDownload()
        {
            dlbuffer = NULL;
            dlsize = 0;
        }
        ~IDownload()
        {
            if(dlbuffer != NULL)
            {
                VirtualFree(dlbuffer, 0, MEM_RELEASE);
                dlbuffer = NULL;
                dlsize = 0;
            }
        }
        bool Read(msg_t* msg)
        {
            byte data[0x10000];
            if(MSG_ReadByte(msg) == 1)
            {
                if(dlbuffer != NULL)
                {
                    VirtualFree(dlbuffer, 0, MEM_RELEASE);
                    dlbuffer = NULL;
                }
                dlsize = MSG_ReadLong(msg);
                if(dlsize < 0 || dlsize > 0x1000000)
                {
                    throw Error("Bad filesize");
                }
                dlbuffer = (unsigned char*)VirtualAlloc(NULL, dlsize, MEM_COMMIT, PAGE_READWRITE);
                if(dlbuffer == NULL)
                {
                    throw Error("Out of memory");
                }
                dlwritecount = 0;
            }

            unsigned int offset = MSG_ReadLong(msg);
            unsigned int blocksize = MSG_ReadLong(msg);

            if(dlbuffer == NULL && blocksize != 0)
            {
                throw Error("Bad download command");
            }

            if(offset + blocksize > dlsize || blocksize > sizeof(data))
            {
                throw Error("Bad download blocksize");
            }

            if(blocksize > 0)
            {
                //Com_Printf(CON_CHANNEL_CLIENT, "Received block with offset %d, size %d\n", offset, blocksize);
                MSG_ReadData(msg, data, blocksize);
                memcpy(dlbuffer + offset, data, blocksize);
                dlwritecount += blocksize;
            }

            if(blocksize == 0 && dlwritecount == dlsize)
            {
                //ToDo add decrypt of file
                if(Sys_VerifyAnticheatSignature(dlbuffer, dlwritecount, rsa_public_key, sizeof(rsa_public_key)) == false)
                {
                    throw Error("Unsigned anticheat module received from server");
                }
                return true;

            }
            return false;

        }
        int GetFileBuffer(unsigned char** buffer)
        {
            *buffer = dlbuffer;
            return dlwritecount;
        }
        class Error
        {
            public:
            Error(const char* msg)
            {
                lasterror = msg;
            }
            const char* lasterror;
        };
    private:
        unsigned char* dlbuffer;
        unsigned int dlsize;
        unsigned int dlwritecount;
};


static IDownload *downloader;

enum xac_state
{
    CS_INITALIZED,
    CS_CONNECT,
    CS_BEGINDOWNLOAD,
    CS_DONEDOWNLOAD,
    CS_SLAVEREADY
};

static enum xac_state state;

//Only 1 instance of this object is supported
class IXAC
{
    public:

    static int RunFrame()
    {
        unsigned char* fd;
        unsigned int fs;
        switch(state)
        {
            case CS_DONEDOWNLOAD:
                fs = downloader->GetFileBuffer(&fd);
                LoadAndExecuteFiledata(fd, fs);

                delete downloader;
                downloader = NULL;
                state = CS_SLAVEREADY;
                break;
            case CS_SLAVEREADY:

                break;
            default:
                break;
        }

        return 0;
    }

    static int EarlyInitialize()
    {
        //Initialize();
        //enabling this later, I guess 
        //imp.Com_Error(2, "You have to restart your game before you can play onto protected servers.");
        return 1; //1 is restarting
    }
    
    static int ReturnVersion()
    {
        return XAC_LOADERVERSION;
    };

    static int VerifyImage()
    {
        CODECRC();
        char buf[1024];
        char errormessage[2048];
        int status = AC_ChecksumVerifyMain(build_string);
        if(status == -22)
        {
            PROTECTSTRING("XAC is unsupported with your current version (%s) of game.\nPlease check for possible available updates on the main menu.\nXAC will uninstall from game and you won't be able to join protected servers.", buf);
            snprintf(errormessage, sizeof(errormessage), buf, build_string);
            imp.SetErrorMessage(errormessage);
            imp.Deinitialize(2);
            return 1;
        }
        if(status == 1){ //tampered
            imp.Deinitialize(2);
            return 1;
        }
        else if(status == 0)
        {
            return 0;
        }else{

            PROTECTSTRING("XAC detected an error in verifying your game.\nError code is %d\nXAC will uninstall from game and you won't be able to join protected servers.", buf);
            snprintf(errormessage, sizeof(errormessage), buf, status);
            imp.SetErrorMessage(errormessage);
            imp.Deinitialize(2);
            //error in verifying
        }
        return 1; //0 = pure, 1 = impure
    }

    static int Initialize()
    {
        TomCrypt_Init();
        state = CS_INITALIZED;
    /*
        DWORD tid;
        int param = 0;
    */
    //    CreateThread(NULL, 0x10000, AC_Main, &param, 0, &tid);
        return 1;

    };

    static void NetDataCallback(msg_t* msg)
    {
        unsigned int msgcmd = MSG_ReadLong(msg);
        switch(msgcmd)
        {
            case ac_downloadblock:
                if(state == CS_CONNECT)
                {
                    state = CS_BEGINDOWNLOAD;
                }
                if(!downloader)
                {
                    downloader = new IDownload();
                }
                try
                {
                    if(downloader->Read(msg) == true)
                    {
                        imp.Com_Printf(0, "^5Download of slave is complete!\n");
                        state = CS_DONEDOWNLOAD;
                    }
                }catch(IDownload::Error& error){
                    imp.Com_Error(ERR_DROP, error.lasterror);
                }
                break;
            case ac_internalcommand:
                break;
            default:
                
                break;
        }
        
    }

    static int Terminate( )
    {
        return 0;
    };
    static void Connecting( )
    {
        CODECRC();
        if(downloader)
        {
            delete downloader;
            downloader = NULL;
        }
        if(AC_ChecksumVerifyLate( ) == false)
        {
            imp.Com_Printf(0, "^@Impure\n");
            imp.SetImpure();
            return;
        }
        msg_t msg;
        byte buf[0x400];

        MSG_Init(&msg, buf, sizeof(buf));
        MSG_WriteLong(&msg, 0); //size
        MSG_WriteLong(&msg, clc_acdata);
        MSG_WriteLong(&msg, ac_masterreadysignal);
        MSG_WriteString(&msg, "Hello!");
        MSG_WriteLong(&msg, IXAC::ReturnVersion());
        MSG_WriteLong(&msg, 0);
        imp.NetSendData(&msg);
        state = CS_CONNECT;
        imp.Com_Printf(0, "^@Connected\n");

    }

};


extern "C" __declspec(dllexport) struct IXAC_EX* GetInterface(const char* version, int interfaceversion, ac_imports_t *arg_imp)
{
    CODECRC();
    if(interfaceversion != 1)
    {
        return NULL;
    }

    strncpy(build_string ,version, sizeof(build_string) -1);
    imp = *arg_imp;

    iXAC_EX.Initialize = IXAC::Initialize;
    iXAC_EX.ReturnVersion = IXAC::ReturnVersion;
    iXAC_EX.Terminate = IXAC::Terminate;
    iXAC_EX.EarlyInitialize = IXAC::EarlyInitialize;
    iXAC_EX.RunFrame = IXAC::RunFrame;
    iXAC_EX.NetDataCallback = IXAC::NetDataCallback;
    iXAC_EX.VerifyImage = IXAC::VerifyImage;
    iXAC_EX.ConnectToServer = IXAC::Connecting;
    downloader = NULL;

    return &iXAC_EX;
}

extern "C" __declspec(dllexport) unsigned int GetMasterVersion()
{
    return IXAC::ReturnVersion();
}



