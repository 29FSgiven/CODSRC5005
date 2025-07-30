#include <stdint.h>

typedef unsigned short ulong16;

struct rc5_32_key {
   int rounds;
   ulong16 K[50];
};

uint32_t rc5_ecb_encrypt(uint32_t pt, const struct rc5_32_key *skey);
int rc5_ecb_decrypt(uint32_t ct, const struct rc5_32_key *skey);
int rc5_setup(const unsigned char *key, int keylen, int num_rounds, struct rc5_32_key *skey);