#include "tomcrypt/tomcrypt.h"

class NETSecureInstance
{
public:
    NETSecureInstance();
    ~NETSecureInstance();
    bool DH_Finish(unsigned char* foreignkeybuf, int keylen, unsigned char *new_foreign_iv, unsigned char* foreign_signature);
    int DH_Setup(unsigned char* keybuf, int maxsize, unsigned char *new_iv);
    int EncryptMessage(unsigned char* message, unsigned int messagelen, unsigned char* outmessage, unsigned int maxoutmsg);
    int DecryptMessage(unsigned char* in_ciphertext, int chipertextlen, unsigned char* outmessage, int maxoutmsg);

private:
    dh_key dhkey;
    dh_key foreign_dhkey;
    int dhstate;
    symmetric_key symmetric_cipherkey;
    unsigned char iv[16];
    unsigned char foreign_iv[16];

};

int EncryptMessage(void* ctx, unsigned char* message, unsigned int messagelen, unsigned int outlimit);
int DecryptMessage(void* ctx, unsigned char* message, unsigned int messagelen);

