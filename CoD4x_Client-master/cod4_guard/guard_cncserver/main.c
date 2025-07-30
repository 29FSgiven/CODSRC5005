#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include "zutil.h"

char core_version[256];

int ACHlp_LoadFile(unsigned char **data, const char* filename);
void AcHlp_GetSectionBaseLen(unsigned char* imagestart, char* sectionname, DWORD* sectiondata, DWORD* sectionlen, DWORD* virtualstart);
int ACHlp_GetNextLine(const char* content, int *filepos, char* ol, int len);
int AcHlp_PerformRelocation(unsigned char* imagestart, DWORD imagebase);

void Error(const char* msg)
{
    printf("%s\n", msg);
    exit(-1);
}

struct image_t
{
    DWORD virtualImagebase;
    byte* textstart;
    DWORD textstartrel;
    DWORD textvirtual;
    DWORD textlen;

    byte* rdatastart;
    DWORD rdatastartrel;
    DWORD rdatavirtual;
    DWORD rdatalen;
    
    byte* bssstart;
    DWORD bssstartrel;
    DWORD bssvirtual; //only len and virtual should exist
    DWORD bsslen;
    //reloc?
};

struct patchSymbol_t
{
    int location;
    int symbol;
    const char* imgname;
    bool relative;
};

#define MAXSYMBOLS 2048
#define MAXSTRINGS 256

struct memoryGlob_t
{
    char version[256];
    struct image_t iw3mp_img;
    struct image_t cod4x17_img;
    struct image_t msvcr100_img;
    struct patchSymbol_t psyms[MAXSYMBOLS];
    int symbolcount;
};

struct stringlist_t{
    int stringcount;
    const char *strings[MAXSTRINGS];
};


const char* AC_DupString(const char* s)
{
    const char* ns;
    static struct stringlist_t list;
    int i;

    for(i = 0; i < list.stringcount; ++i)
    {
        if(!strcmp(s, list.strings[i]))
        {
            return list.strings[i];
        }
    }

    if(list.stringcount >= MAXSTRINGS)
    {
        Error("Error: Out of strings");
    }

    ns = strdup(s);
    list.strings[list.stringcount] = ns;
    ++list.stringcount;

    return ns;
}


void AC_LoadVersionDataFiles(struct memoryGlob_t *memGlob, const char* path)
{
    char* data;
    int filepos = 0;
    char line[1024];
    int i;
    int rel;

   	int symsize = ACHlp_LoadFile((byte**)&data, "symbollist.dat");
    if(symsize > 0)
    {
        ACHlp_GetNextLine(data, &filepos, line, sizeof(line));

        strncpy(memGlob->version, line, sizeof(memGlob->version));

        for(i = 0; ACHlp_GetNextLine(data, &filepos, line, sizeof(line)) > 0 && i < MAXSYMBOLS; ++i)
        {
            char module[128];
            int loc, sym;

            sscanf(line, "%s 0x%x 0x%x %d", module, &loc, &sym, &rel);
            memGlob->psyms[i].location = loc;
            memGlob->psyms[i].symbol = sym;
            memGlob->psyms[i].imgname = AC_DupString(module);
            memGlob->psyms[i].relative = rel;
            memGlob->symbolcount++;
        }

    }
}


void AC_LoadImage(struct image_t *image, const char* name, DWORD imagebase)
{
	unsigned char* imagestart;
	int imagesize = ACHlp_LoadFile(&imagestart, name);
    
    if(imagesize < 1)
    {
        return;
    }
    image->virtualImagebase = imagebase;
    if(strstr(name, ".image"))
    {
        int textlen = *(int*)imagestart;
        image->textstart = imagestart + 4;
        image->textlen = textlen;
        image->rdatastart = imagestart + 8 + textlen;
        image->rdatalen = *(int*)(imagestart + 4 + textlen);
        image->textvirtual = 0;
        image->rdatavirtual = 0;
        image->bssstart = 0;
        image->bssstartrel = 0;
        image->bsslen = 0;
        image->bssvirtual = 0;
        if(!strcmp(name, "iw3mp.image"))
        {
            image->textvirtual = 0x1000;
        }

    }else{
        AcHlp_GetSectionBaseLen(imagestart, ".text", (DWORD*)&image->textstart, &image->textlen, &image->textvirtual);
        AcHlp_GetSectionBaseLen(imagestart, ".rdata", (DWORD*)&image->rdatastart, &image->rdatalen, &image->rdatavirtual);
        AcHlp_GetSectionBaseLen(imagestart, ".bss", (DWORD*)&image->bssstart, &image->bsslen, &image->bssvirtual);
        AcHlp_PerformRelocation(imagestart, image->virtualImagebase);
    }

    image->textstartrel = (DWORD)image->textstart - (DWORD)imagestart;
    image->rdatastartrel = (DWORD)image->rdatastart - (DWORD)imagestart;

/*
    char str[256];
    sprintf(str, "img %s, text.virtual 0x%x text.eff 0x%x\n", name, image->textvirtual, image->textstart);
    MessageBoxA(NULL, str, "Info", MB_OK);
    sprintf(str, "img %s, rdata.virtual 0x%x rdata.eff 0x%x\n", name, image->rdatavirtual, image->rdatastart);
    MessageBoxA(NULL, str, "Info", MB_OK);
    sprintf(str, "img %s, bss.virtual 0x%x bss.eff 0x%x\n", name, image->bssvirtual, image->bssstart);
    MessageBoxA(NULL, str, "Info", MB_OK);
*/
}

void AC_WriteOffset(struct memoryGlob_t *memGlob, int i)
{
    DWORD textoff = 0x1000;
    DWORD rdataoff = 0x291520;
    DWORD offset;
    unsigned char* datastart;
    DWORD symbase;
    

    if(memGlob->psyms[i].location > textoff && memGlob->psyms[i].location < textoff + memGlob->iw3mp_img.textlen)
    {
        offset = textoff;
        datastart = memGlob->iw3mp_img.textstart;
    }
    else if(memGlob->psyms[i].location > rdataoff && memGlob->psyms[i].location < rdataoff + memGlob->iw3mp_img.rdatalen)
    {
        offset = rdataoff;
        datastart = memGlob->iw3mp_img.rdatastart;
    }else{
        Error("Bad symbol offset");
    }
    if(!strcmp(memGlob->psyms[i].imgname, "msvcr100.dll"))
    {
        if(memGlob->psyms[i].symbol < memGlob->msvcr100_img.textvirtual || memGlob->psyms[i].symbol >= memGlob->msvcr100_img.textvirtual + memGlob->msvcr100_img.textlen)
        {
            Error("Bad section referenced");
        }
        symbase = memGlob->msvcr100_img.virtualImagebase;// + memGlob->msvcr100_img.textvirtual;
    }else if(!strncmp(memGlob->psyms[i].imgname, "cod4x_", 6)){
        symbase = memGlob->cod4x17_img.virtualImagebase;// + memGlob->cod4x17_img.textvirtual;
    }else{
        Error("Bad module name");
    }

    DWORD loc_rel = memGlob->psyms[i].location - offset;
    DWORD* write = (DWORD*)(datastart + loc_rel);
    DWORD tow;
    if(memGlob->psyms[i].relative == false)
    {
        tow = memGlob->psyms[i].symbol + symbase;
    }else{
        tow = (memGlob->psyms[i].symbol + symbase) - memGlob->psyms[i].location -4 - memGlob->iw3mp_img.virtualImagebase; 
    }
    *write = tow;
}


void AC_SetImageFromModuleBase(struct memoryGlob_t *memGlob)
{
    int i;

    for(i = 0; i < memGlob->symbolcount; ++i)
    {
        AC_WriteOffset(memGlob, i);
    }

}



void AC_Init(struct memoryGlob_t *memGlob)
{
    memset(memGlob, 0, sizeof(struct memoryGlob_t));
    AC_LoadImage(&memGlob->iw3mp_img, "iw3mp.image", 0x400000);
    AC_LoadImage(&memGlob->cod4x17_img, "cod4x_017.dll", 0x10860000);
    AC_LoadImage(&memGlob->msvcr100_img, "msvcr100.dll", 0x714e0000);

    AC_LoadVersionDataFiles(memGlob, NULL);
}

void AC_GetChecksum(struct memoryGlob_t *memGlob)
{
    int sum = adler32(0, memGlob->iw3mp_img.textstart, memGlob->iw3mp_img.textlen);

    FILE* fh = fopen("vdump.bin","wb");
    fwrite((void*)memGlob->msvcr100_img.textstart,1,memGlob->msvcr100_img.textlen,fh);
    fclose(fh);


    int sum2 = adler32(0, memGlob->iw3mp_img.rdatastart, memGlob->iw3mp_img.rdatalen);


    int sum3 = adler32(0, memGlob->cod4x17_img.textstart, memGlob->cod4x17_img.textlen);
    int sum4 = adler32(0, memGlob->msvcr100_img.textstart, memGlob->msvcr100_img.textlen);

    printf("iw3: %x iw3data: %x cod4x: %x crt: %x\n", sum, sum2, sum3, sum4);
}

DWORD __stdcall AC_Main(void* arg)
{
    struct memoryGlob_t memGlob;

    AC_Init(&memGlob);

    AC_SetImageFromModuleBase(&memGlob);

    AC_GetChecksum(&memGlob);

    printf("Finished\n");
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    DWORD tid;
    int param = 0;

    CreateThread(NULL, 0x10000, AC_Main, &param, 0, &tid);

    Sleep(100000);
    return 0;
}

