#ifndef OC_SPAKE2PLUS_H
#define OC_SPAKE2PLUS_H

#include "mbedtls/bignum.h"

int validate_against_test_vector();
int calculate_w0_w1(size_t len_pw, const uint8_t pw[], const uint8_t salt[32], int it, mbedtls_mpi *w0, mbedtls_mpi *w1);

#endif // OC_SPAKE2PLUS_H