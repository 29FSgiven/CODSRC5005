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
  @file rsa_exptmod.c
  RSA PKCS exptmod, Tom St Denis
  Added RSA blinding --nmav
*/

#ifdef LTC_MRSA

/**
   Compute an RSA modular exponentiation
   @param in         The input data to send into RSA
   @param inlen      The length of the input (octets)
   @param out        [out] The destination
   @param outlen     [in/out] The max size and resulting size of the output
   @param which      Which exponent to use, e.g. PK_PRIVATE or PK_PUBLIC
   @param key        The RSA key to use
   @return CRYPT_OK if successful
*/
int rsa_exptmod(const unsigned char *in,   unsigned long inlen,
                      unsigned char *out,  unsigned long *outlen, int which,
                const rsa_key *key)
{
   void        *tmp, *tmpa, *tmpb;
   #ifdef LTC_RSA_BLINDING
   void        *rnd, *rndi /* inverse of rnd */;
   #endif
   unsigned long x;
   int           err, has_crt_parameters;

   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
   LTC_ARGCHK(key    != NULL);

   /* is the key of the right type for the operation? */
   if (which == PK_PRIVATE && (key->type != PK_PRIVATE)) {
      return CRYPT_PK_NOT_PRIVATE;
   }

   /* must be a private or public operation */
   if (which != PK_PRIVATE && which != PK_PUBLIC) {
      return CRYPT_PK_INVALID_TYPE;
   }

   /* init and copy into tmp */
   if ((err = mp_init_multi(&tmp, &tmpa, &tmpb,
#ifdef LTC_RSA_BLINDING
                                               &rnd, &rndi,
#endif /* LTC_RSA_BLINDING */
                                                           NULL)) != CRYPT_OK)
        { return err; }
   if ((err = mp_read_unsigned_bin(tmp, (unsigned char *)in, (int)inlen)) != CRYPT_OK)
        { goto error; }


   /* sanity check on the input */
   if (mp_cmp(key->N, tmp) == LTC_MP_LT) {
      err = CRYPT_PK_INVALID_SIZE;
      goto error;
   }

   /* are we using the private exponent and is the key optimized? */
   if (which == PK_PRIVATE) {
      #ifdef LTC_RSA_BLINDING
      /* do blinding */
      err = mp_rand(rnd, mp_get_digit_count(key->N));
      if (err != CRYPT_OK) {
             goto error;
      }

      /* rndi = 1/rnd mod N */
      err = mp_invmod(rnd, key->N, rndi);
      if (err != CRYPT_OK) {
             goto error;
      }

      /* rnd = rnd^e */
      err = mp_exptmod( rnd, key->e, key->N, rnd);
      if (err != CRYPT_OK) {
             goto error;
      }

      /* tmp = tmp*rnd mod N */
      err = mp_mulmod( tmp, rnd, key->N, tmp);
      if (err != CRYPT_OK) {
             goto error;
      }
      #endif /* LTC_RSA_BLINDING */

      has_crt_parameters = (key->p != NULL) && (mp_get_digit_count(key->p) != 0) &&
                              (key->q != NULL) && (mp_get_digit_count(key->q) != 0) &&
                                 (key->dP != NULL) && (mp_get_digit_count(key->dP) != 0) &&
                                    (key->dQ != NULL) && (mp_get_digit_count(key->dQ) != 0) &&
                                       (key->qP != NULL) && (mp_get_digit_count(key->qP) != 0);

      if (!has_crt_parameters) {
         /*
          * In case CRT optimization parameters are not provided,
          * the private key is directly used to exptmod it
          */
         if ((err = mp_exptmod(tmp, key->d, key->N, tmp)) != CRYPT_OK)                              { goto error; }
      } else {
         /* tmpa = tmp^dP mod p */
         if ((err = mp_exptmod(tmp, key->dP, key->p, tmpa)) != CRYPT_OK)                            { goto error; }

         /* tmpb = tmp^dQ mod q */
         if ((err = mp_exptmod(tmp, key->dQ, key->q, tmpb)) != CRYPT_OK)                            { goto error; }

         /* tmp = (tmpa - tmpb) * qInv (mod p) */
         if ((err = mp_sub(tmpa, tmpb, tmp)) != CRYPT_OK)                                           { goto error; }
         if ((err = mp_mulmod(tmp, key->qP, key->p, tmp)) != CRYPT_OK)                              { goto error; }

         /* tmp = tmpb + q * tmp */
         if ((err = mp_mul(tmp, key->q, tmp)) != CRYPT_OK)                                          { goto error; }
         if ((err = mp_add(tmp, tmpb, tmp)) != CRYPT_OK)                                            { goto error; }
      }

      #ifdef LTC_RSA_BLINDING
      /* unblind */
      err = mp_mulmod( tmp, rndi, key->N, tmp);
      if (err != CRYPT_OK) {
             goto error;
      }
      #endif

      #ifdef LTC_RSA_CRT_HARDENING
      if (has_crt_parameters) {
         if ((err = mp_exptmod(tmp, key->e, key->N, tmpa)) != CRYPT_OK)                              { goto error; }
         if ((err = mp_read_unsigned_bin(tmpb, (unsigned char *)in, (int)inlen)) != CRYPT_OK)        { goto error; }
         if (mp_cmp(tmpa, tmpb) != LTC_MP_EQ)                                     { err = CRYPT_ERROR; goto error; }
      }
      #endif
   } else {
      /* exptmod it */
      if ((err = mp_exptmod(tmp, key->e, key->N, tmp)) != CRYPT_OK)                                { goto error; }
   }

   /* read it back */
   x = (unsigned long)mp_unsigned_bin_size(key->N);
   if (x > *outlen) {
      *outlen = x;
      err = CRYPT_BUFFER_OVERFLOW;
      goto error;
   }

   /* this should never happen ... */
   if (mp_unsigned_bin_size(tmp) > mp_unsigned_bin_size(key->N)) {
      err = CRYPT_ERROR;
      goto error;
   }
   *outlen = x;

   /* convert it */
   zeromem(out, x);
   if ((err = mp_to_unsigned_bin(tmp, out+(x-mp_unsigned_bin_size(tmp)))) != CRYPT_OK)               { goto error; }

   /* clean up and return */
   err = CRYPT_OK;
error:
   mp_clear_multi(
#ifdef LTC_RSA_BLINDING
                  rndi, rnd,
#endif /* LTC_RSA_BLINDING */
                             tmpb, tmpa, tmp, NULL);
   return err;
}

#endif

/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */
