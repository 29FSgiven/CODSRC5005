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
  @file ltc_ecc_mul2add.c
  ECC Crypto, Shamir's Trick, Tom St Denis
*/

#ifdef LTC_MECC

#ifdef LTC_ECC_SHAMIR

/** Computes kA*A + kB*B = C using Shamir's Trick
  @param A        First point to multiply
  @param kA       What to multiple A by
  @param B        Second point to multiply
  @param kB       What to multiple B by
  @param C        [out] Destination point (can overlap with A or B)
  @param ma       ECC curve parameter a in montgomery form
  @param modulus  Modulus for curve
  @return CRYPT_OK on success
*/
int ltc_ecc_mul2add(const ecc_point *A, void *kA,
                    const ecc_point *B, void *kB,
                          ecc_point *C,
                               void *ma,
                               void *modulus)
{
  ecc_point     *precomp[16];
  unsigned       bitbufA, bitbufB, lenA, lenB, len, nA, nB, nibble;
  unsigned       x, y;
  unsigned char *tA, *tB;
  int            err, first;
  void          *mp, *mu;

  /* argchks */
  LTC_ARGCHK(A       != NULL);
  LTC_ARGCHK(B       != NULL);
  LTC_ARGCHK(C       != NULL);
  LTC_ARGCHK(kA      != NULL);
  LTC_ARGCHK(kB      != NULL);
  LTC_ARGCHK(modulus != NULL);

  /* allocate memory */
  tA = XCALLOC(1, ECC_BUF_SIZE);
  if (tA == NULL) {
     return CRYPT_MEM;
  }
  tB = XCALLOC(1, ECC_BUF_SIZE);
  if (tB == NULL) {
     XFREE(tA);
     return CRYPT_MEM;
  }

  /* get sizes */
  lenA = mp_unsigned_bin_size(kA);
  lenB = mp_unsigned_bin_size(kB);
  len  = MAX(lenA, lenB);

  /* sanity check */
  if ((lenA > ECC_BUF_SIZE) || (lenB > ECC_BUF_SIZE)) {
     err = CRYPT_INVALID_ARG;
     goto ERR_T;
  }

  /* extract and justify kA */
  mp_to_unsigned_bin(kA, (len - lenA) + tA);

  /* extract and justify kB */
  mp_to_unsigned_bin(kB, (len - lenB) + tB);

  /* allocate the table */
  for (x = 0; x < 16; x++) {
     precomp[x] = ltc_ecc_new_point();
     if (precomp[x] == NULL) {
         for (y = 0; y < x; ++y) {
            ltc_ecc_del_point(precomp[y]);
         }
         err = CRYPT_MEM;
         goto ERR_T;
     }
  }

  /* init montgomery reduction */
  if ((err = mp_montgomery_setup(modulus, &mp)) != CRYPT_OK) {
      goto ERR_P;
  }
  if ((err = mp_init(&mu)) != CRYPT_OK) {
      goto ERR_MP;
  }
  if ((err = mp_montgomery_normalization(mu, modulus)) != CRYPT_OK) {
      goto ERR_MU;
  }

  /* copy ones ... */
  if ((err = mp_mulmod(A->x, mu, modulus, precomp[1]->x)) != CRYPT_OK)                                         { goto ERR_MU; }
  if ((err = mp_mulmod(A->y, mu, modulus, precomp[1]->y)) != CRYPT_OK)                                         { goto ERR_MU; }
  if ((err = mp_mulmod(A->z, mu, modulus, precomp[1]->z)) != CRYPT_OK)                                         { goto ERR_MU; }

  if ((err = mp_mulmod(B->x, mu, modulus, precomp[1<<2]->x)) != CRYPT_OK)                                      { goto ERR_MU; }
  if ((err = mp_mulmod(B->y, mu, modulus, precomp[1<<2]->y)) != CRYPT_OK)                                      { goto ERR_MU; }
  if ((err = mp_mulmod(B->z, mu, modulus, precomp[1<<2]->z)) != CRYPT_OK)                                      { goto ERR_MU; }

  /* precomp [i,0](A + B) table */
  if ((err = ltc_mp.ecc_ptdbl(precomp[1], precomp[2], ma, modulus, mp)) != CRYPT_OK)                           { goto ERR_MU; }
  if ((err = ltc_mp.ecc_ptadd(precomp[1], precomp[2], precomp[3], ma, modulus, mp)) != CRYPT_OK)               { goto ERR_MU; }

  /* precomp [0,i](A + B) table */
  if ((err = ltc_mp.ecc_ptdbl(precomp[1<<2], precomp[2<<2], ma, modulus, mp)) != CRYPT_OK)                     { goto ERR_MU; }
  if ((err = ltc_mp.ecc_ptadd(precomp[1<<2], precomp[2<<2], precomp[3<<2], ma, modulus, mp)) != CRYPT_OK)      { goto ERR_MU; }

  /* precomp [i,j](A + B) table (i != 0, j != 0) */
  for (x = 1; x < 4; x++) {
     for (y = 1; y < 4; y++) {
        if ((err = ltc_mp.ecc_ptadd(precomp[x], precomp[(y<<2)], precomp[x+(y<<2)], ma, modulus, mp)) != CRYPT_OK) { goto ERR_MU; }
     }
  }

  nibble  = 3;
  first   = 1;
  bitbufA = tA[0];
  bitbufB = tB[0];

  /* for every byte of the multiplicands */
  for (x = 0;; ) {
     /* grab a nibble */
     if (++nibble == 4) {
        if (x == len) break;
        bitbufA = tA[x];
        bitbufB = tB[x];
        nibble  = 0;
        ++x;
     }

     /* extract two bits from both, shift/update */
     nA = (bitbufA >> 6) & 0x03;
     nB = (bitbufB >> 6) & 0x03;
     bitbufA = (bitbufA << 2) & 0xFF;
     bitbufB = (bitbufB << 2) & 0xFF;

     /* if both zero, if first, continue */
     if ((nA == 0) && (nB == 0) && (first == 1)) {
        continue;
     }

     /* double twice, only if this isn't the first */
     if (first == 0) {
        /* double twice */
        if ((err = ltc_mp.ecc_ptdbl(C, C, ma, modulus, mp)) != CRYPT_OK)              { goto ERR_MU; }
        if ((err = ltc_mp.ecc_ptdbl(C, C, ma, modulus, mp)) != CRYPT_OK)              { goto ERR_MU; }
     }

     /* if not both zero */
     if ((nA != 0) || (nB != 0)) {
        if (first == 1) {
           /* if first, copy from table */
           first = 0;
           if ((err = ltc_ecc_copy_point(precomp[nA + (nB<<2)], C)) != CRYPT_OK)      { goto ERR_MU; }
        } else {
           /* if not first, add from table */
           if ((err = ltc_mp.ecc_ptadd(C, precomp[nA + (nB<<2)], C, ma, modulus, mp)) != CRYPT_OK) { goto ERR_MU; }
        }
     }
  }

  /* reduce to affine */
  err = ltc_ecc_map(C, modulus, mp);

  /* clean up */
ERR_MU:
   mp_clear(mu);
ERR_MP:
   mp_montgomery_free(mp);
ERR_P:
   for (x = 0; x < 16; x++) {
       ltc_ecc_del_point(precomp[x]);
   }
ERR_T:
#ifdef LTC_CLEAN_STACK
   zeromem(tA, ECC_BUF_SIZE);
   zeromem(tB, ECC_BUF_SIZE);
#endif
   XFREE(tA);
   XFREE(tB);

   return err;
}

#endif
#endif

/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */
