/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 */

#include "tomcrypt_private.h"

/* ### Point doubling in Jacobian coordinate system ###
 *
 * let us have a curve:                 y^2 = x^3 + a*x + b
 * in Jacobian coordinates it becomes:  y^2 = x^3 + a*x*z^4 + b*z^6
 *
 * The doubling of P = (Xp, Yp, Zp) is given by R = (Xr, Yr, Zr) where:
 * Xr = M^2 - 2*S
 * Yr = M * (S - Xr) - 8*T
 * Zr = 2 * Yp * Zp
 *
 * M = 3 * Xp^2 + a*Zp^4
 * T = Yp^4
 * S = 4 * Xp * Yp^2
 *
 * SPECIAL CASE: when a == -3 we can compute M as
 * M = 3 * (Xp^2 - Zp^4) = 3 * (Xp + Zp^2) * (Xp - Zp^2)
 */

/**
  @file ltc_ecc_projective_dbl_point.c
  ECC Crypto, Tom St Denis
*/

#if defined(LTC_MECC) && (!defined(LTC_MECC_ACCEL) || defined(LTM_DESC))

/**
   Double an ECC point
   @param P   The point to double
   @param R   [out] The destination of the double
   @param ma  ECC curve parameter a in montgomery form
   @param modulus  The modulus of the field the ECC curve is in
   @param mp       The "b" value from montgomery_setup()
   @return CRYPT_OK on success
*/
int ltc_ecc_projective_dbl_point(const ecc_point *P, ecc_point *R, void *ma, void *modulus, void *mp)
{
   void *t1, *t2;
   int   err, inf;

   LTC_ARGCHK(P       != NULL);
   LTC_ARGCHK(R       != NULL);
   LTC_ARGCHK(modulus != NULL);
   LTC_ARGCHK(mp      != NULL);

   if ((err = mp_init_multi(&t1, &t2, NULL)) != CRYPT_OK) {
      return err;
   }

   if (P != R) {
      if ((err = ltc_ecc_copy_point(P, R)) != CRYPT_OK)                           { goto done; }
   }

   if ((err = ltc_ecc_is_point_at_infinity(P, modulus, &inf)) != CRYPT_OK) return err;
   if (inf) {
      /* if P is point at infinity >> Result = point at infinity */
      err = ltc_ecc_set_point_xyz(1, 1, 0, R);
      goto done;
   }

   /* t1 = Z * Z */
   if ((err = mp_sqr(R->z, t1)) != CRYPT_OK)                                      { goto done; }
   if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK)                 { goto done; }
   /* Z = Y * Z */
   if ((err = mp_mul(R->z, R->y, R->z)) != CRYPT_OK)                              { goto done; }
   if ((err = mp_montgomery_reduce(R->z, modulus, mp)) != CRYPT_OK)               { goto done; }
   /* Z = 2Z */
   if ((err = mp_add(R->z, R->z, R->z)) != CRYPT_OK)                              { goto done; }
   if (mp_cmp(R->z, modulus) != LTC_MP_LT) {
      if ((err = mp_sub(R->z, modulus, R->z)) != CRYPT_OK)                        { goto done; }
   }

   if (ma == NULL) { /* special case for curves with a == -3 (10% faster than general case) */
      /* T2 = X - T1 */
      if ((err = mp_sub(R->x, t1, t2)) != CRYPT_OK)                               { goto done; }
      if (mp_cmp_d(t2, 0) == LTC_MP_LT) {
         if ((err = mp_add(t2, modulus, t2)) != CRYPT_OK)                         { goto done; }
      }
      /* T1 = X + T1 */
      if ((err = mp_add(t1, R->x, t1)) != CRYPT_OK)                               { goto done; }
      if (mp_cmp(t1, modulus) != LTC_MP_LT) {
         if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK)                         { goto done; }
      }
      /* T2 = T1 * T2 */
      if ((err = mp_mul(t1, t2, t2)) != CRYPT_OK)                                 { goto done; }
      if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK)              { goto done; }
      /* T1 = 2T2 */
      if ((err = mp_add(t2, t2, t1)) != CRYPT_OK)                                 { goto done; }
      if (mp_cmp(t1, modulus) != LTC_MP_LT) {
         if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK)                         { goto done; }
      }
      /* T1 = T1 + T2 */
      if ((err = mp_add(t1, t2, t1)) != CRYPT_OK)                                 { goto done; }
      if (mp_cmp(t1, modulus) != LTC_MP_LT) {
         if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK)                         { goto done; }
      }
   }
   else {
      /* T2 = T1 * T1 */
      if ((err = mp_sqr(t1, t2)) != CRYPT_OK)                                     { goto done; }
      if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK)              { goto done; }
      /* T1 = T2 * a */
      if ((err = mp_mul(t2, ma, t1)) != CRYPT_OK)                                 { goto done; }
      if ((err = mp_montgomery_reduce(t1, modulus, mp)) != CRYPT_OK)              { goto done; }
      /* T2 = X * X */
      if ((err = mp_sqr(R->x, t2)) != CRYPT_OK)                                   { goto done; }
      if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK)              { goto done; }
      /* T1 = T2 + T1 */
      if ((err = mp_add(t1, t2, t1)) != CRYPT_OK)                                 { goto done; }
      if (mp_cmp(t1, modulus) != LTC_MP_LT) {
         if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK)                         { goto done; }
      }
      /* T1 = T2 + T1 */
      if ((err = mp_add(t1, t2, t1)) != CRYPT_OK)                                 { goto done; }
      if (mp_cmp(t1, modulus) != LTC_MP_LT) {
         if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK)                         { goto done; }
      }
      /* T1 = T2 + T1 */
      if ((err = mp_add(t1, t2, t1)) != CRYPT_OK)                                 { goto done; }
      if (mp_cmp(t1, modulus) != LTC_MP_LT) {
         if ((err = mp_sub(t1, modulus, t1)) != CRYPT_OK)                         { goto done; }
      }
   }

   /* Y = 2Y */
   if ((err = mp_add(R->y, R->y, R->y)) != CRYPT_OK)                              { goto done; }
   if (mp_cmp(R->y, modulus) != LTC_MP_LT) {
      if ((err = mp_sub(R->y, modulus, R->y)) != CRYPT_OK)                        { goto done; }
   }
   /* Y = Y * Y */
   if ((err = mp_sqr(R->y, R->y)) != CRYPT_OK)                                    { goto done; }
   if ((err = mp_montgomery_reduce(R->y, modulus, mp)) != CRYPT_OK)               { goto done; }
   /* T2 = Y * Y */
   if ((err = mp_sqr(R->y, t2)) != CRYPT_OK)                                      { goto done; }
   if ((err = mp_montgomery_reduce(t2, modulus, mp)) != CRYPT_OK)                 { goto done; }
   /* T2 = T2/2 */
   if (mp_isodd(t2)) {
      if ((err = mp_add(t2, modulus, t2)) != CRYPT_OK)                            { goto done; }
   }
   if ((err = mp_div_2(t2, t2)) != CRYPT_OK)                                      { goto done; }
   /* Y = Y * X */
   if ((err = mp_mul(R->y, R->x, R->y)) != CRYPT_OK)                              { goto done; }
   if ((err = mp_montgomery_reduce(R->y, modulus, mp)) != CRYPT_OK)               { goto done; }

   /* X  = T1 * T1 */
   if ((err = mp_sqr(t1, R->x)) != CRYPT_OK)                                      { goto done; }
   if ((err = mp_montgomery_reduce(R->x, modulus, mp)) != CRYPT_OK)               { goto done; }
   /* X = X - Y */
   if ((err = mp_sub(R->x, R->y, R->x)) != CRYPT_OK)                              { goto done; }
   if (mp_cmp_d(R->x, 0) == LTC_MP_LT) {
      if ((err = mp_add(R->x, modulus, R->x)) != CRYPT_OK)                        { goto done; }
   }
   /* X = X - Y */
   if ((err = mp_sub(R->x, R->y, R->x)) != CRYPT_OK)                              { goto done; }
   if (mp_cmp_d(R->x, 0) == LTC_MP_LT) {
      if ((err = mp_add(R->x, modulus, R->x)) != CRYPT_OK)                        { goto done; }
   }

   /* Y = Y - X */
   if ((err = mp_sub(R->y, R->x, R->y)) != CRYPT_OK)                              { goto done; }
   if (mp_cmp_d(R->y, 0) == LTC_MP_LT) {
      if ((err = mp_add(R->y, modulus, R->y)) != CRYPT_OK)                        { goto done; }
   }
   /* Y = Y * T1 */
   if ((err = mp_mul(R->y, t1, R->y)) != CRYPT_OK)                                { goto done; }
   if ((err = mp_montgomery_reduce(R->y, modulus, mp)) != CRYPT_OK)               { goto done; }
   /* Y = Y - T2 */
   if ((err = mp_sub(R->y, t2, R->y)) != CRYPT_OK)                                { goto done; }
   if (mp_cmp_d(R->y, 0) == LTC_MP_LT) {
      if ((err = mp_add(R->y, modulus, R->y)) != CRYPT_OK)                        { goto done; }
   }

   err = CRYPT_OK;
done:
   mp_clear_multi(t2, t1, NULL);
   return err;
}
#endif
/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */

