#include <stdio.h>
#include <windows.h>
#include "checksumengine.h"
#include "../../exeobfus/exeobfus.h"
#include "common/basic.h"

void AcHlp_GetSectionBaseLen(unsigned char* imagestart, const char* sectionname, DWORD* sectiondata, DWORD* sectionlen, DWORD* virtualstart);
int AcHlp_PerformRelocation(unsigned char* imagestart, DWORD imagebase);
bool AC_CalculateHash(unsigned const char *input, int inputlen, unsigned char *diggest, int diggestlen);

struct XAC_GameBinData* xac_gamebindata_ptr;


/*********************************************************************************/
/* Loading a PE file into memory, applying offset patches and relocation         */
/*********************************************************************************/

struct image_t
{
    DWORD imageBase;
    DWORD virtualImagebase;
    byte* textstart;
    DWORD textstartrel;
    DWORD textvirtual;
    DWORD textlen;
    unsigned char texthash[BLAKE2S_OUTBYTES];
    unsigned char mappedtexthash[BLAKE2S_OUTBYTES];

    byte* rdatastart;
    DWORD rdatastartrel;
    DWORD rdatavirtual;
    DWORD rdatalen;
    unsigned char rdatahash[BLAKE2S_OUTBYTES];
    unsigned char mappedrdatahash[BLAKE2S_OUTBYTES];

    byte* datastart;
    DWORD datastartrel;
    DWORD datavirtual;
    DWORD datalen;    
    unsigned char datahash[BLAKE2S_OUTBYTES];
    unsigned char mappeddatahash[BLAKE2S_OUTBYTES];
};

struct memoryGlob_t
{
    char version[256];
    struct image_t iw3mp_img;
    struct image_t cod4x_img;
    struct image_t msvcr100_img;
    HMODULE hcod4x;
    HMODULE hmsvcr100;
};



int AC_WriteOffset(struct memoryGlob_t *memGlob, XAC_GameOffsetData* psyms, const struct rc5_32_key *skey, int i)
{
    DWORD textoff = 0x1000;
    DWORD rdataoff = 0x291520;
    DWORD dataoff = 0x31b000;
    DWORD offset;
    unsigned char* datastart;
    DWORD symbase;
    DWORD location = rc5_ecb_decrypt(psyms->location, skey) -i;
    DWORD moduleindex;
    DWORD symbol;
    
    CODECRC();

    if(location > textoff && location < textoff + memGlob->iw3mp_img.textlen)
    {
        offset = textoff;
        datastart = memGlob->iw3mp_img.textstart;
    }
    else if(location > rdataoff && location < rdataoff + memGlob->iw3mp_img.rdatalen)
    {
        offset = rdataoff;
        datastart = memGlob->iw3mp_img.rdatastart;
    }
    else if(location > dataoff && location < dataoff + memGlob->iw3mp_img.datalen)
    {
        offset = dataoff;
        datastart = memGlob->iw3mp_img.datastart;
    }else{
        return -5; //Bad symbol offset
    }
    moduleindex = rc5_ecb_decrypt(psyms->moduleindex, skey) -i;
    symbol = rc5_ecb_decrypt(psyms->symbol, skey) -i;
    CODECRC();
    if(moduleindex == 1)
    {
        if(symbol < memGlob->msvcr100_img.textvirtual || symbol >= memGlob->msvcr100_img.textvirtual + memGlob->msvcr100_img.textlen)
        {
            return -6; //Error("Bad section referenced");
        }
        symbase = memGlob->msvcr100_img.virtualImagebase;// + memGlob->msvcr100_img.textvirtual;
    }else if(moduleindex == 0){
        symbase = memGlob->cod4x_img.virtualImagebase;// + memGlob->cod4x17_img.textvirtual;
    }else{
        return -7; //Error("Bad module name");
    }

    DWORD loc_rel = location - offset;
    DWORD* write = (DWORD*)(datastart + loc_rel);
    DWORD tow;
    CODECRC();
    if((rc5_ecb_decrypt(psyms->relative, skey) -i) == 0)
    {
        tow = symbol + symbase;
    }else{
        tow = (symbol + symbase) - location -4 - memGlob->iw3mp_img.virtualImagebase; 
    }
    *write = tow;
    return 0;
}

void AC_ApplyStaticPatches(struct memoryGlob_t *memGlob, const struct rc5_32_key *skey)
{
    int i;
    unsigned const char* unpackptr = xac_gamebindata_ptr->staticdata;
    CODECRC();
    for(i = 0; unpackptr < xac_gamebindata_ptr->staticdata + xac_gamebindata_ptr->staticdatalen ;++i)
    {
        DWORD addr;
        unsigned char* caddr = (unsigned char*)&addr;
        caddr[0] = unpackptr[0];
        caddr[1] = unpackptr[1];
        caddr[2] = unpackptr[2];
        caddr[3] = unpackptr[3];
        unpackptr += 4;
        char type = unpackptr[0];
        unpackptr++;

        unsigned short length;
        unsigned char* clength = (unsigned char*)&length;

        clength[0] = unpackptr[0];
        clength[1] = unpackptr[1];

        
        unpackptr += 2;
        if(type == 'r')
        {
            memset(memGlob->iw3mp_img.textstart + rc5_ecb_decrypt(addr, skey), unpackptr[0], length);
            unpackptr++;
        }else if(type == 'd'){
            memcpy(memGlob->iw3mp_img.textstart + rc5_ecb_decrypt(addr, skey), unpackptr, length);
            unpackptr += length;
        }
    } 

}

int AC_SetImageFromModuleBase(struct memoryGlob_t *memGlob)
{
    int i;
    CODECRC();
    AC_ApplyStaticPatches(memGlob, &xac_gamebindata_ptr->key);

    for(i = 0; xac_gamebindata_ptr->offsets[i].symbol != 0 || xac_gamebindata_ptr->offsets[i].location != 0; ++i)
    {
        int r = AC_WriteOffset(memGlob, &xac_gamebindata_ptr->offsets[i], &xac_gamebindata_ptr->key, i);
        if(r != 0)
        {
            return r;
        }
    }
    byte* dataptr = memGlob->iw3mp_img.datastart;
    DWORD *security_cookie = (DWORD*)(dataptr + 0xC5C);
    *security_cookie = *(DWORD*)0x071BC5C;
    return 0;
}


bool AC_LoadImage(struct image_t *image, const wchar_t* name, DWORD imagebase)
{
    char sbuf[1024];
	unsigned char* imagestart;

    CODECRC();
	int imagesize = ACHlp_LoadFile(&imagestart, name);
    
    if(imagesize < 1)
    {
        return false;
    }
    image->virtualImagebase = imagebase;
    image->imageBase = (DWORD)imagestart;

    PROTECTSTRING(".text", sbuf);

    AcHlp_GetSectionBaseLen(imagestart, sbuf, (DWORD*)&image->textstart, &image->textlen, &image->textvirtual);
    
    PROTECTSTRING(".rdata", sbuf);
    AcHlp_GetSectionBaseLen(imagestart, sbuf, (DWORD*)&image->rdatastart, &image->rdatalen, &image->rdatavirtual);
    
    CODECRC();
    if(AC_CalculateHash(image->textstart, image->textlen, image->texthash, sizeof(image->texthash) ) == false)
    {
        return false;
    }

    if(image->textlen != 0xb1221)
    {
        if(AC_CalculateHash(image->rdatastart, image->rdatalen, image->rdatahash, sizeof(image->rdatahash) ) == false)
        {
            return false;
        }
    }
    PROTECTSTRING(".data", sbuf);
    AcHlp_GetSectionBaseLen(imagestart, sbuf, (DWORD*)&image->datastart, &image->datalen, &image->datavirtual);
    CODECRC();

    if(imagebase == 0x400000)
    {
        if(AC_CalculateHash(image->datastart, image->datalen, image->datahash, sizeof(image->datahash) ) == false)
        {
            return false;
        }
    }else{
        AC_CalculateHash(image->datastart, image->datalen, image->datahash, sizeof(image->datahash) );
        AcHlp_PerformRelocation(imagestart, image->virtualImagebase);
    }


    image->textstartrel = (DWORD)image->textstart - (DWORD)imagestart;
    image->rdatastartrel = (DWORD)image->rdatastart - (DWORD)imagestart;
    
    CODECRC();
    if(image->textstart == 0 || image->textlen == 0 || image->textvirtual == 0)
    {
        return false;
    }
    if(image->textlen == 0xb1221)
    {
        return true;
    }
    if(image->rdatastart == 0 || image->rdatalen == 0 || image->rdatavirtual == 0)
    {
        return false;
    }
    if(imagebase == 0x400000)
    {
        if(image->datastart == 0 || image->datalen == 0 || image->datavirtual == 0)
        {
            return false;
        } 
    }
    return true;
}


void AC_FreeImage(struct image_t *image)
{
    free((void*)image->imageBase);
    image->rdatastart = NULL;
    image->datastart = NULL;
    image->textstart = NULL;
    image->rdatastartrel = 0;
    image->datastartrel = 0;
    image->textstartrel = 0;
}


void AC_FreeFileData(struct memoryGlob_t *memGlob)
{
    AC_FreeImage(&memGlob->cod4x_img);
    AC_FreeImage(&memGlob->iw3mp_img);
    AC_FreeImage(&memGlob->msvcr100_img);
}


bool AC_VerifyImageWithFileData(struct memoryGlob_t *memGlob)
{
    CODECRC();
    //Comparing sections of iw3mp.exe
    if(memcmp(memGlob->iw3mp_img.textstart, (void*)0x401000, memGlob->iw3mp_img.textlen) != 0)
    {
        return false;
    }
    AC_CalculateHash(memGlob->iw3mp_img.textstart, memGlob->iw3mp_img.textlen, memGlob->iw3mp_img.mappedtexthash, sizeof(memGlob->iw3mp_img.mappedtexthash));

    int importlength = 0x520;
    if(memcmp(memGlob->iw3mp_img.rdatastart + importlength, (void*)(0x691000 + importlength), memGlob->iw3mp_img.rdatalen - importlength) != 0)
    {
        return false;
    }
    AC_CalculateHash(memGlob->iw3mp_img.rdatastart + importlength, memGlob->iw3mp_img.rdatalen - importlength, memGlob->iw3mp_img.mappedrdatahash, sizeof(memGlob->iw3mp_img.mappedrdatahash));
    CODECRC();
    if(memcmp(memGlob->iw3mp_img.datastart, (void*)(0x71b000), memGlob->iw3mp_img.datalen) != 0)
    {
        return false;
    }

    //Comparing sections of cod4x_0xx.dll
    if(memcmp(memGlob->cod4x_img.textstart, (byte*)memGlob->hcod4x + memGlob->cod4x_img.textvirtual, memGlob->cod4x_img.textlen) != 0)
    {
        return false;
    }        
    AC_CalculateHash(memGlob->cod4x_img.textstart, memGlob->cod4x_img.textlen, memGlob->cod4x_img.mappedtexthash, sizeof(memGlob->cod4x_img.mappedtexthash));
    CODECRC();
    if(memcmp(memGlob->cod4x_img.rdatastart, (byte*)memGlob->hcod4x + memGlob->cod4x_img.rdatavirtual, memGlob->cod4x_img.rdatalen) != 0)
    {
        return false;
    }
    AC_CalculateHash(memGlob->cod4x_img.rdatastart, memGlob->cod4x_img.rdatalen, memGlob->cod4x_img.mappedrdatahash, sizeof(memGlob->cod4x_img.mappedrdatahash));


    //Comparing sections of msvcr100.dll
    importlength = 0x60c;
    if(memcmp(memGlob->msvcr100_img.textstart + importlength, (byte*)memGlob->hmsvcr100 + memGlob->msvcr100_img.textvirtual + importlength, memGlob->msvcr100_img.textlen - importlength) != 0)
    {
        return false;
    }
    AC_CalculateHash(memGlob->msvcr100_img.textstart + importlength, memGlob->msvcr100_img.textlen - importlength, memGlob->msvcr100_img.mappedtexthash, sizeof(memGlob->msvcr100_img.mappedtexthash));
       
    return true;

}




bool AC_ExtractVersionNumberFromBuildString(const char* buildstring, char* major, char* minor)
{
    char version[16];
    //grabbing version number
    CODECRC();
    const char* cfind = strstr(buildstring, "MP");
    if(!cfind)
    {
        return false;
    }
    cfind += 3;

    strncpy(version, cfind, sizeof(version) -1);
    
    char* find = version;
    find[15] = ' ';

    while(*find != ' ')
    {
        find++;
    }
    *find = 0;

    find = version;
    while(*find != 0)
    {
        if(*find == '.')
        {
            *find = 0;
            strcpy(minor, find+1);
        }
        find++;
    }
    strcpy(major, version);
    if(atoi(major) > 0 && (atoi(minor) > 0 || minor[0] == '0'))
    {
        return true;
    }
    return false;
}



/* return values
    -1: too deep nested cod4 directory or error in accessing handles
    -2: bad version info
    -3: error loading file image
    -5: Bad symbol offset
    -6: Bad section referenced
    -7: Bad module name
     1: tampered
     0: success
*/

static memoryGlob_t memGlob;

int AC_ChecksumVerifyMain(const char* build_string) 
{
    wchar_t iw3mp_path[1024];
    wchar_t msvcr100_path[1024];
    wchar_t cod4x_path[1024];
    char core_version[16];
    char minor[16];
    char cod4xfilename[128];
    char cod4versionid[33];
    char sbuf[1024];
    int i;
    
    CODECRC();
    xac_gamebindata_ptr = NULL;


    if(AC_ExtractVersionNumberFromBuildString(build_string, core_version, minor) == false)
    {
        return -2;
    }

    sprintf(cod4versionid, "%s.%s", core_version, minor);

    i = 0;
    while(symbolversions[i] != NULL)
    {
        if(strcmp(symbolversions[i]->version ,cod4versionid) == 0)
        {
            xac_gamebindata_ptr = symbolversions[i];
            break;
        }
        ++i;
    }
    if(xac_gamebindata_ptr == NULL)
    {
        return -22;
    }

    CODECRC();
    PROTECTSTRING("cod4x_0%s.dll", sbuf);
    sprintf(cod4xfilename, sbuf, core_version);
    
    DWORD r1 = GetModuleFileNameW(NULL, iw3mp_path, sizeof(iw3mp_path) /2);
    HMODULE hcod4x = GetModuleHandleA(cod4xfilename);

    PROTECTSTRING("msvcr100.dll", sbuf);
    HMODULE hmsvcr100 = GetModuleHandleA(sbuf);
    DWORD r2 = GetModuleFileNameW(hcod4x, cod4x_path, sizeof(cod4x_path) /2);
    DWORD r3 = GetModuleFileNameW(hmsvcr100, msvcr100_path, sizeof(msvcr100_path) /2);



    if(r1 == sizeof(iw3mp_path)/2 || r1 < 1 || r2 == sizeof(cod4x_path)/2 || r2 < 1 || r3 == sizeof(msvcr100_path) || r3 < 1)
    {
        return -1;
    }
    CODECRC();
    memset(&memGlob, 0, sizeof(struct memoryGlob_t));
    memGlob.hcod4x = hcod4x;
    memGlob.hmsvcr100 = hmsvcr100;
    bool b1 = AC_LoadImage(&memGlob.iw3mp_img, iw3mp_path, 0x400000);
    bool b2 = AC_LoadImage(&memGlob.cod4x_img, cod4x_path, (DWORD)hcod4x);
    bool b3 = AC_LoadImage(&memGlob.msvcr100_img, msvcr100_path, (DWORD)hmsvcr100);

    if(!b1 || !b2 || !b3)
    {
        return -3;
    }

    if(memcmp(memGlob.iw3mp_img.texthash, xac_gamebindata_ptr->iw3mphash.text, sizeof(memGlob.iw3mp_img.texthash)) != 0)
    {
        return 1;
    }
    if(memcmp(memGlob.iw3mp_img.datahash, xac_gamebindata_ptr->iw3mphash.data, sizeof(memGlob.iw3mp_img.datahash)) != 0)
    {
        return 1;
    }
    CODECRC();
    if(memcmp(memGlob.iw3mp_img.rdatahash, xac_gamebindata_ptr->iw3mphash.rdata, sizeof(memGlob.iw3mp_img.rdatahash)) != 0)
    {
        return 1;
    }   

    if(memcmp(memGlob.cod4x_img.texthash, xac_gamebindata_ptr->cod4xhash.text, sizeof(memGlob.cod4x_img.texthash)) != 0)
    {
        return 1;
    }
    CODECRC();
    if(memcmp(memGlob.cod4x_img.rdatahash, xac_gamebindata_ptr->cod4xhash.rdata, sizeof(memGlob.cod4x_img.rdatahash)) != 0)
    {
        return 1;
    }
    if(memcmp(memGlob.cod4x_img.datahash, xac_gamebindata_ptr->cod4xhash.data, sizeof(memGlob.cod4x_img.datahash)) != 0)
    {
        return 1;
    }
    CODECRC();
    if(memcmp(memGlob.msvcr100_img.texthash, xac_gamebindata_ptr->msvcrt100hash.text, sizeof(memGlob.msvcr100_img.texthash)) != 0)
    {
        return 1;
    }

    int r = AC_SetImageFromModuleBase(&memGlob);
    if(r != 0)
    {
        return r;
    }


    if(AC_VerifyImageWithFileData(&memGlob) == false)
    {
        AC_FreeFileData(&memGlob);
        return 2;
    }
    AC_FreeFileData(&memGlob);
    return 0;

}

bool AC_CalculateHash(unsigned const char *input, int inputlen, unsigned char *diggest, int diggestlen)
{
	blake2s_state S;
	//sizeof(diggest) = BLAKE2S_OUTBYTES
	char key[32];
    
    CODECRC();
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








bool AC_ChecksumVerifyLate()
{
    CODECRC();

    unsigned char diggest[32];

    AC_CalculateHash((unsigned const char*)0x401000, memGlob.iw3mp_img.textlen, diggest, sizeof(diggest));
   
    //Comparing sections of iw3mp.exe
    if(memcmp(diggest, memGlob.iw3mp_img.mappedtexthash, sizeof(diggest)) != 0)
    {
        return false;
    }
    CODECRC();
    int importlength = 0x520;

    AC_CalculateHash((unsigned const char*)(0x691000 + importlength), memGlob.iw3mp_img.rdatalen - importlength, diggest, sizeof(diggest));

    if(memcmp(diggest, memGlob.iw3mp_img.mappedrdatahash, sizeof(diggest)) != 0)
    {
        return false;
    }

    CODECRC();

    //Comparing sections of cod4x_0xx.dll
    AC_CalculateHash((byte*)memGlob.hcod4x + memGlob.cod4x_img.textvirtual, memGlob.cod4x_img.textlen, diggest, sizeof(diggest));

    if(memcmp(diggest, memGlob.cod4x_img.mappedtexthash, sizeof(diggest)) != 0)
    {
        return false;
    }

    CODECRC();

    AC_CalculateHash((byte*)memGlob.hcod4x + memGlob.cod4x_img.rdatavirtual, memGlob.cod4x_img.rdatalen, diggest, sizeof(diggest));

    if(memcmp(diggest, memGlob.cod4x_img.mappedrdatahash, sizeof(diggest)) != 0)
    {
        return false;
    }
    CODECRC();
    //Comparing sections of msvcr100.dll
    importlength = 0x60c;
    AC_CalculateHash((byte*)memGlob.hmsvcr100 + memGlob.msvcr100_img.textvirtual + importlength, memGlob.msvcr100_img.textlen - importlength, diggest, sizeof(diggest));
    CODECRC();
    if(memcmp(diggest, memGlob.msvcr100_img.mappedtexthash, sizeof(diggest)) != 0)
    {
        return false;
    }
       
    return true;
}