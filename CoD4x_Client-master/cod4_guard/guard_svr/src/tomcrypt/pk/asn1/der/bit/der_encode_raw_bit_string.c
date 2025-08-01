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
  @file der_encode_bit_string.c
  ASN.1 DER, encode a BIT STRING, Tom St Denis
*/


#ifdef LTC_DER

#define getbit(n, k) (((n) & ( 1 << (k) )) >> (k))

/**
  Store a BIT STRING
  @param in       The array of bits to store (8 per char)
  @param inlen    The number of bits to store
  @param out      [out] The destination for the DER encoded BIT STRING
  @param outlen   [in/out] The max size and resulting size of the DER BIT STRING
  @return CRYPT_OK if successful
*/
int der_encode_raw_bit_string(const unsigned char *in, unsigned long inlen,
                                unsigned char *out, unsigned long *outlen)
{
   unsigned long len, x, y;
   unsigned char buf;
   int           err;

   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* avoid overflows */
   if ((err = der_length_bit_string(inlen, &len)) != CRYPT_OK) {
      return err;
   }

   if (len > *outlen) {
      *outlen = len;
      return CRYPT_BUFFER_OVERFLOW;
   }

   /* store header (include bit padding count in length) */
   x = 0;
   y = ((inlen + 7) >> 3) + 1;

   out[x++] = 0x03;
   len = *outlen - x;
   if ((err = der_encode_asn1_length(y, out + x, &len)) != CRYPT_OK) {
      return err;
   }
   x += len;

   /* store number of zero padding bits */
   out[x++] = (unsigned char)((8 - inlen) & 7);

   /* store the bits in big endian format */
   for (y = buf = 0; y < inlen; y++) {
      buf |= (getbit(in[y/8],7-y%8)?1:0) << (7 - (y & 7));
      if ((y & 7) == 7) {
         out[x++] = buf;
         buf      = 0;
      }
   }
   /* store last byte */
   if (inlen & 7) {
      out[x++] = buf;
   }

   *outlen = x;
   return CRYPT_OK;
}

#endif

/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */
