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
  @file der_encode_sequence_ex.c
  ASN.1 DER, encode a SEQUENCE, Tom St Denis
*/

#ifdef LTC_DER

/**
   Encode a SEQUENCE
   @param list      The list of items to encode
   @param inlen     The number of items in the list
   @param out       [out] The destination
   @param outlen    [in/out] The size of the output
   @param type_of   LTC_ASN1_SEQUENCE or LTC_ASN1_SET/LTC_ASN1_SETOF
   @return CRYPT_OK on success
*/
int der_encode_sequence_ex(const ltc_asn1_list *list, unsigned long inlen,
                           unsigned char *out,        unsigned long *outlen, int type_of)
{
   int           err;
   ltc_asn1_type type;
   unsigned long size, x, y, z, i;
   void          *data;

   LTC_ARGCHK(list    != NULL);
   LTC_ARGCHK(out     != NULL);
   LTC_ARGCHK(outlen  != NULL);

   /* get size of output that will be required */
   y = 0; z = 0;
   if ((err = der_length_sequence_ex(list, inlen, &y, &z)) != CRYPT_OK) return CRYPT_INVALID_ARG;

   /* too big ? */
   if (*outlen < y) {
      *outlen = y;
      err = CRYPT_BUFFER_OVERFLOW;
      goto LBL_ERR;
   }

   /* store header */
   x = 0;
   out[x++] = (type_of == LTC_ASN1_SEQUENCE) ? 0x30 : 0x31;

   y = *outlen - x;
   if ((err = der_encode_asn1_length(z, &out[x], &y)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   x += y;

   /* store data */
   *outlen -= x;
   for (i = 0; i < inlen; i++) {
       type = list[i].type;
       size = list[i].size;
       data = list[i].data;

       if (type == LTC_ASN1_EOL) {
          break;
       }

       switch (type) {
            case LTC_ASN1_BOOLEAN:
               z = *outlen;
               if ((err = der_encode_boolean(*((int *)data), out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_INTEGER:
               z = *outlen;
               if ((err = der_encode_integer(data, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_SHORT_INTEGER:
               z = *outlen;
               if ((err = der_encode_short_integer(*((unsigned long*)data), out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_BIT_STRING:
               z = *outlen;
               if ((err = der_encode_bit_string(data, size, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_RAW_BIT_STRING:
               z = *outlen;
               if ((err = der_encode_raw_bit_string(data, size, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_OCTET_STRING:
               z = *outlen;
               if ((err = der_encode_octet_string(data, size, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_NULL:
               out[x] = 0x05;
               out[x+1] = 0x00;
               z = 2;
               break;

           case LTC_ASN1_OBJECT_IDENTIFIER:
               z = *outlen;
               if ((err = der_encode_object_identifier(data, size, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_IA5_STRING:
               z = *outlen;
               if ((err = der_encode_ia5_string(data, size, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_PRINTABLE_STRING:
               z = *outlen;
               if ((err = der_encode_printable_string(data, size, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_UTF8_STRING:
               z = *outlen;
               if ((err = der_encode_utf8_string(data, size, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_UTCTIME:
               z = *outlen;
               if ((err = der_encode_utctime(data, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_GENERALIZEDTIME:
               z = *outlen;
               if ((err = der_encode_generalizedtime(data, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_SET:
               z = *outlen;
               if ((err = der_encode_set(data, size, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_SETOF:
               z = *outlen;
               if ((err = der_encode_setof(data, size, out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_SEQUENCE:
               z = *outlen;
               if ((err = der_encode_sequence_ex(data, size, out + x, &z, type)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_CUSTOM_TYPE:
               z = *outlen;
               if ((err = der_encode_custom_type(&list[i], out + x, &z)) != CRYPT_OK) {
                  goto LBL_ERR;
               }
               break;

           case LTC_ASN1_CHOICE:
           case LTC_ASN1_EOL:
           case LTC_ASN1_TELETEX_STRING:
               err = CRYPT_INVALID_ARG;
               goto LBL_ERR;
       }

       x       += z;
       *outlen -= z;
   }
   *outlen = x;
   err = CRYPT_OK;

LBL_ERR:
   return err;
}

#endif

/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */
