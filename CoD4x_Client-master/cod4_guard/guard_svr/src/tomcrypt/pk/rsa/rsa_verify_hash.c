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
  @file rsa_verify_hash.c
  RSA PKCS #1 v1.5 or v2 PSS signature verification, Tom St Denis and Andreas Lange
*/

#ifdef LTC_MRSA

/**
  PKCS #1 de-sign then v1.5 or PSS depad
  @param sig              The signature data
  @param siglen           The length of the signature data (octets)
  @param hash             The hash of the message that was signed
  @param hashlen          The length of the hash of the message that was signed (octets)
  @param padding          Type of padding (LTC_PKCS_1_PSS, LTC_PKCS_1_V1_5 or LTC_PKCS_1_V1_5_NA1)
  @param hash_idx         The index of the desired hash
  @param saltlen          The length of the salt used during signature
  @param stat             [out] The result of the signature comparison, 1==valid, 0==invalid
  @param key              The public RSA key corresponding to the key that performed the signature
  @return CRYPT_OK on success (even if the signature is invalid)
*/
int rsa_verify_hash_ex(const unsigned char *sig,            unsigned long  siglen,
                       const unsigned char *hash,           unsigned long  hashlen,
                             int            padding,
                             int            hash_idx,       unsigned long  saltlen,
                             int           *stat,     const rsa_key       *key)
{
  unsigned long modulus_bitlen, modulus_bytelen, x;
  int           err;
  unsigned char *tmpbuf;

  LTC_ARGCHK(hash  != NULL);
  LTC_ARGCHK(sig   != NULL);
  LTC_ARGCHK(stat  != NULL);
  LTC_ARGCHK(key   != NULL);

  /* default to invalid */
  *stat = 0;

  /* valid padding? */

  if ((padding != LTC_PKCS_1_V1_5) &&
      (padding != LTC_PKCS_1_PSS) &&
      (padding != LTC_PKCS_1_V1_5_NA1)) {
    return CRYPT_PK_INVALID_PADDING;
  }

  if (padding != LTC_PKCS_1_V1_5_NA1) {
    /* valid hash ? */
    if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
       return err;
    }
  }

  /* get modulus len in bits */
  modulus_bitlen = mp_count_bits( (key->N));

  /* outlen must be at least the size of the modulus */
  modulus_bytelen = mp_unsigned_bin_size( (key->N));
  if (modulus_bytelen != siglen) {
     return CRYPT_INVALID_PACKET;
  }

  /* allocate temp buffer for decoded sig */
  tmpbuf = XMALLOC(siglen);
  if (tmpbuf == NULL) {
     return CRYPT_MEM;
  }

  /* RSA decode it  */
  x = siglen;
  if ((err = ltc_mp.rsa_me(sig, siglen, tmpbuf, &x, PK_PUBLIC, key)) != CRYPT_OK) {
     XFREE(tmpbuf);
     return err;
  }

  /* make sure the output is the right size */
  if (x != siglen) {
     XFREE(tmpbuf);
     return CRYPT_INVALID_PACKET;
  }

  if (padding == LTC_PKCS_1_PSS) {
    /* PSS decode and verify it */

    if(modulus_bitlen%8 == 1){
      err = pkcs_1_pss_decode(hash, hashlen, tmpbuf+1, x-1, saltlen, hash_idx, modulus_bitlen, stat);
    }
    else{
      err = pkcs_1_pss_decode(hash, hashlen, tmpbuf, x, saltlen, hash_idx, modulus_bitlen, stat);
    }

  } else {
    /* PKCS #1 v1.5 decode it */
    unsigned char *out;
    unsigned long outlen;
    int           decoded;

    /* allocate temp buffer for decoded hash */
    outlen = ((modulus_bitlen >> 3) + (modulus_bitlen & 7 ? 1 : 0)) - 3;
    out    = XMALLOC(outlen);
    if (out == NULL) {
      err = CRYPT_MEM;
      goto bail_2;
    }

    if ((err = pkcs_1_v1_5_decode(tmpbuf, x, LTC_PKCS_1_EMSA, modulus_bitlen, out, &outlen, &decoded)) != CRYPT_OK) {
      XFREE(out);
      goto bail_2;
    }

    if (padding == LTC_PKCS_1_V1_5) {
      unsigned long loid[16], reallen;
      ltc_asn1_list digestinfo[2], siginfo[2];

      /* not all hashes have OIDs... so sad */
      if (hash_descriptor[hash_idx].OIDlen == 0) {
         err = CRYPT_INVALID_ARG;
         goto bail_2;
      }

      /* now we must decode out[0...outlen-1] using ASN.1, test the OID and then test the hash */
      /* construct the SEQUENCE
        SEQUENCE {
           SEQUENCE {hashoid OID
                     blah    NULL
           }
           hash    OCTET STRING
        }
     */
      LTC_SET_ASN1(digestinfo, 0, LTC_ASN1_OBJECT_IDENTIFIER, loid, sizeof(loid)/sizeof(loid[0]));
      LTC_SET_ASN1(digestinfo, 1, LTC_ASN1_NULL,              NULL,                          0);
      LTC_SET_ASN1(siginfo,    0, LTC_ASN1_SEQUENCE,          digestinfo,                    2);
      LTC_SET_ASN1(siginfo,    1, LTC_ASN1_OCTET_STRING,      tmpbuf,                        siglen);

      if ((err = der_decode_sequence_strict(out, outlen, siginfo, 2)) != CRYPT_OK) {
         /* fallback to Legacy:missing NULL */
         LTC_SET_ASN1(siginfo, 0, LTC_ASN1_SEQUENCE,          digestinfo,                    1);
         if ((err = der_decode_sequence_strict(out, outlen, siginfo, 2)) != CRYPT_OK) {
           XFREE(out);
           goto bail_2;
         }
      }

      if ((err = der_length_sequence(siginfo, 2, &reallen)) != CRYPT_OK) {
         XFREE(out);
         goto bail_2;
      }

      /* test OID */
      if ((reallen == outlen) &&
          (digestinfo[0].size == hash_descriptor[hash_idx].OIDlen) &&
        (XMEMCMP(digestinfo[0].data, hash_descriptor[hash_idx].OID, sizeof(unsigned long) * hash_descriptor[hash_idx].OIDlen) == 0) &&
          (siginfo[1].size == hashlen) &&
        (XMEMCMP(siginfo[1].data, hash, hashlen) == 0)) {
         *stat = 1;
      }
    } else {
      /* only check if the hash is equal */
      if ((hashlen == outlen) &&
          (XMEMCMP(out, hash, hashlen) == 0)) {
        *stat = 1;
      }
    }

#ifdef LTC_CLEAN_STACK
    zeromem(out, outlen);
#endif
    XFREE(out);
  }

bail_2:
#ifdef LTC_CLEAN_STACK
  zeromem(tmpbuf, siglen);
#endif
  XFREE(tmpbuf);
  return err;
}

#endif /* LTC_MRSA */

/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */
