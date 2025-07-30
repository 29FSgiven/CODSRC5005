FLAGS="-m32 -Wall -Os -g -c -I. -DLTC_NO_ROLC -DLTC_SOURCE -DLTC_NO_TEST -DLTC_NO_ECC_TIMING_RESISTANT"
MATHFLAGS="-m32 -Wall -Os -g -c -I. -Imath -DLTC_NO_ROLC -DLTC_SOURCE -DLTM_DESC -DMP_FIXED_CUTOFFS"
gcc $FLAGS ciphers/aes/aes.c
gcc $FLAGS hashes/*.c
gcc $FLAGS misc/crypt/*.c
gcc $FLAGS misc/base64/*.c
gcc $FLAGS misc/pkcs5/*.c
gcc $FLAGS misc/*.c
gcc $FLAGS prngs/*.c
gcc $FLAGS mac/hmac/*.c
gcc $FLAGS pk/asn1/der/bit/*.c
gcc $FLAGS pk/asn1/der/boolean/*.c
gcc $FLAGS pk/asn1/der/choice/*.c
gcc $FLAGS pk/asn1/der/ia5/*.c
gcc $FLAGS pk/asn1/der/integer/*.c
gcc $FLAGS pk/asn1/der/object_identifier/*.c
gcc $FLAGS pk/asn1/der/octet/*.c
gcc $FLAGS pk/asn1/der/printable_string/*.c
gcc $FLAGS pk/asn1/der/sequence/*.c
gcc $FLAGS pk/asn1/der/short_integer/*.c
gcc $FLAGS pk/asn1/der/utctime/*.c
gcc $FLAGS pk/asn1/der/utf8/*.c
gcc $FLAGS pk/asn1/der/set/*.c
gcc $FLAGS pk/asn1/der/custom_type/*.c
gcc $FLAGS pk/asn1/der/general/*.c
gcc $FLAGS pk/asn1/der/generalizedtime/*.c
gcc $FLAGS pk/asn1/der/teletex_string/*.c
gcc $FLAGS pk/dh/*.c
gcc $FLAGS pk/rsa/*.c
gcc $FLAGS pk/pkcs1/*.c
gcc $FLAGS pk/ecc/*.c
gcc $FLAGS stream/chacha/*.c
gcc $MATHFLAGS math/*.c

ar cr ../../lib/libtomcrypt_lnx.a *.o

rm *.o

