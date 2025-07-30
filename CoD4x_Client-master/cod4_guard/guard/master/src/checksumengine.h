#include "blake/blake2.h"
#include "rc5_32.h"

/********************************************************/
/*      Definitions of patched offsets in iw3mp         */
/********************************************************/

//structure used here in include
struct XAC_GameOffsetData
{
    unsigned int moduleindex; //1 = mscvr100, 0 = cod4x_xxx
    unsigned int location;
    unsigned int symbol;
    unsigned int relative;
};

struct hashsums_t
{
    bool textpresent;
    unsigned char text[BLAKE2S_OUTBYTES];
    bool datapresent;
    unsigned char data[BLAKE2S_OUTBYTES];
    bool rdatapresent;
    unsigned char rdata[BLAKE2S_OUTBYTES];
};

struct XAC_GameBinData
{
    const char* version;
    struct rc5_32_key key;
    struct XAC_GameOffsetData* offsets;
    const unsigned char* staticdata;
    unsigned int staticdatalen;
    struct hashsums_t iw3mphash;
    struct hashsums_t msvcrt100hash;
    struct hashsums_t cod4xhash;
};


extern struct XAC_GameBinData *symbolversions[];


int AC_ChecksumVerifyMain(const char* build_string);
bool AC_ChecksumVerifyLate();