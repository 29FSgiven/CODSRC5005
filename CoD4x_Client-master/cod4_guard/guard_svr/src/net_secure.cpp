#define LTM_DESC
#include "tomcrypt/tomcrypt.h"
#include "sys_main.h"

#define IN_NET_SECURECODE
#include "net_secure.h"

/* 
void PrintKey(unsigned char* rawcipherkey, const char* premsg)
{
        int k;
        char hstring[64];
       	for(k = 0; k < 16; ++k)
        {
            sprintf(hstring + 2*k, "%02X", rawcipherkey[k]);
        }
        Com_Printf("%s %s\n", premsg, hstring);
}*/

class NETSecureGlob
{
public:
    NETSecureGlob()
    {
        bready = false;
        unsigned char randombytes[8192];

        ltc_mp = ltm_desc; //tell tomcrypt to use toms math

        Sys_RandomBytes(randombytes, sizeof(randombytes));

        chacha20_prng_start(&prngstate);
        if(chacha20_prng_add_entropy(randombytes, sizeof(randombytes), &prngstate) != CRYPT_OK)
        {
            return;
        }
        if(chacha20_prng_ready(&prngstate) != CRYPT_OK)
        {
            return;
        }
        /* register yarrow */
        if (register_prng(&chacha20_prng_desc) == -1) {
            return;
        }
        if (register_hash(&blake2s_128_desc) == -1) {
            return;
        }
        bready = true;
    }
    ~NETSecureGlob()
    {
        chacha20_prng_done(&prngstate);
    }
    prng_state* GetPrng()
    {
        return &prngstate;
    }
    bool ready()
    {
        return bready;
    }
private:
    prng_state prngstate;
    bool bready;
};


class NETSecureGlob *netSecGlob;

    NETSecureInstance::NETSecureInstance()
    {
        dhstate = 0;

        if(netSecGlob == NULL)
        {
            netSecGlob = new NETSecureGlob();
        }
    }
    
    NETSecureInstance::~NETSecureInstance()
    {
        if(dhstate & 1)
        {
            dh_free(&dhkey);
        }
        if(dhstate & 2)
        {
            dh_free(&foreign_dhkey);
        }
        if(dhstate & 4)
        {
            rijndael_done(&symmetric_cipherkey);
        }
    }

    bool NETSecureInstance::DH_Finish(unsigned char* foreignkeybuf, int keylen, unsigned char *new_foreign_iv, unsigned char *foreign_signature)
    {
        unsigned char buf[4096];
        unsigned char rawcipherkey[16];


        if((dhstate & 1) == false)
        {
            return false;
        }
        /* load key of foreign host */
        if (dh_import(foreignkeybuf, keylen, &foreign_dhkey) != CRYPT_OK) {
            return false;
        }
        dhstate |= 2;
        /* make shared secret */
        unsigned long x = sizeof(buf);
        if (dh_shared_secret(&dhkey, &foreign_dhkey, buf, &x) != CRYPT_OK) {
            return false;
        }

       	hash_state S;

       	if ( sizeof(rawcipherkey) < 16 ) {
            return false;
        }

        if( blake2s_init( &S, 16 , NULL, 0 ) < 0 )
	    {
		    return false;
	    }
   		blake2s_process( &S, ( uint8_t * )buf, x );
       	blake2s_done( &S, rawcipherkey );

        rijndael_setup(rawcipherkey, sizeof(rawcipherkey), 0, &symmetric_cipherkey);

        dhstate |= 4;
        memcpy(foreign_iv, new_foreign_iv, sizeof(foreign_iv));
        return true;
        //Data can get encrypted now with Rijndael-128
    }

    int NETSecureInstance::DH_Setup(unsigned char* keybuf, int maxsize/* , unsigned char* signature, int *signaturelen*/, unsigned char *new_iv)
    {
        if((dhstate & 1) == false)
        {
            /* make up our private key */
            if (dh_set_pg_groupsize(256, &dhkey) != CRYPT_OK) {
                return 0;
            }
            if ( dh_generate_key(netSecGlob->GetPrng(), 0, &dhkey) != CRYPT_OK) {
                return 0;
            }
            dhstate |= 1;
        }
        /* export our key as public */
        unsigned long x = maxsize;
        if (dh_export(keybuf, &x, PK_PUBLIC, &dhkey) != CRYPT_OK) {
            return 0;
        }
        Sys_RandomBytes(iv, sizeof(iv));
        memcpy(new_iv, iv, sizeof(iv));

        unsigned char signinput[1024];

        memset(signinput, 0, sizeof(signinput));
        memcpy(signinput, iv, sizeof(iv));
        int signxlen = x;
        if(signxlen + sizeof(iv) > sizeof(signinput))
        {
            signxlen = sizeof(signinput) - sizeof(iv);
        }
        memcpy(signinput, keybuf, signxlen);


    
        int ACHlp_LoadFile(unsigned char **data, const char* filename)


        fopen("live\\iceops.co\\privkey.pem", "rb");

        rsa_import_x509();

    //    rsa_sign_hash(signinput, sizeof(signinput), outhash, signature, signaturelen, 0, 0, 16, &rsa_key);


        return x;
    }

    int NETSecureInstance::EncryptMessage(unsigned char* message, unsigned int messagelen, unsigned char* outmessage, unsigned int maxoutmsg)
    {
        unsigned char plaintext[16];
        unsigned char ciphertext[16];
        unsigned int i, k;
        bool paddingappended = false;

   		memcpy(ciphertext, iv, sizeof(ciphertext));

        for(i = 0; i < messagelen || paddingappended == false; i += 16)
		{
            if(i + 16 > maxoutmsg)
            {
                return -1;
            }
            int numcpy = sizeof(plaintext);
            if((signed)messagelen - (signed)i < (signed)sizeof(plaintext))
            {
                numcpy = messagelen - i;
            }
			memcpy(plaintext, message + i, numcpy);
            if(numcpy < (signed)sizeof(plaintext))//Padding
            {
                plaintext[numcpy] = 0x5f;
                paddingappended = true;
                for(k = numcpy + 1; k < sizeof(plaintext); ++k)
                {
                    plaintext[k] = 0;
                }
            }

			for(k = 0; k < sizeof(plaintext); ++k) //Cipher Block Chaining Mode
			{
				plaintext[k] ^= ciphertext[k];
			}

			rijndael_ecb_encrypt(plaintext, ciphertext, &symmetric_cipherkey);

			memcpy(outmessage + i, ciphertext, sizeof(ciphertext));
		}
   		memcpy(iv, ciphertext, sizeof(iv));
        return i;
    }

    int NETSecureInstance::DecryptMessage(unsigned char* in_ciphertext, int ciphertextlen, unsigned char* outmessage, int maxoutmsg)
    {
        unsigned char ciphertext[16];
        unsigned char plaintext[16];

        int i, k;

        for(i = 0; i < ciphertextlen; i += 16)
		{
            if(i + 16 > maxoutmsg)
            {
                return -1; //outbuffer too small
            }

   			memcpy(ciphertext, in_ciphertext + i, sizeof(ciphertext));
   			
            rijndael_ecb_decrypt(ciphertext, plaintext, &symmetric_cipherkey);

            for(k = 0; k < 16; ++k) //Cipher Block Chaining Mode
			{
				plaintext[k] ^= foreign_iv[k];
			}
			memcpy(foreign_iv, ciphertext, sizeof(foreign_iv));
			memcpy(outmessage + i, plaintext, sizeof(plaintext));
        }
        if(i < 16)
        {
            return -1; //no message
        }
        for(k = 15; k >= 0; --k)
        {
            if(outmessage[i - 16 + k] == 0x5f)
            {
                break;
            }
        }
        if(k < 0)
        {
            return -1; //No padding
        }
        return i - 16 + k;
    }

int EncryptMessage(void* ctx, unsigned char* message, unsigned int messagelen, unsigned int outlimit)
{
    NETSecureInstance* inst = (NETSecureInstance*)ctx;
    return inst->EncryptMessage(message, messagelen, message, outlimit);
}

int DecryptMessage(void* ctx, unsigned char* message, unsigned int messagelen)
{
    NETSecureInstance* inst = (NETSecureInstance*)ctx;
    return inst->DecryptMessage(message, messagelen, message, messagelen);
}
