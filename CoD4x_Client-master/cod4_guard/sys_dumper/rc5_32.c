/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 */

/**
   @file rc5.c
   LTC_RC5 code by Tom St Denis
   modified for blocklenght of 32 bit
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <mmintrin.h>
#include <stdint.h>

#include "rc5_32.h"

#define BSWAP(x)  ( ((x>>24)&0x000000FFUL) | ((x<<24)&0xFF000000UL)  | \
                    ((x>>8)&0x0000FF00UL)  | ((x<<8)&0x00FF0000UL) )

typedef unsigned ulong32;


#define ROL(x, y) ( (((ulong16)(x)<<(ulong16)((y)&15)) | (((ulong16)(x)&0xFFFFFFFFUL)>>(ulong16)((16-((y)&15))&15))) & 0xFFFFFFFFUL)
#define ROR(x, y) ( ((((ulong16)(x)&0xFFFFFFFFUL)>>(ulong16)((y)&15)) | ((ulong16)(x)<<(ulong16)((16-((y)&15))&15))) & 0xFFFFFFFFUL)
#define ROLc(x, y) ( (((ulong16)(x)<<(ulong16)((y)&15)) | (((ulong16)(x)&0xFFFFFFFFUL)>>(ulong16)((16-((y)&15))&15))) & 0xFFFFFFFFUL)
#define RORc(x, y) ( ((((ulong16)(x)&0xFFFFFFFFUL)>>(ulong16)((y)&15)) | ((ulong16)(x)<<(ulong16)((16-((y)&15))&15))) & 0xFFFFFFFFUL)



#define XMEMCPY memcpy

/* error codes [will be expanded in future releases] */
enum {
   CRYPT_OK=0,             /* Result OK */
   CRYPT_INVALID_KEYSIZE,  /* Invalid key size given */
   CRYPT_INVALID_ROUNDS   /* Invalid number of rounds */

};

#define STORE16L(x, y)                                                                     \
  do { (y)[1] = (unsigned char)(((x)>>8)&255); (y)[0] = (unsigned char)((x)&255); } while(0)

#define LOAD16L(x, y)                            \
  do { x = ((ulong16)((y)[1] & 255)<<8)  | \
           ((ulong16)((y)[0] & 255)); } while(0)


#define MAX(a,b) (((a)>(b))? (a):(b))

static const ulong32 stab[50] = {
0xb7e15163UL, 0x5618cb1cUL, 0xf45044d5UL, 0x9287be8eUL, 0x30bf3847UL, 0xcef6b200UL, 0x6d2e2bb9UL, 0x0b65a572UL,
0xa99d1f2bUL, 0x47d498e4UL, 0xe60c129dUL, 0x84438c56UL, 0x227b060fUL, 0xc0b27fc8UL, 0x5ee9f981UL, 0xfd21733aUL,
0x9b58ecf3UL, 0x399066acUL, 0xd7c7e065UL, 0x75ff5a1eUL, 0x1436d3d7UL, 0xb26e4d90UL, 0x50a5c749UL, 0xeedd4102UL,
0x8d14babbUL, 0x2b4c3474UL, 0xc983ae2dUL, 0x67bb27e6UL, 0x05f2a19fUL, 0xa42a1b58UL, 0x42619511UL, 0xe0990ecaUL,
0x7ed08883UL, 0x1d08023cUL, 0xbb3f7bf5UL, 0x5976f5aeUL, 0xf7ae6f67UL, 0x95e5e920UL, 0x341d62d9UL, 0xd254dc92UL,
0x708c564bUL, 0x0ec3d004UL, 0xacfb49bdUL, 0x4b32c376UL, 0xe96a3d2fUL, 0x87a1b6e8UL, 0x25d930a1UL, 0xc410aa5aUL,
0x62482413UL, 0x007f9dccUL
};

 /**
    Initialize the LTC_RC5 block cipher
    @param key The symmetric key you wish to pass
    @param keylen The key length in bytes
    @param num_rounds The number of rounds desired (0 for default)
    @param skey The key in as scheduled by this function.
    @return CRYPT_OK if successful
 */

int rc5_setup(const unsigned char *key, int keylen, int num_rounds, struct rc5_32_key *skey)
{
    ulong32 L[64], A, B, i, j, v, s, t, l;
    ulong16 *S;
    
    /* test parameters */
    if (num_rounds == 0) {
       num_rounds = 24;
    }

    /* key must be between 64 and 1024 bits */
    if (keylen < 8 || keylen > 128) {
       return CRYPT_INVALID_KEYSIZE;
    }

    skey->rounds = num_rounds;
    S = skey->K;

    /* copy the key into the L array */
    for (A = i = j = 0; i < (ulong32)keylen; ) {
        A = (A << 8) | ((ulong32)(key[i++] & 255));
        if ((i & 3) == 0) {
           L[j++] = BSWAP(A);
           A = 0;
        }
    }

    if ((keylen & 3) != 0) {
       A <<= (ulong32)((8 * (4 - (keylen&3))));
       L[j++] = BSWAP(A);
    }

    /* setup the S array */
    t = (ulong32)(2 * (num_rounds + 1));
    XMEMCPY(S, stab, t * sizeof(*S));

    /* mix buffer */
    s = 3 * MAX(t, j);
    l = j;
    for (A = B = i = j = v = 0; v < s; v++) {
        A = S[i] = ROLc(S[i] + A + B, 3);
        B = L[j] = ROL(L[j] + A + B, (A+B));
        if (++i == t) { i = 0; }
        if (++j == l) { j = 0; }
    }
    return CRYPT_OK;
}


/**
  Encrypts a block of text with LTC_RC5
  @param pt The input plaintext (4 bytes)
  @param skey The key as scheduled
  @return The output ciphertext (4 bytes)
*/

uint32_t rc5_ecb_encrypt(uint32_t pt, const struct rc5_32_key *skey)
{
   uint32_t ct;
   ulong16 A, B;
   const ulong16 *K;
   int r;

   LOAD16L(A, &((char*)&pt)[0]);
   LOAD16L(B, &((char*)&pt)[2]);
   A += skey->K[0];
   B += skey->K[1];
   K  = skey->K + 2;

   if ((skey->rounds & 1) == 0) {
      for (r = 0; r < skey->rounds; r += 2) {
          A = ROL(A ^ B, B) + K[0];
          B = ROL(B ^ A, A) + K[1];
          A = ROL(A ^ B, B) + K[2];
          B = ROL(B ^ A, A) + K[3];
          K += 4;
      }
   } else {
      for (r = 0; r < skey->rounds; r++) {
          A = ROL(A ^ B, B) + K[0];
          B = ROL(B ^ A, A) + K[1];
          K += 2;
      }
   }
   STORE16L(A, &((char*)&ct)[0]);
   STORE16L(B, &((char*)&ct)[2]);

   return ct;
}


/**
  Decrypts a block of text with LTC_RC5
  @param ct The input ciphertext (8 bytes)
  @param pt The output plaintext (8 bytes)
  @param skey The key as scheduled
  @return CRYPT_OK if successful
*/

int rc5_ecb_decrypt(uint32_t ct, const struct rc5_32_key *skey)
{
   uint32_t pt;
   ulong16 A, B;
   const ulong16 *K;
   int r;

   LOAD16L(A, &((char*)&ct)[0]);
   LOAD16L(B, &((char*)&ct)[2]);
   K = skey->K + (skey->rounds << 1);

   if ((skey->rounds & 1) == 0) {
       K -= 2;
       for (r = skey->rounds - 1; r >= 0; r -= 2) {
          B = ROR(B - K[3], A) ^ A;
          A = ROR(A - K[2], B) ^ B;
          B = ROR(B - K[1], A) ^ A;
          A = ROR(A - K[0], B) ^ B;
          K -= 4;
        }
   } else {
      for (r = skey->rounds - 1; r >= 0; r--) {
          B = ROR(B - K[1], A) ^ A;
          A = ROR(A - K[0], B) ^ B;
          K -= 2;
      }
   }
   A -= skey->K[0];
   B -= skey->K[1];
   STORE16L(A, &((char*)&pt)[0]);
   STORE16L(B, &((char*)&pt)[2]);

   return pt;
}

