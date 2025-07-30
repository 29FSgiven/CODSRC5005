#define LTM_DESC
#include "../../../../common/tomcrypt/tomcrypt.h"
#include "rsa_verify.h"
#include "core.h"

int rsa_decrypt(const unsigned char *in,unsigned long inlen,unsigned char *out,unsigned long *outlen,int *stat, rsa_key *key){
	unsigned long modulus_bitlen, modulus_bytelen, x;
	int err;
	unsigned char tmp[0x10000];

	assert(out != NULL);
	assert(outlen != NULL);
	assert(key		!= NULL);
	assert(stat	 != NULL);

	*stat = 0;

	modulus_bitlen = ltc_mp.count_bits( (key->N));

	modulus_bytelen = ltc_mp.unsigned_size( (key->N));
	if(modulus_bytelen != inlen) {
		return CRYPT_INVALID_PACKET;
	}
    if(inlen > sizeof(tmp))
    {
		return CRYPT_INVALID_PACKET;
    }

	x = inlen;
	if ((err = ltc_mp.rsa_me(in, inlen, tmp, &x, PK_PUBLIC, key)) != CRYPT_OK) {
	    return err;
	}

	err = pkcs_1_v1_5_decode(tmp ,x, LTC_LTC_PKCS_1_V1_5, modulus_bitlen, out, outlen, stat);

	return err;
}




int Sec_Pem2Der(char* pemcert, int pemlen, unsigned char* dercert, unsigned int *derlen){
	char* pos;
	unsigned char assemblybuf[32768];
	int i;

	pos = strstr(pemcert, "--\n");
	if(pos == NULL){
		pos = strstr(pemcert, "--\r\n");
	}
	if(pos == NULL){
		return CRYPT_INVALID_PACKET;
	}
	while(*pos != '\n'){
		++pos;
	}
	i = 0;
	while(*pos){
		if(*pos == '-'){
			break;
		}
		if(*pos > ' ' && *pos <= 'z'){
			assemblybuf[i] = *pos;
			++i;
		}
		++pos;
	}

	return base64_decode(assemblybuf, i, dercert, (long unsigned int*)derlen);
}


bool Sec_HashMemory(const void *in, size_t inSize, char *out, long unsigned int outSize){

    int i;

    if(in == NULL || out == NULL || outSize < 64 || inSize < 1){
    	return false;
    }
    
    hash_state md;
    int result;
    unsigned char buff[64];
    
    if((result = sha256_init(&md)) != CRYPT_OK)		   { return false; }
    if((result = sha256_process(&md, (const unsigned char*)in, inSize)) != CRYPT_OK){ return false; }
    if((result = sha256_done(&md,buff)) != CRYPT_OK)	   { return false; }
    
	for(i = 0; i < 32; ++i)
	{
		sprintf(out + 2*i, "%02X", buff[i]);
	}

    return true;
}

bool RSA_VerifyMemory(const char* expectedb64hash, const void* dercert, unsigned int dercertlen, const void* memory, int lenmem)
{
	unsigned char cleartext[16384];
	unsigned char ciphertext[0x20000];
	unsigned long cleartextlen, ciphertextlen;
	int decryptstat;
	int sta;
	char hash[1025];
	rsa_key rsakey;

	ciphertextlen = sizeof(ciphertext);



	if(base64_decode((unsigned char*)expectedb64hash, strlen((char*)expectedb64hash), ciphertext, (long unsigned int*)&ciphertextlen) != CRYPT_OK)
	{
		return false;
	}

	if((sta = rsa_import((const unsigned char*)dercert, dercertlen, &rsakey)) != CRYPT_OK){
		return false;
	}

	cleartextlen = sizeof(cleartext) -1;

	sta = rsa_decrypt(ciphertext, ciphertextlen, cleartext, &cleartextlen, &decryptstat, &rsakey);

	rsa_free(&rsakey);

	if(sta != CRYPT_OK || cleartextlen < 0 || cleartextlen > sizeof(cleartext) -1){
		return false;
	}

	cleartext[cleartextlen] = '\0';

	if(!Sec_HashMemory(memory, lenmem, hash, sizeof(hash))){
		return false;
	}
	if(stricmp(hash, (char*)cleartext) != 0)
	{
		return false;
	}
	return true;

}

void TomCrypt_Init(){
	ltc_mp = ltm_desc;
}