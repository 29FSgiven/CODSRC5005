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
  @file der_encode_integer.c
  ASN.1 DER, encode an integer, Tom St Denis
*/


#ifdef LTC_DER

/* Exports a positive bignum as DER format (upto 2^32 bytes in size) */
/**
  Store a mp_int integer
  @param num      The first mp_int to encode
  @param out      [out] The destination for the DER encoded integers
  @param outlen   [in/out] The max size and resulting size of the DER encoded integers
  @return CRYPT_OK if successful
*/
int der_encode_integer(void *num, unsigned char *out, unsigned long *outlen)
{
   unsigned long tmplen, y, len;
   int           err, leading_zero;

   LTC_ARGCHK(num    != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* find out how big this will be */
   if ((err = der_length_integer(num, &tmplen)) != CRYPT_OK) {
      return err;
   }

   if (*outlen < tmplen) {
      *outlen = tmplen;
      return CRYPT_BUFFER_OVERFLOW;
   }

   if (mp_cmp_d(num, 0) != LTC_MP_LT) {
      /* we only need a leading zero if the msb of the first byte is one */
      if ((mp_count_bits(num) & 7) == 0 || mp_iszero(num) == LTC_MP_YES) {
         leading_zero = 1;
      } else {
         leading_zero = 0;
      }

      /* get length of num in bytes (plus 1 since we force the msbyte to zero) */
      y = mp_unsigned_bin_size(num) + leading_zero;
   } else {
      leading_zero = 0;
      y            = mp_count_bits(num);
      y            = y + (8 - (y & 7));
      y            = y >> 3;
      if (((mp_cnt_lsb(num)+1)==mp_count_bits(num)) && ((mp_count_bits(num)&7)==0)) --y;
   }

   /* now store initial data */
   *out++ = 0x02;
   len = *outlen - 1;
   if ((err = der_encode_asn1_length(y, out, &len)) != CRYPT_OK) {
      return err;
   }
   out += len;

   /* now store msbyte of zero if num is non-zero */
   if (leading_zero) {
      *out++ = 0x00;
   }

   /* if it's not zero store it as big endian */
   if (mp_cmp_d(num, 0) == LTC_MP_GT) {
      /* now store the mpint */
      if ((err = mp_to_unsigned_bin(num, out)) != CRYPT_OK) {
          return err;
      }
   } else if (mp_iszero(num) != LTC_MP_YES) {
      void *tmp;

      /* negative */
      if (mp_init(&tmp) != CRYPT_OK) {
         return CRYPT_MEM;
      }

      /* 2^roundup and subtract */
      y = mp_count_bits(num);
      y = y + (8 - (y & 7));
      if (((mp_cnt_lsb(num)+1)==mp_count_bits(num)) && ((mp_count_bits(num)&7)==0)) y -= 8;
      if (mp_2expt(tmp, y) != CRYPT_OK || mp_add(tmp, num, tmp) != CRYPT_OK) {
         mp_clear(tmp);
         return CRYPT_MEM;
      }
      if ((err = mp_to_unsigned_bin(tmp, out)) != CRYPT_OK) {
         mp_clear(tmp);
         return err;
      }
      mp_clear(tmp);
   }

   /* we good */
   *outlen = tmplen;
   return CRYPT_OK;
}

#endif

/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */
