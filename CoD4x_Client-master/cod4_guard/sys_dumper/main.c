#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include <psapi.h>
#include <assert.h>

#include "rc5_32.h"
#include "blake/blake2.h"
#include <time.h>

HMODULE hm;
HMODULE msvcrtmod;
void AcHlp_Removal(const char* lpFilename);
int ACHlp_LoadFile(unsigned char **data, const char* filename);
void AcHlp_GetSectionBaseLen(unsigned char* imagestart, char* sectionname, bool virtual, DWORD* sectiondata, DWORD* sectionlen, DWORD* virtsectionlen);
bool AcHlp_WriteChecksums(FILE* fh);

struct modinfo_s
{
    DWORD start;
    DWORD len;
};


struct modinfo_s msvcrt;
struct modinfo_s cod4x_xxx;
char cod4x_modname[32];
static char mod_version[128];
struct rc5_32_key cryptokey;




__declspec(dllexport) void HlStart(const char* version)
{
    int i;
    unsigned char plaintextkey[16];

    for(i = 17; i < 99; ++i)
    {
        sprintf(cod4x_modname, "cod4x_0%d.dll", i);
        hm = GetModuleHandleA(cod4x_modname);
        if(hm)
        {
            break;
        }
    }
    if(i == 99)
    {
        MessageBoxA(NULL, "Couldn't find module cod4x_0xx.dll", "Couldn't find module", MB_OK);
        exit(-1);
    }

    msvcrtmod = GetModuleHandleA("msvcr100.dll");

    if(msvcrtmod == NULL)
    {
        MessageBoxA(NULL, "Error getting handle to msvcr100", "Couldn't find module", MB_OK);
        //exit(-1);      
    }

    MODULEINFO clientDllInfo = {0};

    GetModuleInformation(GetCurrentProcess(), hm, &clientDllInfo, sizeof(clientDllInfo));

    cod4x_xxx.start = (DWORD)clientDllInfo.lpBaseOfDll;
    cod4x_xxx.len = clientDllInfo.SizeOfImage;

    MODULEINFO msvcrtDllInfo = {0};
    GetModuleInformation(GetCurrentProcess(), msvcrtmod, &msvcrtDllInfo, sizeof(msvcrtDllInfo));

    msvcrt.start = (DWORD)msvcrtDllInfo.lpBaseOfDll;
    msvcrt.len = msvcrtDllInfo.SizeOfImage;

    strncpy(mod_version, version, sizeof(mod_version));

    srand(GetTickCount());

    for(i = 0; i < sizeof(plaintextkey); ++i)
    {
        plaintextkey[i] = rand();
    }
    rc5_setup(plaintextkey, sizeof(plaintextkey), 16, &cryptokey);
}

void AcHlpWriteSymbols();
void AcHlpWritePatchedModule();

__declspec(dllexport) void HlFini()
{
    char path[1024];

    MessageBoxA(NULL, "Anticheat Helper finished and has written all AC-data out. You have to feed this data into the anticheat control server", "Anticheat helper finished", MB_OK);
    GetModuleFileNameA(hm, path, sizeof(path));
    AcHlp_Removal(path);

    AcHlpWriteSymbols();
    exit(0);
}

struct SymbolPair_s
{
    DWORD addr;
    DWORD symbol;
    bool relative;
    const char* module;
};


static struct SymbolPair_s symbollist[10000];
static int numsymbols = 0;


void AcHlpAddSymbol(DWORD addr, DWORD symbol, const char* module, bool relative)
{
    if(module == NULL)
    {
        char er[1024];
        sprintf(er,"Error symbol from unknown module. 0x%x\n", (unsigned int)symbol);
        MessageBoxA(NULL, er, "Module Error", MB_OK);
        exit(4);       
    }
    if(numsymbols >= 10000)
    {
        MessageBoxA(NULL, "Error Max Symbols", "Error Max Symbols", MB_OK);
        exit(4);
    }
    symbollist[numsymbols].addr = addr;
    symbollist[numsymbols].symbol = symbol;// - (DWORD)hm;
    symbollist[numsymbols].module = module;
    symbollist[numsymbols].relative = relative;
    ++numsymbols;
}



__declspec(dllexport) void HlGet(DWORD addr, DWORD symbol, bool relative)
{
    const char* module = NULL;
    if(addr < 0x401000 || addr > 0xD934000) //error if not in .rdata, not in .data and not in .text
    {
        /*
        if(addr >= 0x71B000 && addr < 0xD934000) //ignore .data
        {
            return;
        }
        */
        char strerr[1024];
        sprintf(strerr, "HlGet failed!\n Bad addr 0x%x!", (unsigned int)addr);
        MessageBoxA(NULL, strerr, "HlGet failed", MB_OK);
		exit(0);
    }
    addr -= 0x400000; //relative to the imagebase

    if(symbol >= 0x401000 && symbol < 0xD934000)
    {
        return; //This address range can never shift. it is iw3mp.exe
    }
    if(symbol >= msvcrt.start && symbol < msvcrt.start + msvcrt.len)
    {
        module = "msvcr100.dll";
        symbol = symbol - msvcrt.start;
    }
    if(symbol >= cod4x_xxx.start && symbol < cod4x_xxx.start + cod4x_xxx.len)
    {
        module = cod4x_modname;
        symbol = symbol - cod4x_xxx.start;
    }
    AcHlpAddSymbol(addr, symbol, module, relative);
}

//structure not used, just info
struct XAC_GameOffsetData
{
    int moduleindex;
    unsigned int address;
    unsigned int symbol;
    bool relative;
};

struct XAC_GameBinData
{
    const char* version;
    const char* key;
    struct XAC_GameOffsetData offsets[];
};

bool AcHlpIsSymbol(DWORD offset)
{
    int i;
    for(i = 0; i < numsymbols; ++i)
    {
        if(symbollist[i].addr == offset)
        {
            return true;
        }
    }
    return false;
}

int AcHlpCountRunLength(const unsigned char* data, const unsigned char* orig, int maxlen)
{
    int i;
    unsigned char startval = data[0];

    for(i = 0; i < maxlen; ++i)
    {
        if(startval != data[i])
        {
            return i;
        }
    }
    return 0;
}

int AcHlpCountModifiedLength(const unsigned char* data, const unsigned char* orig, int offset, int maxlen)
{
    int i;
    for(i = 0; i < maxlen; ++i)
    {
        if(AcHlpIsSymbol(offset + i + 0x1000))
        {
            return i;
        }
        if(data[i] == orig[i] && data[i+1] == orig[i+1] && data[i+2] == orig[i+2] && data[i+3] == orig[i+3])
        {
            return i;
        }
        if(AcHlpCountRunLength(data + i, orig + i, maxlen - i) > 8)
        {
            return i;
        }
    }
    return 0;
}


void AcHlpWriteStaticChanges(FILE* fh)
{
    int i, y;
    int len = 0x691000 - 0x401000;
    unsigned char* diffimg = (unsigned char*)0x401000;
    unsigned char* orgdatafile;
    char path[1024];
    DWORD dwSectionSize, dwVirtSectionSize;
	DWORD dwSectionBase;

    GetModuleFileNameA(NULL, path, sizeof(path));

    int datafilelen = ACHlp_LoadFile(&orgdatafile, path);
    if(datafilelen != 3330048)
    {
		MessageBoxA(NULL, "AcHlpWriteStaticChanges failed to read delta datafile of correct size", "AcHlpWriteStaticChanges failed", MB_OK);
		exit(0);
    }
    AcHlp_GetSectionBaseLen(orgdatafile, ".text", false, &dwSectionBase, &dwSectionSize, &dwVirtSectionSize);
    if(dwSectionSize != len)
    {
		MessageBoxA(NULL, "AcHlpWriteStaticChanges section size mismatch in datafile compared to real image", "AcHlpWriteStaticChanges failed", MB_OK);
		exit(0);        
    }
    unsigned char* orgimg = (unsigned char*)dwSectionBase;
    fprintf(fh, "static unsigned char xac_staticmoddata[] = {\n");

    for(i = 0; i < len;)
    {
        if(orgimg[i] != diffimg[i])
        {
            if(AcHlpIsSymbol(i + 0x1000))
            {
                i += 4;
                continue;
            }
            int addr = rc5_ecb_encrypt(i, &cryptokey);
            const unsigned char* caddr = (unsigned char*)&addr;
            fprintf(fh, "\t0x%x, 0x%x, 0x%x, 0x%x, ", caddr[0], caddr[1], caddr[2], caddr[3]);
            unsigned int rl = AcHlpCountRunLength(diffimg + i, orgimg + i, len - i);
            if(rl > 7)
            {
                unsigned short srl = rl;
                const unsigned char* crl = (unsigned char*)&srl;
                fprintf(fh, "'r', 0x%x, 0x%x, 0x%x,\n", crl[0], crl[1], diffimg[i]);
                i += rl;
                continue;
            }
            unsigned int dl = AcHlpCountModifiedLength(diffimg + i, orgimg + i, i, len - i);
            unsigned short sdl = dl;
            const unsigned char* cdl = (unsigned char*)&sdl;
            fprintf(fh, "'d', 0x%x, 0x%x, ", cdl[0], cdl[1]);
            for(y = 0; y < dl; ++y, i++)
            {
                fprintf(fh, "0x%x, ", diffimg[i]);
            }
            fprintf(fh, "\n");
            continue;
        }
        ++i;
    }
    fseek(fh, -3, SEEK_CUR);
    fprintf(fh, "\n};\n\n");
}


int AcHlpGetModuleID(const char* name)
{
    if(stricmp(name, cod4x_modname) == 0)
    {
        return 0;
    }
    if(stricmp(name, "msvcr100.dll") == 0)
    {
        return 1;
    }
    return -1;
}


void AcHlpWriteSymbols()
{
    int i;
    char version[16];
    char dotversion[16];
    char filename[1024];
 	
    //grabbing version number
    char* find = strstr(mod_version, "MP");
    if(!find)
    {
    	MessageBoxA(NULL, "AcHlpWriteSymbols failed   strstr(mod_version, \"MP\")", "AcHlpWriteSymbols failed", MB_OK);
		exit(0);
    }
    find += 3;

    strncpy(version, find, sizeof(version));
    find = version;    
    find[15] = ' ';

    while(*find != ' ')
    {
        find++;
    }
    *find = 0;

    find = version;
    strncpy(dotversion, version, sizeof(dotversion));
    while(*find != 0)
    {
        if(*find == '.')
        {
            *find = '_';
        }
        find++;
    }

    CreateDirectoryA("symbollists", NULL);

    char timestring[256];

    time_t rawtime;
    struct tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);

    strftime(timestring, sizeof(timestring), "%y-%j-%H-%M", timeinfo );

    snprintf(filename, sizeof(filename), "symbollists\\symbollist_%s-%s.h", dotversion, timestring);

    FILE* fh = fopen(filename, "wb");
	if(!fh)
	{
		MessageBoxA(NULL, "AcHlpWriteSymbols failed  fopen(filename, \"wb\")", "AcHlpWriteSymbols failed", MB_OK);
		exit(0);
	}

    fprintf(fh, "static struct XAC_GameOffsetData xac_gameoffsetdata[] = {\n");



    for(i = 0; i < numsymbols; ++i)
    {
        fprintf(fh, "\t{0x%x, 0x%x, 0x%x, 0x%x},\n", rc5_ecb_encrypt(AcHlpGetModuleID(symbollist[i].module) +i, &cryptokey), rc5_ecb_encrypt(symbollist[i].addr +i, &cryptokey), rc5_ecb_encrypt(symbollist[i].symbol +i, &cryptokey), rc5_ecb_encrypt(symbollist[i].relative +i, &cryptokey));
    }
    fprintf(fh, "\t{0, 0, 0, 0}\n};\n\n");

    
    AcHlpWriteStaticChanges(fh);

    fprintf(fh, "struct XAC_GameBinData xac_gamebindata_%s = { \"%s\", { %d , {", version, dotversion, cryptokey.rounds);
    
    for(i = 0; i < sizeof(cryptokey.K) / sizeof(cryptokey.K[0]); ++i)
    {
        fprintf(fh, "0x%x", cryptokey.K[i]);
        if(i != sizeof(cryptokey.K) / sizeof(cryptokey.K[0]) -1)
        {
            fprintf(fh, ", ");
        }
    }
    fprintf(fh, "}},xac_gameoffsetdata, xac_staticmoddata, sizeof(xac_staticmoddata),");
    
    AcHlp_WriteChecksums(fh);

    fprintf(fh, "};\n");

    fclose(fh);
}



bool AC_CalculateHash(unsigned const char *input, int inputlen, unsigned char *diggest, int diggestlen)
{
	blake2s_state S;
	//sizeof(diggest) = BLAKE2S_OUTBYTES
	char key[32];

	memset(key, 0x8f, sizeof(key));
	key[sizeof(key) -1] = '\0';

	if( blake2s_init_key( &S, sizeof(diggest), key, sizeof(key) ) < 0 )
	{
        return false;
	}else{

	  blake2s_update( &S, input, inputlen );

	  blake2s_final( &S, diggest, diggestlen );
	}
    return true;
}



struct hashsums_t
{
    bool textpresent;
    unsigned char text[BLAKE2S_OUTBYTES];
    bool datapresent;
    unsigned char data[BLAKE2S_OUTBYTES];
    bool rdatapresent;
    unsigned char rdata[BLAKE2S_OUTBYTES];
};


void AcHlp_GetHashes(const char* lpFilename, struct hashsums_t* sums)
{
    DWORD rawlen;
	DWORD textstart;
	DWORD textlen;
	DWORD rdatastart;
	DWORD rdatalen;
	DWORD datastart;
	DWORD datalen;
	unsigned char* imagestart;

    sums->textpresent = false;
    sums->datapresent = false;
    sums->rdatapresent = false;

	int imagesize = ACHlp_LoadFile(&imagestart, lpFilename);
	if(imagesize == 0)
	{
        MessageBoxA(NULL, lpFilename, "Error loading shared library", MB_OK);
		exit(-1);
	}

	AcHlp_GetSectionBaseLen(imagestart, ".text", false, &textstart, &rawlen ,&textlen);
    AcHlp_GetSectionBaseLen(imagestart, ".rdata", false, &rdatastart, &rawlen ,&rdatalen);
    AcHlp_GetSectionBaseLen(imagestart, ".data", false, &datastart, &rawlen,&datalen);


	if(AC_CalculateHash((unsigned char*)textstart, textlen, sums->text, sizeof(sums->text) ) == false)
    {
        return;
    }
    sums->textpresent = true;
	if(rdatalen > 0 && AC_CalculateHash((unsigned char*)rdatastart, rdatalen, sums->rdata, sizeof(sums->rdata) ) == true)
    {
        sums->rdatapresent = true;
    }
	if(datalen > 0 && AC_CalculateHash((unsigned char*)datastart, datalen, sums->data, sizeof(sums->data) ) == true)
    {
        sums->datapresent = true;
    } 
}

void AcHlp_WriteSingleFileHash(FILE* fh, struct hashsums_t* h)
{
    int i;
    fprintf(fh, "{");
    if(h->textpresent)
    {
        fprintf(fh, "true, {");
        for(i = 0; i < sizeof(h->text) -1; ++i)
        {
            fprintf(fh, "0x%x, ", h->text[i]);
        }

        fprintf(fh, "0x%x }, ", h->text[i]);
    }else{
        fprintf(fh, "false, { 0 },");
    }

    if(h->datapresent)
    {
        fprintf(fh, "true, {");
        for(i = 0; i < sizeof(h->data) -1; ++i)
        {
            fprintf(fh, "0x%x, ", h->data[i]);
        }

        fprintf(fh, "0x%x }, ", h->data[i]);
    }else{
        fprintf(fh, "false, { 0 },");
    }

    if(h->rdatapresent)
    {
        fprintf(fh, "true, {");
        for(i = 0; i < sizeof(h->rdata) -1; ++i)
        {
            fprintf(fh, "0x%x, ", h->rdata[i]);
        }

        fprintf(fh, "0x%x }, ", h->rdata[i]);
    }else{
        fprintf(fh, "false, { 0 }");
    }
    fprintf(fh, "}");

}


bool AcHlp_WriteChecksums(FILE* fh)
{
    char iw3mp_path[1024];
    char msvcr100_path[1024];
    char cod4x_path[1024];
    struct hashsums_t hcod4x;
    struct hashsums_t hmsvcrt;
    struct hashsums_t hiw3mp;

    DWORD r1 = GetModuleFileNameA(NULL, iw3mp_path, sizeof(iw3mp_path));
    DWORD r2 = GetModuleFileNameA(hm, cod4x_path, sizeof(cod4x_path));
    DWORD r3 = GetModuleFileNameA(msvcrtmod, msvcr100_path, sizeof(msvcr100_path));

    if(r1 == sizeof(iw3mp_path) || r1 < 1 || r2 == sizeof(cod4x_path) || r2 < 1 || r3 == sizeof(msvcr100_path) || r3 < 1)
    {
        return false;
    }

    AcHlp_GetHashes(iw3mp_path, &hiw3mp);
    AcHlp_GetHashes(msvcr100_path, &hmsvcrt);
    AcHlp_GetHashes(cod4x_path, &hcod4x);

    AcHlp_WriteSingleFileHash(fh, &hiw3mp);
    fprintf(fh, ", ");
    AcHlp_WriteSingleFileHash(fh, &hmsvcrt);
    fprintf(fh, ", ");
    AcHlp_WriteSingleFileHash(fh, &hcod4x);
    return true;
}