#include "q_shared.h"
#include "sys_net.h"
#include "protocol.h"
#include "sys_main.h"
#include "net_secure.h"
#include <new>
#include <list>
#include <string.h>
#include <stdio.h>  
#include <stdlib.h> 
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif
/********************************************************************
Common
********************************************************************/

#define BUFFER_STARTSIZE 2048


class Buffer
{
public:
    Buffer();
    ~Buffer();
    void WriteInt64(uint64_t l);
    void WriteLong(uint32_t l);
    void WriteByte(uint8_t b);
    void WriteData(uint8_t* data, int len);
    void WriteString(const char* string);
    uint8_t ReadByte();
    uint32_t ReadLong();
    uint32_t GetLong();
    uint64_t GetInt64();
    uint64_t ReadInt64();
    int ReadData(uint8_t* data, int len); //Read data and move read pointer ahead
    void ReadString(char* outstring, int maxlen);
    int GetData(uint8_t* data, int len); //Read data but keep the read pointer
    uint32_t GetAvailableBytesCount();
    uint32_t GetReadPointer();
    void SetReadPointer(uint32_t rp);
    void AdvanceReadPointer(int len); //Moves the read pointer ahead by this amount of bytes
    unsigned int GetWriteCount();
    void UpdateLong(unsigned int pos, uint32_t);
    void TryReset();
    void BeginFrame();
    void FinalizeFrame();
    void SetSecurityContext(void* ctx)
    {
        security_ctx = ctx;
    }
    void* GetSecurityContext()
    {
        return security_ctx;
    }
    void BeginReadingMessage();
private:
    void Resize( );
    void Resize(unsigned int newlen);
    uint8_t* data;
    unsigned int readcount;
    unsigned int writecount;
    unsigned int buffersize;
    unsigned int lastframestart;
    void* security_ctx;
};

//Try to shrink buffer to its base size
void Buffer::TryReset()
{
    if(writecount == readcount)
    {
        writecount = 0;
        readcount = 0;
        lastframestart = 0;
        if(buffersize != BUFFER_STARTSIZE)
        {
            delete[] data;
            data = new uint8_t[BUFFER_STARTSIZE];
            buffersize = BUFFER_STARTSIZE;
        }
    }
}

void Buffer::Resize(unsigned int newlen)
{
    if(newlen < writecount - readcount)
    {
        return;
    }
    uint8_t* newdata = new uint8_t[newlen];

    memcpy(newdata, data + readcount, writecount - readcount);
    buffersize = newlen;
    delete[] data;
    data = newdata;
    writecount = writecount - readcount;
    if(lastframestart > readcount)
    {
        lastframestart -= readcount;
    }else{
        lastframestart = 0;
    }
    readcount = 0;

}

void Buffer::Resize( )
{
    Resize(2 * buffersize);
}

Buffer::Buffer()
{
    data = new uint8_t[BUFFER_STARTSIZE];
    readcount = 0;
    writecount = 0;
    lastframestart = 0;
    buffersize = BUFFER_STARTSIZE;
    security_ctx = NULL;
}

Buffer::~Buffer()
{
    delete data;
    readcount = 0;
    writecount = 0;
    buffersize = 0;
    lastframestart = 0;
}

uint32_t Buffer::GetAvailableBytesCount()
{
    return writecount - readcount;
}

uint64_t Buffer::GetInt64()
{
    uint64_t r;
    
    if(readcount + sizeof(r) > writecount)
    {
        //Exception?
        return -1;
    }

    uint8_t *p =  data + readcount;
    uint32_t lowbyte = *(uint32_t*)p;
    uint32_t highbyte = *(uint32_t*)(p + sizeof(lowbyte));
    uint64_t hlbyte = ntohl(lowbyte);
    uint64_t hhbyte = ntohl(highbyte);
    r = hlbyte | (hhbyte << 32);
    return r;
}

uint32_t Buffer::GetLong()
{
    uint32_t r;
    
    if(readcount + sizeof(r) > writecount)
    {
        //Exception?
        return -1;
    }

    uint8_t *p = data + readcount;
    r = *(uint32_t*)p;
    return ntohl(r);
}

uint64_t Buffer::ReadInt64()
{
    uint64_t r = GetInt64();
    AdvanceReadPointer(sizeof(r));
    return r;
}


uint32_t Buffer::ReadLong()
{
    uint32_t r = GetLong();
    AdvanceReadPointer(sizeof(r));
    return r;
}

uint8_t Buffer::ReadByte()
{
    uint8_t r;
    
    if(readcount > writecount + sizeof(r))
    {
        //Exception?
        return -1;
    }

    uint8_t *p =  data + readcount;
    r = *p;
    AdvanceReadPointer( sizeof(r));
    return r;
}

//Read data and move read pointer ahead
int Buffer::ReadData(uint8_t* d, int len)
{

    int rl = GetData(d, len);
    AdvanceReadPointer(rl);
    return rl;
}


void Buffer::ReadString(char* string, int maxlen)
{
    GetData((uint8_t*)string, maxlen);
    string[maxlen -1] = '\0';
    int sl = strlen(string) +1;
    AdvanceReadPointer(sl);
}


//Read data but keep the read pointer
int Buffer::GetData(uint8_t* d, int len)
{
    if((unsigned int)len > writecount - readcount)
    {
        len = writecount - readcount;
    }
    memcpy(d, data + readcount, len);
    return len;
}

//Moves the read pointer ahead by this amount of bytes
void Buffer::AdvanceReadPointer(int len)
{
    if((signed int)len == -1)
    {
        Com_PrintError("Error AdvanceReadPointer for -1 bytes\n", len);
        return;
    }
    if((unsigned int)len > writecount - readcount)
    {
        Com_PrintError("Error AdvanceReadPointer for %d bytes\n", len);
        return;
    }
    readcount += len;
}

void Buffer::WriteLong(uint32_t l)
{
    if(buffersize < writecount + sizeof(l))
    {
        Resize();
    }
    uint8_t *p =  data + writecount;
    *(uint32_t*)p = htonl(l);
    writecount += sizeof(l);
}

void Buffer::UpdateLong(unsigned int pos, uint32_t l)
{
    if(pos < 0 || pos > writecount)
    {
        return;
    }
    uint8_t *p =  data + pos;
    *(uint32_t*)p = htonl(l);
}


void Buffer::WriteInt64(uint64_t l)
{
    if(buffersize < writecount + sizeof(l))
    {
        Resize();
    }
    uint8_t *p =  data + writecount;

    uint32_t lowbyte = l & (uint64_t)0xFFFFFFFF;
    uint32_t highbyte = l >> (uint64_t)32;
    *(uint32_t*)p = htonl(lowbyte);
    *(uint32_t*)(p + sizeof(lowbyte)) = htonl(highbyte);
    writecount += sizeof(l);
}

void Buffer::WriteByte(uint8_t l)
{
    if(buffersize < writecount + sizeof(l))
    {
        Resize();
    }
    uint8_t *p =  data + writecount;
    *p = l;
    writecount += sizeof(l);
}

void Buffer::WriteData(uint8_t* idata, int len)
{
    if(buffersize < writecount + len)
    {
        if(2 * buffersize > writecount + len)
        {
            Resize();
        }else{
            Resize(buffersize + len);
        }
    }
    uint8_t *p =  data + writecount;
    memcpy(p, idata, len);
    writecount += len;
}

void Buffer::WriteString(const char* string)
{
    int len = strlen(string);
    WriteData((uint8_t*)string, len +1);
}

unsigned int Buffer::GetWriteCount()
{
    return writecount;
}


void Buffer::BeginFrame()
{
    if(lastframestart > 0)
    {
        FinalizeFrame();
    }
    lastframestart = writecount;
    WriteLong(0); //Frame length gets written by FinalizeFrame
}

void Buffer::FinalizeFrame()
{
    if(security_ctx) //This is an encrypted frame
    {
        if(buffersize < writecount + 16  + sizeof(uint32_t)) //Make space for the cipher padding
        {
            Resize();
        }
        int b = EncryptMessage(security_ctx, data + lastframestart + sizeof(uint32_t), writecount - lastframestart - sizeof(uint32_t), buffersize - lastframestart);
        writecount = b + lastframestart  + sizeof(uint32_t);
    }
    unsigned int oldwritecount = writecount;
    writecount = lastframestart;
    WriteLong(oldwritecount - lastframestart);
    writecount = oldwritecount;
    
    lastframestart = 0;
}

uint32_t Buffer::GetReadPointer()
{
    return readcount;
}
void Buffer::SetReadPointer(uint32_t rp)
{
    if(rp >= 0 && rp <= writecount)
    {
        readcount = rp;
    }
}

void Buffer::BeginReadingMessage()
{
    if(security_ctx == NULL)
    {
        return;
    }
    //Message decipher
    uint32_t size = GetLong();
    if((int32_t)size == -1)
    {
        return;
    }
    uint8_t *p =  data + readcount;

    int b = DecryptMessage(security_ctx, p + sizeof(uint32_t), size - sizeof(uint32_t));
    p[b + sizeof(uint32_t)] = 0;
}


class FileList
{
public:
    void Free();
    void Init();
    void Print();
    void PrintNeededFiles( );
    void CompareFiles(const char* dir);
    unsigned int numfiles;    //Number of all files
    unsigned int currentFile; //For client only: Current file the client downloads.
    unsigned int nextFile; //For client only: The next file the client should download. State here to remember what comes next
    struct FileData
    {
        const char* name;
        uint64_t filesize;
        uint64_t lastmodified;  //For server only to detect which file got modified
        uint8_t hash[32];
        bool needed;        //For client only. Will be true if this file is different from server and has to be downloaded
    };
    struct FileData* files;
};


void FileList::Free()
{
    unsigned int i;
    if(files == NULL)
    {
        return;
    }
    for(i = 0; i < numfiles; ++i)
    {
        if(files[i].name)
        {
            delete files[i].name;
        }
    }
    Z_Free(files);
    files = NULL;
}

void FileList::Print( )
{
    unsigned int i;
    char hstring[256];

    Com_Printf("FileList is:\n");

    for(i = 0; i < numfiles; ++i)
    {
        Blakesum2String(files[i].hash, hstring, sizeof(hstring));
        Com_Printf("File: %s, Checksum: %s, Size: %lld Needed: %d\n", files[i].name, hstring, files[i].filesize, (int)files[i].needed);
    }
    Com_Printf("%d files in directory\n", numfiles);
   
}

void FileList::PrintNeededFiles( )
{
    unsigned int i, n;
    uint64_t ts;
    for(i = 0, n = 0, ts = 0; i < numfiles; ++i)
    {
        if(files[i].needed)
        {
            Com_Printf("File: %s Size: %lld\n", files[i].name, files[i].filesize);
            ++n;
            ts += files[i].filesize;
        }
    }
    Com_Printf("%d files with total size of %lldMiB have to be downloaded\n", n, ts / (1024*1024));
}

void FileList::CompareFiles(const char* dir)
{
    unsigned int i;
    char filename[1024];
    char cdir[1024];
    uint8_t hash[32];

    while(*dir <= ' ' && *dir != '\0')
    {
        ++dir;
    }
    Q_strncpyz(cdir, dir, sizeof(cdir));
    FS_StripTrailingSeparator(cdir);


    for(i = 0; i < numfiles; ++i)
    {
        if(files[i].filesize == (uint64_t)-1)
        {
            files[i].needed = false;
            Com_PrintWarning("Received invalid fileinfo for file: %s\n", files[i].name);
            continue;   
        }

        filename[0] = '\0';
        if(cdir[0])
        {
            Com_sprintf(filename, sizeof(filename), "%s/%s", dir, files[i].name);
        }else{
            Com_sprintf(filename, sizeof(filename), "%s", files[i].name);
        }
        uint64_t size = FS_FileHash(filename, hash, sizeof(hash));
        if(size == (uint64_t)-1 || size != files[i].filesize)
        {
            files[i].needed = true;
            continue;
        }
        if(memcmp(hash, files[i].hash, sizeof(hash)) != 0)
        {
            files[i].needed = true;
            continue;   
        }
        files[i].needed = false;
    }


}

void FileList::Init( )
{
    int n;
    char **filestringlist = Sys_ListFiles( "", &n );
    
    if(n <= 0)
    {
        numfiles = 0;
        return;
    }
    numfiles = (unsigned int)n;

    files = (FileList::FileData*)Z_Malloc( sizeof(FileList::FileData) * numfiles);
    if(files == NULL)
    {
        numfiles = 0;
        return;
    }    
    unsigned int i;
    for(i = 0; i < numfiles; ++i)
    {
        files[i].name = filestringlist[i];
        FS_TurnSeparatorsForward( (char*)files[i].name ); //Turn all "\" into "/"
        files[i].filesize = FS_FileHash(files[i].name, files[i].hash, sizeof(files[i].hash));
        files[i].lastmodified = FS_GetFileModifiedTime(files[i].name);
    }

    Z_Free( filestringlist ); //Just release the string array but not the strings

}


int NET_TcpSendData( netadr_t*, unsigned char *data, int length, char* errormsg, int maxerrorlen );

/*******************************************************
Server
*******************************************************/

class Client
{
public:
    Client(netadr_t* foreignhost);
    ~Client();
    bool ParseAndExecuteMessage();
    void ExecuteMessage(CommandType t, uint32_t size);
    void DoHandshake();
    void ReadAndSendDirInfo();
    void SelectFile();
    void BeginSendingDownload();
    void SendCurrentDownload();
    void WriteFeature(Features f, uint32_t version);
    class Buffer *receivebuffer;
    class Buffer *sendbuffer;
    void SetupSecureChannel();
    void FinishSecureChannelSetup();
private:
    bool inuse;
    netadr_t foreignhost;
    char plaintextbuf[2048];
    int plaintextbufsize;
    const char* selectedfile;
    const char* selecteddir;
    FILE* openfile;
    int64_t fileoffset;
};


class Server
{
public:
    Client* AddClient(netadr_t* remote);
    void RemoveClient(Client* cl);
    bool InitServer(const char* privatekeyfile, const char* privatekeypassword, const char* certfiles);
    void BuildFileList(const char* dir, class Buffer *sendbuffer);
    int64_t GetFileinfo(const char* filename, uint8_t* outhash, int maxhash);
    void SendDownloads();
private:
    std::list<class Client*> clients;
    class FileList filelist;
};

class Server server;



Client::Client(netadr_t* adr)
{
    receivebuffer = new Buffer();
    sendbuffer = new Buffer();
    foreignhost = *adr;
    inuse = true;
    openfile = NULL;
    selecteddir = NULL;
    selectedfile = NULL;
    fileoffset = -1;
}

Client::~Client()
{
    delete receivebuffer;
    delete sendbuffer;
    memset(&foreignhost, 0, sizeof(foreignhost));
    inuse = false;
    if(selecteddir)
    {
        delete selecteddir;
    }
    if(selectedfile)
    {
        delete selectedfile;
    }
    selecteddir = NULL;
    selectedfile = NULL;
    FS_FCloseFileOSPath(openfile);
    openfile = NULL;
    fileoffset = -1;
}

void Client::WriteFeature(Features f, uint32_t version)
{
    sendbuffer->WriteLong((int)f); //Feature type
    sendbuffer->WriteLong(version); //Version of feature
    sendbuffer->WriteLong(-1); //Additional flags. -1 is end marker
}

void Client::DoHandshake()
{
    /* HELO response with server version */
    sendbuffer->BeginFrame();
    sendbuffer->WriteLong((int)CommandType::HELORESPONSE);
    sendbuffer->WriteLong(VERSION_MAJOR);
    sendbuffer->WriteLong(VERSION_MINOR);
    sendbuffer->FinalizeFrame();

    sendbuffer->BeginFrame();
    sendbuffer->WriteLong((int)CommandType::FEATURES);
    WriteFeature(Features::NOLOGIN, 1);
    WriteFeature(Features::LOGINPLAIN, 1);
    sendbuffer->WriteLong(-1); //3 times -1 is ending marker for features list
    sendbuffer->WriteLong(-1); //3 times -1 is ending marker for features list
    sendbuffer->WriteLong(-1); //3 times -1 is ending marker for features list
    sendbuffer->FinalizeFrame();
    
}


void Client::ReadAndSendDirInfo()
{
    char dir[1024];
    receivebuffer->ReadString(dir, sizeof(dir));
    
    Com_Printf("Read directory: %s\n", dir);

    sendbuffer->BeginFrame();
    
    sendbuffer->WriteLong((int)CommandType::DIRINFORESPONSE);

    if(strstr(dir, "..") || strstr(dir, "::")) //Never allow to go up the base dir
    {
        sendbuffer->WriteLong(0); //0 files
        sendbuffer->FinalizeFrame();
        return;
    }

    server.BuildFileList(dir, sendbuffer);
    
    sendbuffer->FinalizeFrame();

}

void Client::SelectFile()
{
    char file[1024], dir[1024];
    char path[1024];
    uint8_t hash[32];

    receivebuffer->ReadString(dir, sizeof(dir));
    receivebuffer->ReadString(file, sizeof(file));

    FS_StripLeadingSeparator(dir);
    FS_StripLeadingSeparator(file);
    FS_StripTrailingSeparator(dir);
    FS_StripTrailingSeparator(file);

    sendbuffer->BeginFrame();
    
    sendbuffer->WriteLong((int)CommandType::GETFILERESPONSE);

    if(strstr(dir, "..") || strstr(dir, "::") || strstr(file, "..") || strstr(file, "::")) //Never allow to go up the base dir
    {
        uint8_t nullbuf[32];
        memset(nullbuf, 0, sizeof(nullbuf));
        sendbuffer->WriteString(""); //empty name - error
        sendbuffer->WriteInt64(0);
        sendbuffer->WriteData(nullbuf, sizeof(nullbuf));
        sendbuffer->FinalizeFrame();
        return;
    }

    if(selecteddir)
    {
        delete[] selecteddir;
    }
    if(selectedfile)
    {
        delete[] selectedfile;
    }
    selecteddir = NULL;
    selectedfile = NULL;

    Com_sprintf(path, sizeof(path), "%s/%s", dir, file);
//    Com_Printf("Selected file: %s\n", path);

    int64_t len = server.GetFileinfo(path, hash, sizeof(hash));
    if(len < 0) //Error
    {
        uint8_t nullbuf[32];
        memset(nullbuf, 0, sizeof(nullbuf));
        sendbuffer->WriteString(""); //empty name - error
        sendbuffer->WriteInt64(0);
        sendbuffer->WriteData(nullbuf, sizeof(nullbuf));
        sendbuffer->FinalizeFrame();
        return;
    }

    FS_FCloseFileOSPath(openfile);
    openfile = NULL;

    int64_t reallen = FS_FOpenFileReadOSPath( path, &openfile );

    if(reallen != len) //Error
    {
        FS_FCloseFileOSPath(openfile);
        openfile = NULL;

        uint8_t nullbuf[32];
        memset(nullbuf, 0, sizeof(nullbuf));
        sendbuffer->WriteString(""); //empty name - error
        sendbuffer->WriteInt64(0);
        sendbuffer->WriteData(nullbuf, sizeof(nullbuf));
        sendbuffer->FinalizeFrame();
        return;
    }

    selecteddir = CopyString(dir);
    selectedfile = CopyString(file);

    sendbuffer->WriteString(selectedfile);
    sendbuffer->WriteInt64(len);
    sendbuffer->WriteData(hash, sizeof(hash));
    
    sendbuffer->FinalizeFrame();

}

void Client::BeginSendingDownload()
{
    receivebuffer->ReadInt64(); //Offset is ignored
    fileoffset = 0; //This does startup the file send-loop
}


void Client::SetupSecureChannel()
{
    unsigned char key[4096];
    unsigned char iv[16];


    Com_Printf("Server has to setup TLS...\n");

    NETSecureInstance* newcipherctx = new NETSecureInstance();

    int outkeylen = newcipherctx->DH_Setup(key, sizeof(key), iv);
    if(outkeylen == 0)
    {
        Com_Printf("Error generating cipher key. Should close conn?\n");
        return;
    }

    sendbuffer->BeginFrame();
    sendbuffer->WriteLong((int)CommandType::TLSSWITCH);
    sendbuffer->WriteData(iv, sizeof(iv));
    sendbuffer->WriteLong(outkeylen);
    sendbuffer->WriteData(key, outkeylen);
    sendbuffer->FinalizeFrame();
    
    sendbuffer->SetSecurityContext(newcipherctx);
}


void Client::FinishSecureChannelSetup()
{
    unsigned char foreign_iv[16];
    unsigned char foreign_key[4096];
    unsigned char signature[512];

    Com_Printf("Server has finish cipher...\n");


    receivebuffer->ReadData(foreign_iv, sizeof(foreign_iv));
    unsigned int foreignkeysize = receivebuffer->ReadLong();
    if(foreignkeysize > sizeof(foreign_key))
    {
        Com_Printf("Error: bad cipher key. Should close conn?\n");
        return; 
    }
    receivebuffer->ReadData(foreign_key, foreignkeysize);



    void* ctx = sendbuffer->GetSecurityContext();
    ((NETSecureInstance*)ctx)->DH_Finish(foreign_key, foreignkeysize, foreign_iv, signature);
    receivebuffer->SetSecurityContext(ctx);

}


void Client::ExecuteMessage(CommandType t, uint32_t size)
{
    switch(t)
    {
        case CommandType::HELO:
//            Com_Printf("Helo\n");
            DoHandshake();
            break;
        case CommandType::FEATURES:
//            Com_Printf("Features\n");
            break;
        case CommandType::STARTTLS:
            SetupSecureChannel();
            break;
        case CommandType::TLSSWITCH:
            FinishSecureChannelSetup();
            break;
        case CommandType::GETDIRINFO:
            ReadAndSendDirInfo();
            break;
        case CommandType::GETFILE:
            SelectFile();
            break;
        case CommandType::GETCHUNKS:
            BeginSendingDownload();
            break;
        default:
            Com_Printf("Unknown command %d\n", (unsigned int)t);
            break;
    }
}


bool Client::ParseAndExecuteMessage()
{
    uint32_t size = receivebuffer->GetLong();
    uint32_t avail = receivebuffer->GetAvailableBytesCount();
    if((int32_t)size < 8 || size > 1024*1024*1)
    {
        return false;
    }
    if(avail < size)
    {
        return false;
    }
    uint32_t rcount = receivebuffer->GetReadPointer();
    receivebuffer->BeginReadingMessage();
    size = receivebuffer->ReadLong();
    
    CommandType type = (CommandType)receivebuffer->ReadLong();

    ExecuteMessage(type, size);    
    receivebuffer->SetReadPointer(rcount + size);
    return true;
}

void Client::SendCurrentDownload()
{
    uint8_t filedata[512*1024];
    uint8_t buf[128*1024];

    uint32_t avail = sendbuffer->GetAvailableBytesCount();
    if(avail > 0)
    {
        int trysendlen = sendbuffer->GetData(buf, sizeof(buf));
        int sentlen = NET_TcpSendData(&foreignhost, buf, trysendlen, NULL, 0);
        if(sentlen > 0)
        {
            sendbuffer->AdvanceReadPointer(sentlen);
        }
        sendbuffer->TryReset();
        return;
    }
    if(openfile == NULL || fileoffset < 0)
    {
        return;
    }

    sendbuffer->BeginFrame();
    
    int readlen = FS_ReadOSPath(filedata, sizeof(filedata), openfile);

    if(readlen < 0)
    {
        sendbuffer->WriteLong((int)CommandType::GETCHUNKSRESPONSE);
        sendbuffer->WriteString("Error reading requested file\n");
        sendbuffer->FinalizeFrame();
        fileoffset = -1; //We are done with sending file because of error
        FS_FCloseFileOSPath(openfile);
        openfile = NULL;
    }

    sendbuffer->WriteLong((int)CommandType::GETCHUNKSRESPONSE);
    sendbuffer->WriteLong(1);
    sendbuffer->WriteLong(readlen);
    sendbuffer->WriteInt64(fileoffset);
    sendbuffer->WriteData(filedata, readlen);
    sendbuffer->FinalizeFrame();

    fileoffset += readlen;
    if(readlen == 0)
    {
        fileoffset = -1; //We are done with sending file
        FS_FCloseFileOSPath(openfile);
        openfile = NULL;
    }
}


int64_t Server::GetFileinfo(const char* filename, uint8_t *outhash, int maxhash)
{
    unsigned int i;

    for(i = 0; i < filelist.numfiles; ++i)
    {
        if(strcmp(filename, filelist.files[i].name) == 0)
        {
            break;
        }
    }

    if(i == server.filelist.numfiles) //Error
    {
        return -1;
    }
    int len = sizeof(filelist.files[i].hash);
    if(len > maxhash)
    {
        len = maxhash;
    }
    memcpy(outhash, filelist.files[i].hash, len);
    return filelist.files[i].filesize;
}


void Server::BuildFileList(const char* dir, class Buffer *sendbuffer)
{
    char bdir[1024];

    while(*dir == '\\' || *dir == '/' || *dir == ' ')
    {
        ++dir;
    }
    unsigned int i, c;
    unsigned int filecountstreampos = sendbuffer->GetWriteCount();
    sendbuffer->WriteLong(0); //Write 0 files and update it later

    Q_strncpyz(bdir, dir, sizeof(bdir) -1);
    FS_TurnSeparatorsForward(bdir);

    /* String trailing path separators */
   	FS_StripTrailingSeparator(bdir);
    int dirlen = strlen(bdir);

    if(dirlen < 1)
    {
        return;
    }

    bdir[dirlen] = '/';
    ++dirlen;
    
    for(i = 0, c = 0; i < filelist.numfiles; ++i)
    {
        if(strncmp(filelist.files[i].name, bdir, dirlen) != 0)
        {
            continue;
        }
        sendbuffer->WriteString(filelist.files[i].name + dirlen);
        sendbuffer->WriteInt64(filelist.files[i].filesize);
        sendbuffer->WriteData(filelist.files[i].hash, sizeof(filelist.files[i].hash));
        ++c;
   }
   sendbuffer->UpdateLong(filecountstreampos , c);
}

Client* Server::AddClient(netadr_t* remote)
{
    class Client* cl;

    cl = new Client(remote);
    //New clients
    clients.push_front(cl);
    return cl;
}

void Server::RemoveClient(Client* cl)
{
    clients.remove(cl);
}

bool Server::InitServer(const char* privatekeyfile, const char* privatekeypassword, const char* certfiles)
{
    NET_TcpServerInit(privatekeyfile, privatekeypassword, certfiles);
    filelist.Init();
    return true;
}


void NET_TCPConnectionClosed(netadr_t* adr, uint64_t connectionId, uint64_t serviceId)
{
    server.RemoveClient((Client*)connectionId);
}


int NET_TcpSendData( netadr_t* hremote, unsigned char *data, int length, char* errormsg, int maxerrorlen ) {
    return _NET_TcpSendData( hremote->sock, data, length, errormsg, maxerrorlen );

/*    if(hremote->unsentbytes > 0)
    {
        int s = _NET_TcpSendData( hremote->sock, hremote->unsentbuffer, hremote->unsentbytes, errormsg, maxerrorlen );
        if(s <= 0)
        {
            return s;
        }
        if(s < hremote->unsentbytes)
        {
            memmove(hremote->unsentbuffer, hremote->unsentbuffer + s, hremote->unsentbytes - s);
            hremote->unsentbytes -= s;
        }else{
	    hremote->unsentbytes = 0;
	}
        if(hremote->unsentbytes > 0)
        {
            return 0;
        }
    }
    int i;

    for(i = 0; i < length; i += sizeof(hremote->unsentbuffer))
    {
        int l = length -i;
        if(l > (signed)sizeof(hremote->unsentbuffer))
        {
            l = sizeof(hremote->unsentbuffer);
        }
        int s = _NET_TcpSendData( hremote->sock, data + i, l, errormsg, maxerrorlen );

        if(s < l)
        {
            int nl = l;
            if(s > 0)
            {
                nl = l - s;
            }
            memcpy(hremote->unsentbuffer, data, nl);
            hremote->unsentbytes = nl;
        Com_Printf("Fragment %d\n", nl);

            if(s >= 0)
            {
                return i + hremote->unsentbytes;
            }
            return s;
        }
    }
    return length;
*/
}


qboolean NET_TCPPacketEvent(netadr_t* remote, byte* bufData, int cursize, uint64_t *connectionId, uint64_t *serviceId)
{
    class Client* cl;
    uint8_t buf[4096];

    if(*connectionId == 0)
    {
        cl = server.AddClient(remote);
        *connectionId = (uint64_t)cl;
    }else{
        cl = (Client*)*connectionId;
    }



    cl->receivebuffer->WriteData(bufData, cursize);

    if(cursize > 0)
    {
        try
        {
            while(cl->ParseAndExecuteMessage());
        }catch(int code){
            //Deinitialize;
            return qtrue;
        }
    }

    int trysendlen = cl->sendbuffer->GetData(buf, sizeof(buf));

    int sentlen = NET_TcpSendData(remote, buf, trysendlen, NULL, 0);
    if(sentlen > 0)
    {
        cl->sendbuffer->AdvanceReadPointer(sentlen);
    }

    cl->sendbuffer->TryReset();
    cl->receivebuffer->TryReset();

    return qfalse;
}

std::list<class Client*> clients;


void Server::SendDownloads()
{
    for (std::list<class Client*>::iterator i=clients.begin(); i != clients.end(); ++i)
    {
        (*i)->SendCurrentDownload();
    }
}

void RunServer()
{
    Com_Printf("Server is ready!\n");

    while(1)
    {
        NET_Sleep(10000);
        NET_TcpServerPacketEventLoop();
        server.SendDownloads();
    }
}


/********************************************************************
Client
********************************************************************/


class LocalClient
{
public:
    int InitClient(const char* foreignaddr, const char* dir);
    bool ClientFrame();
    void DoHandshake();
    void SendReceive();
    bool ParseAndExecuteMessage();
    void ParseFeatures();
    void ParseFileList();
    void ParseGetFileResponse();
    void ParseServerError();
    void ParseNextChunk();
    void DownloadFiles();
    void DownloadNextFile();
    void DownloadRequestChunks();
    void SetNextState();
    void RetriveDirectoryInfo();
    void ExecuteMessage(CommandType t, uint32_t size);
    void Shutdown();
    void Restart();
    void SetupSecureChannel();
    class Buffer *receivebuffer;
    class Buffer *sendbuffer;

private:
    enum
    {
        UNINITIALIZED,
        HANDSHAKE_HELOSENT,
        HANDSHAKE_GOTHELO,
        HANDSHAKE_GOTFEATURES,
        CMD_RETRIVEDIRINFO,
        CMD_DIRINFOREQUESTSENT,
        CMD_DIRINFOPARSED,
        COMPFILES,
        FILECOMPDONE,
        DOWNLOADFILES,
        DOWNLOADNEXTFILE,
        DOWNLOADWAITRESPONSE, 
        DOWNLOADREQUESTFIRSTCHUNK,
        DOWNLOADRECEIVE,
        DONE
    }state;

    netadr_t foreignhost;
    struct
    {
        uint32_t version;
        uint32_t options;
    }features[(uint32_t)Features::MAXFEATURES];
    char commonname[256];
    class FileList filelist;
    const char* directory;
    FILE* openfile;
};

class LocalClient localclient;


void LocalClient::SendReceive( )
{
    uint8_t buf[1024*1024];

    int rlen = NET_TcpClientGetData(foreignhost.sock, buf, sizeof(buf), NULL, 0);
    if(rlen > 0)
    {
        receivebuffer->WriteData(buf, rlen);
        while(ParseAndExecuteMessage());
    }else if(rlen != NET_WANT_READ){
        if(rlen == 0){
            Com_Printf("Connection should close\n");
        }else if(rlen == NET_CONNRESET){
            Com_Printf("Connection reset\n");
        }else if(rlen == NET_ERROR){
            Com_Printf("Network error\n");
        }
    }
    int trysendlen = sendbuffer->GetData(buf, sizeof(buf));

    int sentlen = NET_TcpSendData(&foreignhost, buf, trysendlen, NULL, 0);
    if(sentlen > 0)
    {
        sendbuffer->AdvanceReadPointer(sentlen);
    }

    sendbuffer->TryReset();
    receivebuffer->TryReset();
    if(rlen == NET_CONNRESET)
    {
        throw 0x1001;
    }else if(rlen == NET_ERROR){
        throw 0x1000;
    }
}

void LocalClient::SetupSecureChannel()
{
    unsigned char key[4096];
    unsigned char iv[16];
    unsigned char foreign_iv[16];
    unsigned char foreign_key[4096];

    Com_Printf("Server has to setup TLS...\n");

    NETSecureInstance* newcipherctx = new NETSecureInstance();

    int outkeylen = newcipherctx->DH_Setup(key, sizeof(key), iv);
    if(outkeylen == 0)
    {
        Com_Printf("Error generating cipher key. Should close conn?\n");
        return;
    }

    receivebuffer->ReadData(foreign_iv, sizeof(foreign_iv));
    unsigned int foreignkeysize = receivebuffer->ReadLong();
    if(foreignkeysize > sizeof(foreign_key))
    {
        Com_Printf("Error: bad cipher key. Should close conn?\n");
        return; 
    }
    receivebuffer->ReadData(foreign_key, foreignkeysize);
    
    sendbuffer->BeginFrame();
    sendbuffer->WriteLong((int)CommandType::TLSSWITCH);
    sendbuffer->WriteData(iv, sizeof(iv));
    sendbuffer->WriteLong(outkeylen);
    sendbuffer->WriteData(key, outkeylen);
    sendbuffer->FinalizeFrame();

    newcipherctx->DH_Finish(foreign_key, foreignkeysize, foreign_iv, NULL);
    sendbuffer->SetSecurityContext(newcipherctx);
    receivebuffer->SetSecurityContext(newcipherctx);

}


void LocalClient::ParseFileList()
{
    char filename[1024];
    unsigned int i;

    filelist.Free();

    filelist.numfiles = receivebuffer->ReadLong();
    filelist.files = (FileList::FileData*)Z_Malloc( sizeof(FileList::FileData) * filelist.numfiles);

    for(i = 0; i < filelist.numfiles; ++i)
    {
        receivebuffer->ReadString(filename, sizeof(filename));
        filelist.files[i].name = CopyString(filename);
        filelist.files[i].filesize = receivebuffer->ReadInt64();
        receivebuffer->ReadData(filelist.files[i].hash, sizeof(filelist.files[i].hash));
        filelist.files[i].lastmodified = 0;
        filelist.files[i].needed = false;
    }
    state = CMD_DIRINFOPARSED;
}

bool LocalClient::ParseAndExecuteMessage()
{
    uint32_t size = receivebuffer->GetLong();
    uint32_t avail = receivebuffer->GetAvailableBytesCount();

    if((int32_t)size < 8 || size > 1024*1024*1)
    {
        return false;
    }
    if(avail < size)
    {
        return false;
    }
    uint32_t rcount = receivebuffer->GetReadPointer();
    receivebuffer->BeginReadingMessage();
    size = receivebuffer->ReadLong();

    CommandType type = (CommandType)receivebuffer->ReadLong();

    ExecuteMessage(type, size);    

    receivebuffer->SetReadPointer(rcount + size);
    return true;
}


void LocalClient::ParseFeatures()
{
    Features feature;
    uint32_t version;
    uint32_t options;

    state = HANDSHAKE_GOTFEATURES;

    while( (feature = (Features)(receivebuffer->ReadLong())) != Features::FEOF)
    {
        version = receivebuffer->ReadLong();
        options = receivebuffer->ReadLong();
        if(options != (uint32_t)-1)
        {
            while(receivebuffer->ReadLong() != (uint32_t)-1);
        }

        if((uint32_t)feature >= (uint32_t)Features::MAXFEATURES || (uint32_t)feature < 0)
        {
            return;
        }
        features[(uint32_t)feature].version = version;
        features[(uint32_t)feature].options = options;
    }
}
void LocalClient::ParseGetFileResponse()
{
    char filename[1024];
    uint8_t hash[32];

    receivebuffer->ReadString(filename, sizeof(filename));
    receivebuffer->ReadInt64(); //filesize
    receivebuffer->ReadData(hash, sizeof(hash));

    if(filename[0] == 0)
    {
        Com_PrintError("Error on serverside opening or reading file\n");
        state = DONE;
        return;
    }
    if(strcmp(filelist.files[filelist.currentFile].name, filename) != 0)
    {
        Com_PrintError("Server selected wrong file\n");
        state = DONE;
        return;
    }

    FS_FCloseFileOSPath(openfile);

    openfile = FS_FOpenFileWriteOSPath(filename);
    if(!openfile)
    {
        Com_Error("Couldn't open file %s for writing\n", filename);
    }
    state = DOWNLOADREQUESTFIRSTCHUNK;
    Com_Printf("Selected file %s for downloading\n", filename);

}

void LocalClient::ParseNextChunk()
{
    uint8_t buf[1024*64];

    if(state != DOWNLOADRECEIVE)
    {
        return;
    }

    if(!openfile)
    {
        Com_Error("File is not open although it should be\n");
    }

    uint32_t chead = receivebuffer->ReadLong();
    if(chead == 0)
    {
        Restart();
        return;
    }
    if(chead != 1)
    {
        FS_FCloseFileOSPath(openfile);
        return;
    }
    uint32_t chunksize = receivebuffer->ReadLong();
    int64_t offset = receivebuffer->ReadInt64();

//    Com_Printf("Received Chunk of size %d at offset %lld\n", chunksize, offset);

    if(chunksize == 0)
    {
        state = DOWNLOADNEXTFILE;
        if(offset == 0)
        {
            //ToDo just touch file. Isn't it already happening?

        }
        FS_FCloseFileOSPath(openfile);
        return;
    }

    unsigned int readlen;

    while(chunksize > 0)
    {
        readlen = chunksize;
        if(readlen > sizeof(buf))
        {
            readlen = sizeof(buf);
        }
        receivebuffer->ReadData(buf, readlen);

        FS_WriteOSPath(buf, readlen, openfile);

        chunksize -= readlen;
    }
}

void LocalClient::ParseServerError()
{
    char errorline[1024];
    receivebuffer->ReadString(errorline, sizeof(errorline));
    Com_PrintError("Server: %s\n", errorline);
    throw(0x40);
}

void LocalClient::ExecuteMessage(CommandType t, uint32_t size)
{
    switch(t)
    {
        case CommandType::HELORESPONSE:
//            Com_Printf("Helo Response\n");
            state = HANDSHAKE_GOTHELO;
            break;
        case CommandType::FEATURES:
            ParseFeatures();
//            Com_Printf("Features\n");
            break;
        case CommandType::DIRINFORESPONSE:
            ParseFileList();
            break;
        case CommandType::GETFILERESPONSE:
            ParseGetFileResponse();
            break;        
        case CommandType::GETCHUNKSRESPONSE:
            ParseNextChunk();
            break;
        case CommandType::SERVERERROR:
            ParseServerError();
            break;
        case CommandType::TLSSWITCH:
            SetupSecureChannel();
            break;
        default:
            Com_Printf("Unknown command %d\n", (unsigned int)t);
            break;
    }
}



void LocalClient::SetNextState()
{
    switch(state)
    {
        case HANDSHAKE_GOTFEATURES:
            state = CMD_RETRIVEDIRINFO;
            break;
        case CMD_DIRINFOPARSED:
            state = COMPFILES;
            break;
        case FILECOMPDONE:
            state = DOWNLOADFILES;
            break;
        default:
            break;
    }
}


void LocalClient::DoHandshake()
{
    sendbuffer->BeginFrame();
    sendbuffer->WriteLong((int)CommandType::HELO);
    sendbuffer->WriteLong(VERSION_MAJOR);
    sendbuffer->WriteLong(VERSION_MINOR);
    sendbuffer->FinalizeFrame();

    sendbuffer->BeginFrame();
    sendbuffer->WriteLong((int)CommandType::STARTTLS);
    sendbuffer->FinalizeFrame();

    state = HANDSHAKE_HELOSENT;
}

void LocalClient::RetriveDirectoryInfo()
{
    sendbuffer->BeginFrame();
    sendbuffer->WriteLong((int)CommandType::GETDIRINFO);
    sendbuffer->WriteString(directory);
    sendbuffer->FinalizeFrame();
    state = CMD_DIRINFOREQUESTSENT;
}

void LocalClient::DownloadNextFile()
{
    const char *dir = directory;
    unsigned int i;

    for(i = filelist.nextFile; i < filelist.numfiles && !filelist.files[i].needed; ++i);

    if(i == filelist.numfiles)
    {
        Com_Printf("All files downloaded\n");
        state = DONE;
        return;
    }

    sendbuffer->BeginFrame();
    sendbuffer->WriteLong((int)CommandType::GETFILE);
    sendbuffer->WriteString(dir);
    sendbuffer->WriteString(filelist.files[i].name);
    sendbuffer->FinalizeFrame();

    state = DOWNLOADWAITRESPONSE;
    filelist.nextFile = i + 1;
    filelist.currentFile = i;
}

void LocalClient::DownloadRequestChunks()
{
    sendbuffer->BeginFrame();
    sendbuffer->WriteLong((int)CommandType::GETCHUNKS);
    sendbuffer->WriteInt64(0);
    sendbuffer->FinalizeFrame();
    state = DOWNLOADRECEIVE;
}

void LocalClient::DownloadFiles()
{
    Com_Printf("Have to download the following files from remote server:\n");
    filelist.PrintNeededFiles();
    Com_Printf("\n");
    state = DOWNLOADNEXTFILE;
}


bool LocalClient::ClientFrame()
{
    struct timeval tv;
    fd_set fs;
    if(foreignhost.sock != -1)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;


        FD_ZERO(&fs);
        FD_SET(foreignhost.sock, &fs);

        select(foreignhost.sock +1, &fs, NULL, NULL, &tv);
    }else{
        Sys_SleepSec(1);
    }
    try
    {
        switch(state)
        {
            case UNINITIALIZED:
                DoHandshake();
                break;
            case HANDSHAKE_HELOSENT:
                break;
            case HANDSHAKE_GOTHELO:
                break;
            case HANDSHAKE_GOTFEATURES:
                Com_Printf("Got Features\n");
                SetNextState();
                break;
            case CMD_DIRINFOPARSED:
                SetNextState();
                break;        
            case COMPFILES:
                filelist.CompareFiles("");
                state = FILECOMPDONE;
                break;
            case FILECOMPDONE:
                SetNextState();
                break;
            case DOWNLOADFILES:
                DownloadFiles();
                break;
            case DOWNLOADNEXTFILE:
                DownloadNextFile();
                break;
            case DOWNLOADREQUESTFIRSTCHUNK:
                DownloadRequestChunks();
                break;
            case DOWNLOADRECEIVE:
            case DOWNLOADWAITRESPONSE:
                break;
            case CMD_RETRIVEDIRINFO:
                RetriveDirectoryInfo();
                break;
            case CMD_DIRINFOREQUESTSENT:
                break;
            case DONE:
                throw(0);
                break;
        }
        SendReceive();
    }
    catch(int code)
    {
        return false;
    }
    return true;
}

void LocalClient::Restart()
{
    filelist.Free();
    FS_FCloseFileOSPath(openfile);
    openfile = NULL;
    state = CMD_RETRIVEDIRINFO;
}

void LocalClient::Shutdown()
{
    filelist.Free();
    FS_FCloseFileOSPath(openfile);
    openfile = NULL;
    if(sendbuffer)
    {
        delete sendbuffer;
    }
    if(receivebuffer)
    {
        delete receivebuffer;
    }
    sendbuffer = receivebuffer = NULL;
    if(directory)
    {
        delete directory;
    }
    directory = NULL;
    NET_TcpCloseSocket(foreignhost.sock);
    foreignhost.sock = -1;
    state = UNINITIALIZED;

}

int LocalClient::InitClient(const char* foreignaddr, const char* dir)
{
    int tries = 0;
    int clsock;

    memset(&foreignhost, 0, sizeof(foreignhost));

    foreignhost.sock = -1;
    directory = CopyString(dir);

    Q_strncpyz(commonname, foreignaddr, sizeof(commonname));

    do{
        if(NET_StringToAdr(foreignaddr, &foreignhost, NA_UNSPEC))
        {
            Com_Printf("Resolved %s to %s\n", foreignaddr, NET_AdrToString(&foreignhost));
            clsock = NET_TcpClientConnectToAdr(&foreignhost);
        }else{
            clsock = -1;
        }
        if(clsock == -1)
        {
            tries++;
            Sys_SleepSec(5);
            continue;
        }
    }while(clsock == -1 && tries < 5);

    if(clsock != -1)
    {
        sendbuffer = new Buffer();
        receivebuffer = new Buffer();        
        foreignhost.sock = clsock;
    }
    return clsock;

}

void RunClient(const char* foreignaddr, const char* dir)
{

    int clsock = localclient.InitClient(foreignaddr, dir);

    if(clsock != -1)
    {
        while(localclient.ClientFrame());
    }

    localclient.Shutdown();

}


void printOptions()
{
    Com_Printf("Client: No remote address given\n");
    Com_Printf("Usage:\n");
    Com_Printf("filesync -connect 1.2.3.4 -directory example\n");
    Com_Printf("filesync -server\n");
    Com_Printf("filesync -server -pkey <private key file> -cert <certificate full chain file>\n");
    Com_Printf("filesync -server -pkey <private key file> -pkeypw <private key password> -cert <certificate full chain file>\n");
}







int main(int count, const char** args)
{
    int i;
    const char* arg;
    char foreignaddr[256];
    char dir[1024];
    char keyfile[1024];
    char keyfilepassword[1024];
    char certfile[4096];
    *foreignaddr = '\0';
    bool isserver = false;

    bool parseaddr = false;
    bool parsekeyfile = false;
    bool parsecertfile = false;
    bool parsekeyfilepw = false;
    bool parsedir = false;
    certfile[0] = '\0';
    keyfile[0] = '\0';
    keyfilepassword[0] = '\0';
 
    for(i = 0; i < count; ++i)
    {
        arg = args[i];
        if(arg[0] != '-')
        {
            if(parseaddr)
            {
                Q_strncpyz(foreignaddr, arg, sizeof(foreignaddr));
                parseaddr = false;
                continue;
            }
            if(parsedir)
            {
                Q_strncpyz(dir, arg, sizeof(dir));
                parsedir = false;
                continue;   
            }
            if(parsekeyfile)
            {
                Q_strncpyz(keyfile, arg, sizeof(keyfile));
                parsekeyfile = false;
                continue;
            }
            if(parsekeyfilepw)
            {
                Q_strncpyz(keyfilepassword, arg, sizeof(keyfilepassword));
                parsekeyfilepw = false;
                continue;
            }            
            if(parsecertfile)
            {
                Q_strcat(certfile, sizeof(certfile), arg);
                Q_strcat(certfile, sizeof(certfile), "\n");
            }
            continue;
        }
        parsecertfile = false;
        if(_stricmp(arg + 1, "server") == 0)
        {
            isserver = true;
            continue;
        }
        if(_stricmp(arg +1, "connect") == 0)
        {
            parseaddr = true;
            continue;
        }
        if(_stricmp(arg +1, "directory") == 0 || _stricmp(arg +1, "dir") == 0)
        {
            parsedir = true;
            continue;
        }
        if(_stricmp(arg +1, "pkey") == 0 || _stricmp(arg +1, "privatekey") == 0)
        {
            parsekeyfile = true;
            continue;
        }
        if(_stricmp(arg +1, "pkeypw") == 0 || _stricmp(arg +1, "privatekeypw") == 0)
        {
            parsekeyfilepw = true;
            continue;
        }       
        if(_stricmp(arg +1, "cert") == 0 || _stricmp(arg +1, "certificate") == 0)
        {
            parsecertfile = true;
            continue;        
        }
        Com_Printf("Invalid commandline argument %s\n", arg);
        return 1;        
    }
    if(isserver)
    {
        Com_Printf("Starting server...\n");
        server.InitServer(keyfile, keyfilepassword, certfile);
    }else{
        Com_Printf("Starting client...\n");
        if(foreignaddr[0] == '\0' || dir[0] == '\0')
        {
            printOptions();
            return 1;
        }
    }

    NET_Init((qboolean)isserver, PORT_SERVER);
    if(isserver)
    {
        RunServer();
    }else{
        RunClient(foreignaddr, dir);
    }
    NET_Shutdown( );

    return 0;
}


