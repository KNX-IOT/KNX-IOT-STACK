/*
// Copyright (c) 2022 Cascoda Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#ifdef OC_SPAKE

#include "mbedtls/md.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/pkcs5.h"
#include <assert.h>

#include "oc_spake2plus.h"
#include "port/oc_random.h"
#include "port/oc_log.h"

static mbedtls_ctr_drbg_context *ctr_drbg_ctx;
static mbedtls_ecp_group grp;

// clang-format off
// mbedTLS cannot decode the compressed points in the specification, so we have to do it ourselves.
// generated using the Python `cryptography` module:
// M = ec.EllipticCurvePublicKey.from_encoded_point(curve(), (0x02886...).to_bytes(33, 'big'))
// N = ec.EllipticCurvePublicKey.from_encoded_point(curve(), (0x03d8b...).to_bytes(33, 'big'))
// M.public_bytes(Encoding.X962, PublicFormat.UncompressedPoint).hex()
// N.public_bytes(Encoding.X962, PublicFormat.UncompressedPoint).hex()
// clang-format on

uint8_t bytes_M[] = {
  0x04, 0x88, 0x6e, 0x2f, 0x97, 0xac, 0xe4, 0x6e, 0x55, 0xba, 0x9d, 0xd7, 0x24,
  0x25, 0x79, 0xf2, 0x99, 0x3b, 0x64, 0xe1, 0x6e, 0xf3, 0xdc, 0xab, 0x95, 0xaf,
  0xd4, 0x97, 0x33, 0x3d, 0x8f, 0xa1, 0x2f, 0x5f, 0xf3, 0x55, 0x16, 0x3e, 0x43,
  0xce, 0x22, 0x4e, 0x0b, 0x0e, 0x65, 0xff, 0x02, 0xac, 0x8e, 0x5c, 0x7b, 0xe0,
  0x94, 0x19, 0xc7, 0x85, 0xe0, 0xca, 0x54, 0x7d, 0x55, 0xa1, 0x2e, 0x2d, 0x20
};
uint8_t bytes_N[] = {
  0x04, 0xd8, 0xbb, 0xd6, 0xc6, 0x39, 0xc6, 0x29, 0x37, 0xb0, 0x4d, 0x99, 0x7f,
  0x38, 0xc3, 0x77, 0x07, 0x19, 0xc6, 0x29, 0xd7, 0x01, 0x4d, 0x49, 0xa2, 0x4b,
  0x4f, 0x98, 0xba, 0xa1, 0x29, 0x2b, 0x49, 0x07, 0xd6, 0x0a, 0xa6, 0xbf, 0xad,
  0xe4, 0x50, 0x08, 0xa6, 0x36, 0x33, 0x7f, 0x51, 0x68, 0xc6, 0x4d, 0x9b, 0xd3,
  0x60, 0x34, 0x80, 0x8c, 0xd5, 0x64, 0x49, 0x0b, 0x1e, 0x65, 0x6e, 0xdb, 0xe7
};

static char password[33] = { 0 };

#define KNX_RNG_LEN (32)
#define KNX_SALT_LEN (32)

struct spake_parameters
{
  int loaded; /// 0: not loaded, 1: loaded
  mbedtls_mpi w0;
  mbedtls_ecp_point L;
  uint8_t salt[32];
  uint8_t rand[32];
  uint32_t iter;
} g_spake_parameters;

int
oc_spake_init(void)
{
  int ret = 0;
  // initialize entropy and drbg contexts
  mbedtls_ecp_group_init(&grp);

  MBEDTLS_MPI_CHK(mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1));

  ctr_drbg_ctx = oc_random_get_ctr_drbg_context();
cleanup:
  return ret;
}

int
oc_spake_free(void)
{
  mbedtls_ecp_group_free(&grp);
  return 0;
}

const char *
oc_spake_get_password()
{
  return password;
}

void
oc_spake_set_password(char *new_pass)
{
  strncpy(password, new_pass, sizeof(password));
}

int
oc_spake_set_parameters(uint8_t rand[32], uint8_t salt[32], int it,
                        mbedtls_mpi w0, mbedtls_ecp_point L)
{
  int ret;
  g_spake_parameters.loaded = 0;
  memcpy(g_spake_parameters.rand, rand, 32);
  memcpy(g_spake_parameters.salt, salt, 32);
  g_spake_parameters.iter = it;
  MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&g_spake_parameters.w0, &w0));
  MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&g_spake_parameters.L.X, &L.X));
  MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&g_spake_parameters.L.Y, &L.Y));
  MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&g_spake_parameters.L.Z, &L.Z));
  g_spake_parameters.loaded = 1;
  return 0;
cleanup:
  mbedtls_mpi_free(&g_spake_parameters.w0);
  mbedtls_ecp_point_free(&g_spake_parameters.L);
  return ret;
}

int
oc_spake_get_parameters(uint8_t *rand, uint8_t *salt, int *it, mbedtls_mpi *w0,
                        mbedtls_ecp_point *L)
{
  if (g_spake_parameters.loaded != 1)
    return 1;
  int ret;
  if (rand) {
    memcpy(rand, g_spake_parameters.rand, 32);
  }
  if (salt) {
    memcpy(salt, g_spake_parameters.salt, 32);
  }
  if (it) {
    *it = g_spake_parameters.iter;
  }
  if (w0) {
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(w0, &g_spake_parameters.w0));
  }
  if (L) {
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&L->X, &g_spake_parameters.L.X));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&L->Y, &g_spake_parameters.L.Y));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&L->Z, &g_spake_parameters.L.Z));
  }
  return 0;
cleanup:
  mbedtls_mpi_free(w0);
  mbedtls_ecp_point_free(L);
  return ret;
}

int
oc_spake_get_pbkdf_params(uint8_t rnd[32], uint8_t salt[32], int *it)
{
  if (oc_spake_get_parameters(rnd, salt, it, NULL, NULL) == 0)
    return 0;

  // generate random numbers for rnd, salt & it (# of iterations)
  return oc_spake_parameter_exchange(rnd, salt, it);
}

int
oc_spake_get_w0_L(const char *pw, size_t len_salt, const uint8_t *salt, int it,
                  mbedtls_mpi *w0, mbedtls_ecp_point *L)
{
  int ret;
  if (oc_spake_get_parameters(NULL, NULL, NULL, w0, L) == 0)
    return 0;

  ret = oc_spake_calc_w0_L(password, len_salt, salt, it, w0, L);

  if (ret != 0) {
    OC_ERR("oc_spake_calc_w0_L failed with code %d", ret);
  }
  return ret;
}

// encode value as zero-padded little endian bytes
// returns number of bytes written (always 8)
// buffer must be able to fit 8 bytes
size_t
encode_uint(uint64_t value, uint8_t *buffer)
{
  buffer[0] = (value >> 0) & 0xff;
  buffer[1] = (value >> 8) & 0xff;
  buffer[2] = (value >> 16) & 0xff;
  buffer[3] = (value >> 24) & 0xff;
  buffer[4] = (value >> 32) & 0xff;
  buffer[5] = (value >> 40) & 0xff;
  buffer[6] = (value >> 48) & 0xff;
  buffer[7] = (value >> 56) & 0xff;
  return 8;
}

// encode string as length followed by bytes
// returns number of bytes written
size_t
encode_string(const char *str, uint8_t *buffer)
{
  size_t len = encode_uint(strlen(str), buffer);
  memcpy(buffer + len, str, strlen(str));
  return len + strlen(str);
}

// encode point as length followed by bytes
// returns number of bytes written
size_t
encode_point(mbedtls_ecp_group *grp, const mbedtls_ecp_point *point,
             uint8_t *buffer)
{
  size_t len_point = 0;
  size_t len_len = 0;
  uint8_t point_buf[kPubKeySize];
  int ret;
  ret =
    mbedtls_ecp_point_write_binary(grp, point, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                   &len_point, point_buf, sizeof(point_buf));
  assert(ret == 0);

  len_len = encode_uint(len_point, buffer);
  memcpy(buffer + len_len, point_buf, len_point);
  return len_len + len_point;
}

// encode mpi as length followed by bytes
// returns number of bytes written
size_t
encode_mpi(mbedtls_mpi *mpi, uint8_t *buffer)
{
  size_t len_mpi = 0;
  size_t len_len = 0;
  uint8_t mpi_buf[64];
  int ret;

  len_mpi = mbedtls_mpi_size(mpi);

  ret = mbedtls_mpi_write_binary(mpi, mpi_buf, len_mpi);
  assert(ret == 0);

  len_len = encode_uint(len_mpi, buffer);
  memcpy(buffer + len_len, mpi_buf, len_mpi);
  return len_len + len_mpi;
}

void
oc_spake_print_point(mbedtls_ecp_point *p)
{
  uint8_t buf[kPubKeySize];
  size_t len = 0;

  len = encode_point(&grp, p, buf);

  for (size_t i = 0; i < len; i++) {
    PRINT("%02x", buf[i]);
  }
  PRINT("\n");
}

void
oc_spake_print_mpi(mbedtls_mpi *m)
{
  uint8_t buf[64];
  size_t len = 0;

  len = encode_mpi(m, buf);

  for (size_t i = 0; i < len; i++) {
    PRINT("%02x", buf[i]);
  }
  PRINT("\n");
}

int
oc_spake_encode_pubkey(mbedtls_ecp_point *P, uint8_t out[kPubKeySize])
{
  size_t olen;
  return mbedtls_ecp_point_write_binary(&grp, P, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                        &olen, out, kPubKeySize);
}

int
oc_spake_parameter_exchange(uint8_t rnd[32], uint8_t salt[32], int *it)
{
  unsigned int it_seed;
  int ret;

  MBEDTLS_MPI_CHK(mbedtls_ctr_drbg_random(ctr_drbg_ctx, rnd, KNX_RNG_LEN));
  MBEDTLS_MPI_CHK(mbedtls_ctr_drbg_random(ctr_drbg_ctx, salt, KNX_SALT_LEN));
  MBEDTLS_MPI_CHK(mbedtls_ctr_drbg_random(
    ctr_drbg_ctx, (unsigned char *)&it_seed, sizeof(it_seed)));

  *it = it_seed % (KNX_MAX_IT - KNX_MIN_IT) + KNX_MIN_IT;
cleanup:
  return ret;
}

int
oc_spake_calc_w0_w1(const char *pw, size_t len_salt, const uint8_t *salt,
                    int it, mbedtls_mpi *w0, mbedtls_mpi *w1)
{
  int ret;
  mbedtls_md_context_t ctx;
  uint8_t *input = malloc(3 * sizeof(uint64_t) + strlen(pw));
  size_t len_input = 0;

  // Hmm, SPAKE2+ mandates this be 40 bytes or longer,
  // but KNX-IoT says it's 32 bytes maybe?
//  const size_t output_len = 40;
#define output_len 80
  uint8_t output[output_len];

  mbedtls_mpi w0s, w1s;

  mbedtls_md_init(&ctx);
  mbedtls_mpi_init(&w0s);
  mbedtls_mpi_init(&w1s);

  len_input += encode_string(pw, input + len_input); // password
  len_input += encode_string("", input + len_input); // null idProver
  len_input += encode_string("", input + len_input); // null idVerifier

  MBEDTLS_MPI_CHK(
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1));
  MBEDTLS_MPI_CHK(mbedtls_pkcs5_pbkdf2_hmac(&ctx, input, len_input, salt,
                                            len_salt, it, output_len, output));

  // extract w0s and w1s from the output
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&w0s, output, output_len / 2));
  MBEDTLS_MPI_CHK(
    mbedtls_mpi_read_binary(&w1s, output + output_len / 2, output_len / 2));

  // calculate w0 and w1
  // the cofactor of P-256 is 1, so the order of the group is equal to the large
  // prime p
  MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(w0, &w0s, &grp.N));
  MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(w1, &w1s, &grp.N));

cleanup:
  mbedtls_md_free(&ctx);
  mbedtls_mpi_free(&w0s);
  mbedtls_mpi_free(&w1s);
  free(input);
  return ret;
}

int
oc_spake_calc_w0_L(const char *pw, size_t len_salt, const uint8_t *salt, int it,
                   mbedtls_mpi *w0, mbedtls_ecp_point *L)
{
  int ret;
  mbedtls_mpi w1;
  mbedtls_mpi_init(&w1);
  MBEDTLS_MPI_CHK(oc_spake_calc_w0_w1(pw, len_salt, salt, it, w0, &w1));
  MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&grp, L, &w1, &grp.G, mbedtls_ctr_drbg_random,
                                  ctr_drbg_ctx));
cleanup:
  mbedtls_mpi_free(&w1);
  return ret;
}

int
oc_spake_gen_keypair(mbedtls_mpi *y, mbedtls_ecp_point *pub_y)
{
  return mbedtls_ecp_gen_keypair(&grp, y, pub_y, mbedtls_ctr_drbg_random,
                                 ctr_drbg_ctx);
}

int
oc_gen_masterkey(uint8_t *array)
{
  return mbedtls_ctr_drbg_random(ctr_drbg_ctx, array, OSCORE_KEY_LEN);
}

// generic formula for
// pX = pubX + wX * L
static int
calculate_pX(mbedtls_ecp_point *pX, const mbedtls_ecp_point *pubX,
             const mbedtls_mpi *wX, const uint8_t bytes_L[], size_t len_L)
{
  mbedtls_mpi one;
  mbedtls_ecp_point L;
  int ret;

  mbedtls_mpi_init(&one);
  mbedtls_ecp_point_init(&L);

  // MBEDTLS_MPI_CHK sets ret to the return value of f and goes to cleanup if
  // ret is nonzero
  MBEDTLS_MPI_CHK(mbedtls_ecp_point_read_binary(&grp, &L, bytes_L, len_L));
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&one, 10, "1"));

  // pA = 1 * pubA + w0 * M
  MBEDTLS_MPI_CHK(mbedtls_ecp_muladd(&grp, pX, &one, pubX, wX, &L));

cleanup:
  mbedtls_mpi_free(&one);
  mbedtls_ecp_point_free(&L);
  return ret;
}

// pA = pubA + w0 * M
int
oc_spake_calc_pA(mbedtls_ecp_point *pA, const mbedtls_ecp_point *pubA,
                 const mbedtls_mpi *w0)
{
  return calculate_pX(pA, pubA, w0, bytes_M, sizeof(bytes_M));
}

// pB = pubB + w0 * N
int
oc_spake_calc_pB(mbedtls_ecp_point *pB, const mbedtls_ecp_point *pubB,
                 const mbedtls_mpi *w0)
{
  return calculate_pX(pB, pubB, w0, bytes_N, sizeof(bytes_N));
}

// generic formula for
// J = f * (K - g * L)
static int
calculate_JfKgL(mbedtls_ecp_point *J, const mbedtls_mpi *f,
                const mbedtls_ecp_point *K, const mbedtls_mpi *g,
                const mbedtls_ecp_point *L)
{
  int ret;
  mbedtls_mpi negative_g, zero, one;
  mbedtls_mpi_init(&negative_g);
  mbedtls_mpi_init(&zero);
  mbedtls_mpi_init(&one);

  mbedtls_ecp_point K_minus_g_L;
  mbedtls_ecp_point_init(&K_minus_g_L);

  // negative_g = -g
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&zero, 10, "0"));
  MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&negative_g, &zero, g));
  MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&negative_g, &negative_g, &grp.N));

  // K_minus_g_L = K - g * L
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&one, 10, "1"));
  MBEDTLS_MPI_CHK(
    mbedtls_ecp_muladd(&grp, &K_minus_g_L, &one, K, &negative_g, L));

  // J = f * (K_minus_g_L)
  MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&grp, J, f, &K_minus_g_L,
                                  mbedtls_ctr_drbg_random, ctr_drbg_ctx));

cleanup:
  mbedtls_mpi_free(&negative_g);
  mbedtls_mpi_free(&zero);
  mbedtls_mpi_free(&one);
  mbedtls_ecp_point_free(&K_minus_g_L);
  return ret;
}

// Z = h*x*(Y - w0*N)
// also works for:
// V = h*w1*(Y - w0*N)
static int
calculate_ZV_N(mbedtls_ecp_point *Z, const mbedtls_mpi *x,
               const mbedtls_ecp_point *Y, const mbedtls_mpi *w0)
{
  int ret;

  mbedtls_ecp_point N;
  mbedtls_ecp_point_init(&N);

  MBEDTLS_MPI_CHK(
    mbedtls_ecp_point_read_binary(&grp, &N, bytes_N, sizeof(bytes_N)));

  // For the secp256r1 curve, h is 1, so we don't need to do anything
  MBEDTLS_MPI_CHK(calculate_JfKgL(Z, x, Y, w0, &N));

cleanup:
  mbedtls_ecp_point_free(&N);
  return ret;
}
// Z = h*y*(X - w0*M)
static int
calculate_Z_M(mbedtls_ecp_point *Z, const mbedtls_mpi *x,
              const mbedtls_ecp_point *Y, const mbedtls_mpi *w0)
{
  int ret;

  mbedtls_ecp_point M;
  mbedtls_ecp_point_init(&M);

  MBEDTLS_MPI_CHK(
    mbedtls_ecp_point_read_binary(&grp, &M, bytes_M, sizeof(bytes_M)));

  // For the secp256r1 curve, h is 1, so we don't need to do anything
  MBEDTLS_MPI_CHK(calculate_JfKgL(Z, x, Y, w0, &M));

cleanup:
  mbedtls_ecp_point_free(&M);
  return ret;
}

int
calc_transcript_responder(spake_data_t *spake_data,
                          const uint8_t X_enc[kPubKeySize],
                          mbedtls_ecp_point *Y, bool use_testing_context)
{
  int ret = 0;
  mbedtls_ecp_point Z, V, X;
  uint8_t ttbuf[2048];
  size_t ttlen = 0;

  mbedtls_ecp_point_init(&Z);
  mbedtls_ecp_point_init(&V);
  mbedtls_ecp_point_init(&X);

  mbedtls_ecp_point_read_binary(&grp, &X, X_enc, kPubKeySize);
  // abort if X is the point at infinity
  MBEDTLS_MPI_CHK(mbedtls_ecp_is_zero(&X));

  // Z = h*y*(X - w0*M)
  MBEDTLS_MPI_CHK(calculate_Z_M(&Z, &spake_data->y, &X, &spake_data->w0));

  // V = h*y*L, where L = w1*P
  MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&grp, &V, &spake_data->y, &spake_data->L,
                                  mbedtls_ctr_drbg_random, ctr_drbg_ctx));

  // calculate transcript
  // Context
  if (use_testing_context) {
    ttlen += encode_string("SPAKE2+-P256-SHA256-HKDF draft-01", ttbuf + ttlen);
  } else {
    ttlen += encode_string(SPAKE_CONTEXT, ttbuf + ttlen);
  }
  // null idProver
  ttlen += encode_string("", ttbuf + ttlen);
  // null idVerifier
  ttlen += encode_string("", ttbuf + ttlen);
  // M
  mbedtls_ecp_point M;
  mbedtls_ecp_point_init(&M);
  MBEDTLS_MPI_CHK(
    mbedtls_ecp_point_read_binary(&grp, &M, bytes_M, sizeof(bytes_M)));
  ttlen += encode_point(&grp, &M, ttbuf + ttlen);
  // N
  mbedtls_ecp_point N;
  mbedtls_ecp_point_init(&N);
  MBEDTLS_MPI_CHK(
    mbedtls_ecp_point_read_binary(&grp, &N, bytes_N, sizeof(bytes_N)));
  ttlen += encode_point(&grp, &N, ttbuf + ttlen);
  // X
  ttlen += encode_point(&grp, &X, ttbuf + ttlen);
  // Y
  ttlen += encode_point(&grp, Y, ttbuf + ttlen);
  // Z
  ttlen += encode_point(&grp, &Z, ttbuf + ttlen);
  // V
  ttlen += encode_point(&grp, &V, ttbuf + ttlen);
  // w0
  ttlen += encode_mpi(&spake_data->w0, ttbuf + ttlen);

  // calculate hash
  mbedtls_sha256(ttbuf, ttlen, spake_data->Ka_Ke, 0);

cleanup:
  mbedtls_ecp_point_free(&Z);
  mbedtls_ecp_point_free(&V);
  mbedtls_ecp_point_free(&X);
  return ret;
}

int
oc_spake_calc_transcript_responder(spake_data_t *spake_data,
                                   const uint8_t X_enc[kPubKeySize],
                                   mbedtls_ecp_point *Y)
{

  return calc_transcript_responder(spake_data, X_enc, Y, false);
}

int
calc_transcript_initiator(mbedtls_mpi *w0, mbedtls_mpi *w1, mbedtls_mpi *x,
                          mbedtls_ecp_point *X,
                          const uint8_t Y_enc[kPubKeySize], uint8_t Ka_Ke[32],
                          bool use_testing_context)

{
  int ret;
  mbedtls_ecp_point Y, Z, V;
  uint8_t ttbuf[2048];
  size_t ttlen = 0;
  mbedtls_ecp_point_init(&Y);
  mbedtls_ecp_point_init(&Z);
  mbedtls_ecp_point_init(&V);

  mbedtls_ecp_point_read_binary(&grp, &Y, Y_enc, kPubKeySize);

  // Z = h*x*(Y - w0*N)
  MBEDTLS_MPI_CHK(calculate_ZV_N(&Z, x, &Y, w0));

  // V = h*w1*(Y - w0*N)
  MBEDTLS_MPI_CHK(calculate_ZV_N(&V, w1, &Y, w0));

  // calculate transcript
  // Context
  if (use_testing_context) {
    ttlen += encode_string("SPAKE2+-P256-SHA256-HKDF draft-01", ttbuf + ttlen);
  } else {
    ttlen += encode_string(SPAKE_CONTEXT, ttbuf + ttlen);
  }
  // null idProver
  ttlen += encode_string("", ttbuf + ttlen);
  // null idVerifier
  ttlen += encode_string("", ttbuf + ttlen);
  // M
  mbedtls_ecp_point M;
  mbedtls_ecp_point_init(&M);
  MBEDTLS_MPI_CHK(
    mbedtls_ecp_point_read_binary(&grp, &M, bytes_M, sizeof(bytes_M)));
  ttlen += encode_point(&grp, &M, ttbuf + ttlen);
  // N
  mbedtls_ecp_point N;
  mbedtls_ecp_point_init(&N);
  MBEDTLS_MPI_CHK(
    mbedtls_ecp_point_read_binary(&grp, &N, bytes_N, sizeof(bytes_N)));
  ttlen += encode_point(&grp, &N, ttbuf + ttlen);
  // X
  ttlen += encode_point(&grp, X, ttbuf + ttlen);
  // Y
  ttlen += encode_point(&grp, &Y, ttbuf + ttlen);
  // Z
  ttlen += encode_point(&grp, &Z, ttbuf + ttlen);
  // V
  ttlen += encode_point(&grp, &V, ttbuf + ttlen);
  // w0
  ttlen += encode_mpi(w0, ttbuf + ttlen);

  // calculate hash
  mbedtls_sha256(ttbuf, ttlen, Ka_Ke, 0);

cleanup:
  mbedtls_ecp_point_free(&Y);
  mbedtls_ecp_point_free(&Z);
  mbedtls_ecp_point_free(&V);
  return ret;
}

int
oc_spake_calc_transcript_initiator(mbedtls_mpi *w0, mbedtls_mpi *w1,
                                   mbedtls_mpi *x, mbedtls_ecp_point *X,
                                   const uint8_t Y_enc[kPubKeySize],
                                   uint8_t Ka_Ke[32])
{

  return calc_transcript_initiator(w0, w1, x, X, Y_enc, Ka_Ke, false);
}

int
oc_spake_calc_cB(uint8_t *Ka_Ke, uint8_t cB[32], uint8_t bytes_X[kPubKeySize])
{
  // |KcA| + |KcB| = 16 bytes
  uint8_t KcA_KcB[32];
  mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), NULL, 0, Ka_Ke, 16,
               (const unsigned char *)"ConfirmationKeys",
               strlen("ConfirmationKeys"), KcA_KcB, 32);

  // Calculate cB
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                  KcA_KcB + sizeof(KcA_KcB) / 2, sizeof(KcA_KcB) / 2, bytes_X,
                  kPubKeySize, cB);
  return 0;
}

int
oc_spake_calc_cA(uint8_t *Ka_Ke, uint8_t cA[32], uint8_t bytes_Y[kPubKeySize])
{
  // |KcA| + |KcB| = 16 bytes
  uint8_t KcA_KcB[32];
  mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), NULL, 0, Ka_Ke, 16,
               (const unsigned char *)"ConfirmationKeys",
               strlen("ConfirmationKeys"), KcA_KcB, 32);

  // Calculate cA
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), KcA_KcB,
                  sizeof(KcA_KcB) / 2, bytes_Y, kPubKeySize, cA);
  return 0;
}

#endif // OC_SPAKE
