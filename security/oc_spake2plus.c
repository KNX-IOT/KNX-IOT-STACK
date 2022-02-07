#include "mbedtls/md.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/pkcs5.h"
#include <assert.h>

#include "oc_spake2plus.h"

static mbedtls_entropy_context entropy_ctx;
static mbedtls_ctr_drbg_context ctr_drbg_ctx;
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

static char password[33];

#define KNX_RNG_LEN (32)
#define KNX_SALT_LEN (32)
#define KNX_MIN_IT (1000)
#define KNX_MAX_IT (100000)

int
oc_spake_init(void)
{
  int ret = 0;
  // initialize entropy and drbg contexts
  mbedtls_entropy_init(&entropy_ctx);
  mbedtls_ctr_drbg_init(&ctr_drbg_ctx);
  mbedtls_ecp_group_init(&grp);

  MBEDTLS_MPI_CHK(mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1));
  MBEDTLS_MPI_CHK(mbedtls_ctr_drbg_seed(&ctr_drbg_ctx, mbedtls_entropy_func,
                                        &entropy_ctx, NULL, 0));
cleanup:
  return ret;
}

int
oc_spake_free(void)
{
  mbedtls_ctr_drbg_free(&ctr_drbg_ctx);
  mbedtls_entropy_free(&entropy_ctx);
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
oc_spake_encode_pubkey(mbedtls_ecp_point *P, uint8_t out[kPubKeySize])
{
  size_t olen;
  return mbedtls_ecp_point_write_binary(&grp, P, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                        &olen, out, kPubKeySize);
}

int
oc_spake_parameter_exchange(oc_string_t *rnd, oc_string_t *salt, int *it)
{
  unsigned int it_seed;
  int ret;
  oc_free_string(rnd);
  oc_free_string(salt);

  oc_alloc_string(rnd, KNX_RNG_LEN);
  oc_alloc_string(salt, KNX_SALT_LEN);

  MBEDTLS_MPI_CHK(
    mbedtls_ctr_drbg_random(&ctr_drbg_ctx, rnd->ptr, KNX_RNG_LEN));
  MBEDTLS_MPI_CHK(
    mbedtls_ctr_drbg_random(&ctr_drbg_ctx, salt->ptr, KNX_SALT_LEN));
  MBEDTLS_MPI_CHK(mbedtls_ctr_drbg_random(
    &ctr_drbg_ctx, (unsigned char *)&it_seed, sizeof(it_seed)));

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

  // Hmm, SPAKE2+ mandates this be 40 bytes or longer,
  // but KNX-IoT says it's 32 bytes maybe?
  const size_t output_len = 40;
  uint8_t output[output_len];

  mbedtls_mpi w0s, w1s;

  mbedtls_md_init(&ctx);
  mbedtls_mpi_init(&w0s);
  mbedtls_mpi_init(&w1s);

  MBEDTLS_MPI_CHK(
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1));
  MBEDTLS_MPI_CHK(mbedtls_pkcs5_pbkdf2_hmac(&ctx, pw, strlen(pw), salt,
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
                                  &ctr_drbg_ctx));
cleanup:
  mbedtls_mpi_free(&w1);
  return ret;
}

int
oc_spake_gen_y(mbedtls_mpi *y, mbedtls_ecp_point *pub_y)
{
  return mbedtls_ecp_gen_keypair(&grp, y, pub_y, mbedtls_ctr_drbg_random,
                                 &ctr_drbg_ctx);
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
                                  mbedtls_ctr_drbg_random, &ctr_drbg_ctx));

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

int
oc_spake_calc_transcript_responder(spake_data_t *spake_data,
                                   uint8_t X_enc[kPubKeySize],
                                   mbedtls_ecp_point *Y)
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
                                  mbedtls_ctr_drbg_random, &ctr_drbg_ctx));

  // calculate transcript
  // Context
  ttlen += encode_string(SPAKE_CONTEXT, ttbuf + ttlen);
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

// separate function for transcript of party A, using bytes of pB and pA ECP
// Point
int
oc_spake_calc_transcript_initiator(mbedtls_ecp_point *X,
                                   uint8_t Y_enc[kPubKeySize])

{
  // TODO adapt this code from the test vector to work with the
  // function definition
  mbedtls_ecp_point Z;
  mbedtls_ecp_point_init(&Z);

  // Z = h*x*(Y - w0*N)
  MBEDTLS_MPI_CHK(calculate_ZV_N(&Z, &x, &Y, &w0));

  MBEDTLS_MPI_CHK(mbedtls_ecp_point_write_binary(
    &grp, &Z, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));
  assert(memcmp(bytes_Z, cmpbuf, cmplen) == 0);

  mbedtls_ecp_point V;
  mbedtls_ecp_point_init(&V);

  mbedtls_mpi w1;
  mbedtls_mpi_init(&w1);
  mbedtls_mpi_read_binary(&w1, bytes_w1, sizeof(bytes_w1));

  // V = h*w1*(Y - w0*N)
  MBEDTLS_MPI_CHK(calculate_ZV_N(&V, &w1, &Y, &w0));
  MBEDTLS_MPI_CHK(mbedtls_ecp_point_write_binary(
    &grp, &V, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));
  assert(memcmp(bytes_V, cmpbuf, cmplen) == 0);

}

int
oc_spake_calc_cB(spake_data_t *spake_data, uint8_t cB[32],
                 uint8_t bytes_X[kPubKeySize])
{
  // |KcA| + |KcB| = 16 bytes
  uint8_t KcA_KcB[32];
  mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), NULL, 0,
               spake_data->Ka_Ke, sizeof(spake_data->Ka_Ke) / 2,
               "ConfirmationKeys", strlen("ConfirmationKeys"), KcA_KcB, 32);

  // Calculate cB
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                  KcA_KcB + sizeof(KcA_KcB) / 2, sizeof(KcA_KcB) / 2, bytes_X,
                  kPubKeySize, cB);
}

int
oc_spake_calc_cA(spake_data_t *spake_data, uint8_t cA[32],
                 uint8_t bytes_Y[kPubKeySize])
{
  // |KcA| + |KcB| = 16 bytes
  uint8_t KcA_KcB[32];
  mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), NULL, 0,
               spake_data->Ka_Ke, sizeof(spake_data->Ka_Ke) / 2,
               "ConfirmationKeys", strlen("ConfirmationKeys"), KcA_KcB, 32);

  // Calculate cA and cB
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), KcA_KcB,
                  sizeof(KcA_KcB) / 2, bytes_Y, kPubKeySize, cA);
}

int
oc_spake_test_vector()
{
  // Test Vector values from Spake2+ draft.
  // Using third set, as we only have easy access to the server (e.g. device)
  // identity.
  char Context[] = "SPAKE2+-P256-SHA256-HKDF draft-01";
  char A[] = "";
  char B[] = "server";

  uint8_t bytes_w0[] = { 0xe6, 0x88, 0x7c, 0xf9, 0xbd, 0xfb, 0x75, 0x79,
                         0xc6, 0x9b, 0xf4, 0x79, 0x28, 0xa8, 0x45, 0x14,
                         0xb5, 0xe3, 0x55, 0xac, 0x03, 0x48, 0x63, 0xf7,
                         0xff, 0xaf, 0x43, 0x90, 0xe6, 0x7d, 0x79, 0x8c };
  uint8_t bytes_w1[] = { 0x24, 0xb5, 0xae, 0x4a, 0xbd, 0xa8, 0x68, 0xec,
                         0x93, 0x36, 0xff, 0xc3, 0xb7, 0x8e, 0xe3, 0x1c,
                         0x57, 0x55, 0xbe, 0xf1, 0x75, 0x92, 0x27, 0xef,
                         0x53, 0x72, 0xca, 0x13, 0x9b, 0x94, 0xe5, 0x12 };

  uint8_t bytes_L[] = { 0x04, 0x95, 0x64, 0x5c, 0xfb, 0x74, 0xdf, 0x6e, 0x58,
                        0xf9, 0x74, 0x8b, 0xb8, 0x3a, 0x86, 0x62, 0x0b, 0xab,
                        0x7c, 0x82, 0xe1, 0x07, 0xf5, 0x7d, 0x68, 0x70, 0xda,
                        0x8c, 0xbc, 0xb2, 0xff, 0x9f, 0x70, 0x63, 0xa1, 0x4b,
                        0x64, 0x02, 0xc6, 0x2f, 0x99, 0xaf, 0xcb, 0x97, 0x06,
                        0xa4, 0xd1, 0xa1, 0x43, 0x27, 0x32, 0x59, 0xfe, 0x76,
                        0xf1, 0xc6, 0x05, 0xa3, 0x63, 0x97, 0x45, 0xa9, 0x21,
                        0x54, 0xb9 };

  uint8_t bytes_x[] = { 0xba, 0x0f, 0x0f, 0x5b, 0x78, 0xef, 0x23, 0xfd,
                        0x07, 0x86, 0x8e, 0x46, 0xae, 0xca, 0x63, 0xb5,
                        0x1f, 0xda, 0x51, 0x9a, 0x34, 0x20, 0x50, 0x1a,
                        0xcb, 0xe2, 0x3d, 0x53, 0xc2, 0x91, 0x87, 0x48 };
  uint8_t bytes_X[] = { 0x04, 0xc1, 0x4d, 0x28, 0xf4, 0x37, 0x0f, 0xea, 0x20,
                        0x74, 0x51, 0x06, 0xce, 0xa5, 0x8b, 0xcf, 0xb6, 0x0f,
                        0x29, 0x49, 0xfa, 0x4e, 0x13, 0x1b, 0x9a, 0xff, 0x5e,
                        0xa1, 0x3f, 0xd5, 0xaa, 0x79, 0xd5, 0x07, 0xae, 0x1d,
                        0x22, 0x9e, 0x44, 0x7e, 0x00, 0x0f, 0x15, 0xeb, 0x78,
                        0xa9, 0xa3, 0x2c, 0x2b, 0x88, 0x65, 0x2e, 0x34, 0x11,
                        0x64, 0x20, 0x43, 0xc1, 0xb2, 0xb7, 0x99, 0x2c, 0xf2,
                        0xd4, 0xde };

  uint8_t bytes_y[] = { 0x39, 0x39, 0x7f, 0xbe, 0x6d, 0xb4, 0x7e, 0x9f,
                        0xbd, 0x1a, 0x26, 0x3d, 0x79, 0xf5, 0xd0, 0xaa,
                        0xa4, 0x4d, 0xf2, 0x6c, 0xe7, 0x55, 0xf7, 0x8e,
                        0x09, 0x26, 0x44, 0xb4, 0x34, 0x53, 0x3a, 0x42 };
  uint8_t bytes_Y[] = { 0x04, 0xd1, 0xbe, 0xe3, 0x12, 0x0f, 0xd8, 0x7e, 0x86,
                        0xfe, 0x18, 0x9c, 0xb9, 0x52, 0xdc, 0x68, 0x88, 0x23,
                        0x08, 0x0e, 0x62, 0x52, 0x4d, 0xd2, 0xc0, 0x8d, 0xff,
                        0xe3, 0xd2, 0x2a, 0x0a, 0x89, 0x86, 0xaa, 0x64, 0xc9,
                        0xfe, 0x01, 0x91, 0x03, 0x3c, 0xaf, 0xbc, 0x9b, 0xca,
                        0xef, 0xc8, 0xe2, 0xba, 0x8b, 0xa8, 0x60, 0xcd, 0x12,
                        0x7a, 0xf9, 0xef, 0xdd, 0x7f, 0x1c, 0x3a, 0x41, 0x92,
                        0x0f, 0xe8 };

  uint8_t bytes_Z[] = { 0x04, 0xaa, 0xc7, 0x1c, 0xf4, 0xc8, 0xdf, 0x81, 0x81,
                        0xb8, 0x67, 0xc9, 0xec, 0xbe, 0xe9, 0xd0, 0x96, 0x3c,
                        0xaf, 0x51, 0xf1, 0x53, 0x4a, 0x82, 0x34, 0x29, 0xc2,
                        0x6f, 0xe5, 0x24, 0x83, 0x13, 0xff, 0xc5, 0xc5, 0xe4,
                        0x4e, 0xa8, 0x16, 0x21, 0x61, 0xab, 0x6b, 0x3d, 0x73,
                        0xb8, 0x77, 0x04, 0xa4, 0x58, 0x89, 0xbf, 0x63, 0x43,
                        0xd9, 0x6f, 0xa9, 0x6c, 0xd1, 0x64, 0x1e, 0xfa, 0x71,
                        0x60, 0x7c };

  uint8_t bytes_V[] = { 0x04, 0xc7, 0xc9, 0x50, 0x53, 0x65, 0xf7, 0xce, 0x57,
                        0x29, 0x3c, 0x92, 0xa3, 0x7f, 0x1b, 0xbd, 0xc6, 0x8e,
                        0x03, 0x22, 0x90, 0x1e, 0x61, 0xed, 0xef, 0x59, 0xfe,
                        0xe7, 0x87, 0x6b, 0x17, 0xb0, 0x63, 0xe0, 0xfa, 0x4a,
                        0x12, 0x6e, 0xae, 0x0a, 0x67, 0x1b, 0x37, 0xf1, 0x46,
                        0x4c, 0xf1, 0xcc, 0xad, 0x59, 0x1c, 0x33, 0xae, 0x94,
                        0x4e, 0x3b, 0x1f, 0x31, 0x8d, 0x76, 0xe3, 0x6f, 0xea,
                        0x99, 0x66 };

  uint8_t Ka[] = { 0xec, 0x8d, 0x19, 0xb8, 0x07, 0xff, 0xb1, 0xd1,
                   0xee, 0xa8, 0x1a, 0x93, 0xba, 0x35, 0xcd, 0xfe };
  uint8_t Ke[] = { 0x2e, 0xa4, 0x0e, 0x4b, 0xad, 0xfa, 0x54, 0x52,
                   0xb5, 0x74, 0x4d, 0xc5, 0x98, 0x3e, 0x99, 0xba };

  uint8_t KcA[] = { 0x66, 0xde, 0x53, 0x4d, 0x9b, 0xf1, 0xe4, 0x4e,
                    0x96, 0xa5, 0x3a, 0x4b, 0x48, 0xd6, 0xb3, 0x53 };
  uint8_t KcB[] = { 0x49, 0x45, 0xc3, 0x8b, 0xb4, 0x76, 0xcb, 0x0f,
                    0x34, 0x7f, 0x32, 0x22, 0xbe, 0x9b, 0x64, 0xa2 };

  uint8_t cA[] = { 0xe5, 0x64, 0xc9, 0x3b, 0x30, 0x15, 0xef, 0xb9,
                   0x46, 0xdc, 0x16, 0xd6, 0x42, 0xbb, 0xe7, 0xd1,
                   0xc8, 0xda, 0x5b, 0xe1, 0x64, 0xed, 0x9f, 0xc3,
                   0xba, 0xe4, 0xe0, 0xff, 0x86, 0xe1, 0xbd, 0x3c };
  uint8_t cB[] = { 0x07, 0x2a, 0x94, 0xd9, 0xa5, 0x4e, 0xdc, 0x20,
                   0x1d, 0x88, 0x91, 0x53, 0x4c, 0x23, 0x17, 0xca,
                   0xdf, 0x3e, 0xa3, 0x79, 0x28, 0x27, 0xf4, 0x79,
                   0xe8, 0x73, 0xf9, 0x3e, 0x90, 0xf2, 0x15, 0x52 };

  uint8_t cmpbuf[128];
  size_t cmplen;
  int ret;

  // initialize rng
  oc_spake_init();

  // =========================
  // Check that X = x*P + w0*M
  // =========================
  mbedtls_mpi x, w0;
  mbedtls_mpi_init(&x);
  mbedtls_mpi_init(&w0);

  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&x, bytes_x, sizeof(bytes_x)));
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&w0, bytes_w0, sizeof(bytes_w0)));

  mbedtls_ecp_point X, pubA;
  mbedtls_ecp_point_init(&X);
  mbedtls_ecp_point_init(&pubA);
  // pubA = x*P (P is the generator group element, mbedtls uses G)
  MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&grp, &pubA, &x, &grp.G,
                                  mbedtls_ctr_drbg_random, &ctr_drbg_ctx));

  // X = pubA + w0*M
  MBEDTLS_MPI_CHK(oc_spake_calc_pA(&X, &pubA, &w0));
  MBEDTLS_MPI_CHK(mbedtls_ecp_point_write_binary(
    &grp, &X, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));

  // check the value of X is correct
  assert(memcmp(bytes_X, cmpbuf, cmplen) == 0);

  // =========================
  // Check that Y = y*P + w0*N
  // =========================
  mbedtls_mpi y;
  mbedtls_mpi_init(&y);

  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&y, bytes_y, sizeof(bytes_y)));

  mbedtls_ecp_point Y, pubB;
  mbedtls_ecp_point_init(&Y);
  mbedtls_ecp_point_init(&pubB);
  // pubB = y*P
  MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&grp, &pubB, &y, &grp.G,
                                  mbedtls_ctr_drbg_random, &ctr_drbg_ctx));

  // Y = pubB + w0*N
  MBEDTLS_MPI_CHK(oc_spake_calc_pB(&Y, &pubB, &w0));
  MBEDTLS_MPI_CHK(mbedtls_ecp_point_write_binary(
    &grp, &Y, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));
  // check the value of Y is correct
  assert(memcmp(bytes_Y, cmpbuf, cmplen) == 0);

  // ==============================
  // Check that altering the inputs
  // does indeed change the result
  // ==============================
  mbedtls_mpi bad_y;
  mbedtls_ecp_point bad_pubB, bad_Y;
  mbedtls_mpi_init(&bad_y);
  mbedtls_ecp_point_init(&bad_pubB);
  mbedtls_ecp_point_init(&bad_Y);

  bytes_y[5]++;
  mbedtls_mpi_read_binary(&bad_y, bytes_y, sizeof(bytes_y));
  bytes_y[5]--;

  // pubB = y*P
  MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&grp, &bad_pubB, &bad_y, &grp.G,
                                  mbedtls_ctr_drbg_random, &ctr_drbg_ctx));

  // Y = pubB + w0*N
  MBEDTLS_MPI_CHK(oc_spake_calc_pB(&bad_Y, &bad_pubB, &w0));
  MBEDTLS_MPI_CHK(
    mbedtls_ecp_point_write_binary(&grp, &bad_Y, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                   &cmplen, cmpbuf, sizeof(cmpbuf)));
  // check the value of Y is NOT correct
  assert(memcmp(bytes_Y, cmpbuf, cmplen) != 0);

  // ================================
  // Check that party A can calculate
  // the shared secret key material
  // ================================

  mbedtls_ecp_point Z;
  mbedtls_ecp_point_init(&Z);

  // Z = h*x*(Y - w0*N)
  MBEDTLS_MPI_CHK(calculate_ZV_N(&Z, &x, &Y, &w0));

  MBEDTLS_MPI_CHK(mbedtls_ecp_point_write_binary(
    &grp, &Z, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));
  assert(memcmp(bytes_Z, cmpbuf, cmplen) == 0);

  mbedtls_ecp_point V;
  mbedtls_ecp_point_init(&V);

  mbedtls_mpi w1;
  mbedtls_mpi_init(&w1);
  mbedtls_mpi_read_binary(&w1, bytes_w1, sizeof(bytes_w1));

  // V = h*w1*(Y - w0*N)
  MBEDTLS_MPI_CHK(calculate_ZV_N(&V, &w1, &Y, &w0));
  MBEDTLS_MPI_CHK(mbedtls_ecp_point_write_binary(
    &grp, &V, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));
  assert(memcmp(bytes_V, cmpbuf, cmplen) == 0);

  // ================================
  // Check that party B can calculate
  // the shared secret key material
  // ================================

  // Z = h*y*(X - w0*M)
  MBEDTLS_MPI_CHK(calculate_Z_M(&Z, &y, &X, &w0));
  MBEDTLS_MPI_CHK(mbedtls_ecp_point_write_binary(
    &grp, &Z, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));
  assert(memcmp(bytes_Z, cmpbuf, cmplen) == 0);

  // V = h*y*L, where L = w1*P
  mbedtls_ecp_point L;
  mbedtls_ecp_point_init(&L);

  MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&grp, &L, &w1, &grp.G,
                                  mbedtls_ctr_drbg_random, &ctr_drbg_ctx));
  MBEDTLS_MPI_CHK(
    mbedtls_ecp_mul(&grp, &V, &y, &L, mbedtls_ctr_drbg_random, &ctr_drbg_ctx));

  MBEDTLS_MPI_CHK(mbedtls_ecp_point_write_binary(
    &grp, &V, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));
  assert(memcmp(bytes_V, cmpbuf, cmplen) == 0);

  // ====================
  // Calculate transcript
  // ====================

  uint8_t ttbuf[2048];
  size_t ttlen = 0;
  // Context
  ttlen += encode_string(Context, ttbuf + ttlen);
  // A
  ttlen += encode_string(A, ttbuf + ttlen);
  // B
  ttlen += encode_string(B, ttbuf + ttlen);
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
  ttlen += encode_point(&grp, &Y, ttbuf + ttlen);
  // Z
  ttlen += encode_point(&grp, &Z, ttbuf + ttlen);
  // V
  ttlen += encode_point(&grp, &V, ttbuf + ttlen);
  // w0
  ttlen += encode_mpi(&w0, ttbuf + ttlen);

  // ===================
  // Calculate Key & Key
  //     Confirmation
  // ===================
  uint8_t Ka_Ke[32];

  mbedtls_sha256(ttbuf, ttlen, Ka_Ke, 0);
  // first half of Ka_Ke
  assert(memcmp(Ka, Ka_Ke, 16) == 0);
  // second half of Ka_Ke
  assert(memcmp(Ke, Ka_Ke + 16, 16) == 0);

  // Calculate KcA, KcB

  // |KcA| + |KcB| = 16 bytes
  uint8_t KcA_KcB[32];
  mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), NULL, 0, Ka,
               sizeof(Ka), "ConfirmationKeys", strlen("ConfirmationKeys"),
               KcA_KcB, 32);

  assert(memcmp(KcA, KcA_KcB, 16) == 0);
  assert(memcmp(KcB, KcA_KcB + 16, 16) == 0);

  // Calculate cA and cB
  uint8_t test_cA[32];
  uint8_t test_cB[32];
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), KcA,
                  sizeof(KcA), bytes_Y, sizeof(bytes_Y), test_cA);
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), KcB,
                  sizeof(KcB), bytes_X, sizeof(bytes_X), test_cB);
  assert(memcmp(cA, test_cA, 32) == 0);
  assert(memcmp(cB, test_cB, 32) == 0);

cleanup:
  return ret;
}