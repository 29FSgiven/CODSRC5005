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
  @file der_length_utf8_string.c
  ASN.1 DER, get length of UTF8 STRING, Tom St Denis
*/

#ifdef LTC_DER

/** Return the size in bytes of a UTF-8 character
  @param c   The UTF-8 character to measure
  @return    The size in bytes
*/
unsigned long der_utf8_charsize(const wchar_t c)
{
   if (c <= 0x7F) {
      return 1;
   }
   if (c <= 0x7FF) {
      return 2;
   }
#if LTC_WCHAR_MAX == 0xFFFF
   return 3;
#else
   if (c <= 0xFFFF) {
      return 3;
   }
   return 4;
#endif
}

/**
  Test whether the given code point is valid character
  @param c   The UTF-8 character to test
  @return    1 - valid, 0 - invalid
*/
int der_utf8_valid_char(const wchar_t c)
{
   LTC_UNUSED_PARAM(c);
#if !defined(LTC_WCHAR_MAX) || LTC_WCHAR_MAX > 0xFFFF
   if (c > 0x10FFFF) return 0;
#endif
#if LTC_WCHAR_MAX != 0xFFFF && LTC_WCHAR_MAX != 0xFFFFFFFF
   if (c < 0) return 0;
#endif
   return 1;
}

/**
  Gets length of DER encoding of UTF8 STRING
  @param in       The characters to measure the length of
  @param noctets  The number of octets in the string to encode
  @param outlen   [out] The length of the DER encoding for the given string
  @return CRYPT_OK if successful
*/
int der_length_utf8_string(const wchar_t *in, unsigned long noctets, unsigned long *outlen)
{
   unsigned long x, len;
   int err;

   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(outlen != NULL);

   len = 0;
   for (x = 0; x < noctets; x++) {
      if (!der_utf8_valid_char(in[x])) return CRYPT_INVALID_ARG;
      len += der_utf8_charsize(in[x]);
   }

   if ((err = der_length_asn1_length(len, &x)) != CRYPT_OK) {
      return err;
   }
   *outlen = 1 + x + len;

   return CRYPT_OK;
}

#endif


/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */
