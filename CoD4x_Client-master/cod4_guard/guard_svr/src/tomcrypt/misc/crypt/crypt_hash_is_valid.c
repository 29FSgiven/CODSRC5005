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
  @file crypt_hash_is_valid.c
  Determine if hash is valid, Tom St Denis
*/

/*
   Test if a hash index is valid
   @param idx   The index of the hash to search for
   @return CRYPT_OK if valid
*/
int hash_is_valid(int idx)
{
   LTC_MUTEX_LOCK(&ltc_hash_mutex);
   if (idx < 0 || idx >= TAB_SIZE || hash_descriptor[idx].name == NULL) {
      LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
      return CRYPT_INVALID_HASH;
   }
   LTC_MUTEX_UNLOCK(&ltc_hash_mutex);
   return CRYPT_OK;
}

/* ref:         HEAD -> develop */
/* git commit:  4ed50d8da1b8dabe02c5ffa2abca3c57811bdf14 */
/* commit time: 2019-06-05 09:24:19 +0200 */
