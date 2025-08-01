/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 */
#include "tomcrypt_private.h"

/**
  @file der_length_boolean.c
  ASN.1 DER, get length of a BOOLEAN, Tom St Denis
*/

#ifdef LTC_DER
/**
  Gets length of DER encoding of a BOOLEAN
  @param outlen [out] The length of the DER encoding
  @return CRYPT_OK if successful
*/
int der_length_boolean(unsigned long *outlen)
{
   LTC_ARGCHK(outlen != NULL);
   *outlen = 3;
   return CRYPT_OK;
}

#endif

/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */
