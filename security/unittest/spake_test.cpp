#include "gtest/gtest.h"
#include "mbedtls/md.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/pkcs5.h"

extern "C" {
#include "security/oc_spake2plus.h"

// Use implementation of Spake2+ with testing context
int calc_transcript_initiator(mbedtls_mpi *w0, mbedtls_mpi *w1, mbedtls_mpi *x,
                              mbedtls_ecp_point *X,
                              const uint8_t Y_enc[kPubKeySize],
                              uint8_t Ka_Ke[32], bool use_testing_context);

int calc_transcript_responder(spake_data_t *spake_data,
                              uint8_t X_enc[kPubKeySize], mbedtls_ecp_point *Y,
                              bool use_testing_context);
}

static mbedtls_entropy_context entropy_ctx;
static mbedtls_ctr_drbg_context ctr_drbg_ctx;
static mbedtls_ecp_group grp;

class Spake2Plus : public ::testing::Test {
protected:
  virtual void SetUp()
  {
    oc_spake_init();
    int ret = 0;
    // initialize entropy and drbg contexts
    mbedtls_entropy_init(&entropy_ctx);
    mbedtls_ctr_drbg_init(&ctr_drbg_ctx);
    mbedtls_ecp_group_init(&grp);

    MBEDTLS_MPI_CHK(mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1));
    MBEDTLS_MPI_CHK(mbedtls_ctr_drbg_seed(&ctr_drbg_ctx, mbedtls_entropy_func,
                                          &entropy_ctx, NULL, 0));
  cleanup:
    return;
  };

  virtual void TearDown()
  {
    oc_spake_free();
    mbedtls_ctr_drbg_free(&ctr_drbg_ctx);
    mbedtls_entropy_free(&entropy_ctx);
    mbedtls_ecp_group_free(&grp);
    return;
  };
};

// Test Vector values from Spake2+ draft 04 - this is the only test vector that
// uses null identities for A and B
const char Context[] = "SPAKE2+-P256-SHA256-HKDF draft-01";
const char A[] = "";
const char B[] = "";

const uint8_t bytes_w0[] = { 0xe6, 0x88, 0x7c, 0xf9, 0xbd, 0xfb, 0x75, 0x79,
                             0xc6, 0x9b, 0xf4, 0x79, 0x28, 0xa8, 0x45, 0x14,
                             0xb5, 0xe3, 0x55, 0xac, 0x03, 0x48, 0x63, 0xf7,
                             0xff, 0xaf, 0x43, 0x90, 0xe6, 0x7d, 0x79, 0x8c };
const uint8_t bytes_w1[] = { 0x24, 0xb5, 0xae, 0x4a, 0xbd, 0xa8, 0x68, 0xec,
                             0x93, 0x36, 0xff, 0xc3, 0xb7, 0x8e, 0xe3, 0x1c,
                             0x57, 0x55, 0xbe, 0xf1, 0x75, 0x92, 0x27, 0xef,
                             0x53, 0x72, 0xca, 0x13, 0x9b, 0x94, 0xe5, 0x12 };

const uint8_t bytes_x[] = { 0x5b, 0x47, 0x86, 0x19, 0x80, 0x4f, 0x49, 0x38,
                            0xd3, 0x61, 0xfb, 0xba, 0x3a, 0x20, 0x64, 0x87,
                            0x25, 0x22, 0x2f, 0x0a, 0x54, 0xcc, 0x4c, 0x87,
                            0x61, 0x39, 0xef, 0xe7, 0xd9, 0xa2, 0x17, 0x86 };

const uint8_t bytes_X[] = {
  0x04, 0xa6, 0xdb, 0x23, 0xd0, 0x01, 0x72, 0x3f, 0xb0, 0x1f, 0xcf, 0xc9, 0xd0,
  0x87, 0x46, 0xc3, 0xc2, 0xa0, 0xa3, 0xfe, 0xff, 0x86, 0x35, 0xd2, 0x9c, 0xad,
  0x28, 0x53, 0xe7, 0x35, 0x86, 0x23, 0x42, 0x5c, 0xf3, 0x97, 0x12, 0xe9, 0x28,
  0x05, 0x45, 0x61, 0xba, 0x71, 0xe2, 0xdc, 0x11, 0xf3, 0x00, 0xf1, 0x76, 0x0e,
  0x71, 0xeb, 0x17, 0x70, 0x21, 0xa8, 0xf8, 0x5e, 0x78, 0x68, 0x90, 0x71, 0xcd
};

const uint8_t bytes_y[] = { 0x76, 0x67, 0x70, 0xda, 0xd8, 0xc8, 0xee, 0xcb,
                            0xa9, 0x36, 0x82, 0x3c, 0x0a, 0xed, 0x04, 0x4b,
                            0x8c, 0x3c, 0x4f, 0x76, 0x55, 0xe8, 0xbe, 0xec,
                            0x44, 0xa1, 0x5d, 0xcb, 0xca, 0xf7, 0x8e, 0x5e };

const uint8_t bytes_Y[] = {
  0x04, 0x39, 0x0d, 0x29, 0xbf, 0x18, 0x5c, 0x3a, 0xbf, 0x99, 0xf1, 0x50, 0xae,
  0x7c, 0x13, 0x38, 0x8c, 0x82, 0xb6, 0xbe, 0x0c, 0x07, 0xb1, 0xb8, 0xd9, 0x0d,
  0x26, 0x85, 0x3e, 0x84, 0x37, 0x4b, 0xbd, 0xc8, 0x2b, 0xec, 0xdb, 0x97, 0x8c,
  0xa3, 0x79, 0x2f, 0x47, 0x24, 0x24, 0x10, 0x6a, 0x25, 0x78, 0x01, 0x27, 0x52,
  0xc1, 0x19, 0x38, 0xfc, 0xf6, 0x0a, 0x41, 0xdf, 0x75, 0xff, 0x7c, 0xf9, 0x47
};

const uint8_t bytes_Z[] = {
  0x04, 0x0a, 0x15, 0x0d, 0x9a, 0x62, 0xf5, 0x14, 0xc9, 0xa1, 0xfe, 0xdd, 0x78,
  0x2a, 0x02, 0x40, 0xa3, 0x42, 0x72, 0x10, 0x46, 0xce, 0xfb, 0x11, 0x11, 0xc3,
  0xad, 0xb3, 0xbe, 0x89, 0x3c, 0xe9, 0xfc, 0xd2, 0xff, 0xa1, 0x37, 0x92, 0x2f,
  0xcf, 0x8a, 0x58, 0x8d, 0x0f, 0x76, 0xba, 0x9c, 0x55, 0xc8, 0x5d, 0xa2, 0xaf,
  0x3f, 0x1c, 0x78, 0x9c, 0xa1, 0x79, 0x76, 0x81, 0x03, 0x87, 0xfb, 0x1d, 0x7e
};

const uint8_t bytes_V[] = {
  0x04, 0xf8, 0xe2, 0x47, 0xcc, 0x26, 0x3a, 0x18, 0x46, 0x27, 0x2f, 0x5a, 0x3b,
  0x61, 0xb6, 0x8a, 0xa6, 0x0a, 0x5a, 0x26, 0x65, 0xd1, 0x0c, 0xd2, 0x2c, 0x89,
  0xcd, 0x6b, 0xad, 0x05, 0xdc, 0x0e, 0x5e, 0x65, 0x0f, 0x21, 0xff, 0x01, 0x71,
  0x86, 0xcc, 0x92, 0x65, 0x1a, 0x4c, 0xd7, 0xe6, 0x6c, 0xe8, 0x8f, 0x52, 0x92,
  0x99, 0xf3, 0x40, 0xea, 0x80, 0xfb, 0x90, 0xa9, 0xba, 0xd0, 0x94, 0xe1, 0xa6
};

const uint8_t Ka[] = { 0x59, 0x29, 0xa3, 0xce, 0x98, 0x22, 0xc8, 0x14,
                       0x01, 0xbf, 0x0f, 0x76, 0x4f, 0x69, 0xaf, 0x08 };
const uint8_t Ke[] = { 0xea, 0x32, 0x76, 0xd6, 0x83, 0x34, 0x57, 0x60,
                       0x97, 0xe0, 0x4b, 0x19, 0xee, 0x5a, 0x3a, 0x8b };

const uint8_t KcA[] = { 0x7f, 0x84, 0xb9, 0x39, 0xd6, 0x00, 0x11, 0x72,
                        0x56, 0xb0, 0xc8, 0xa6, 0xd4, 0x0c, 0xf1, 0x81 };
const uint8_t KcB[] = { 0xf7, 0xd7, 0x54, 0x7c, 0xed, 0x93, 0xf6, 0x81,
                        0xe8, 0xdf, 0x4c, 0x25, 0x8c, 0x45, 0x16, 0xfd };

const uint8_t cA[] = { 0x71, 0xd9, 0x41, 0x27, 0x79, 0xb6, 0xc4, 0x5a,
                       0x2c, 0x61, 0x5c, 0x9d, 0xf3, 0xf1, 0xfd, 0x93,
                       0xdc, 0x0a, 0xaf, 0x63, 0x10, 0x4d, 0xa8, 0xec,
                       0xe4, 0xaa, 0x1b, 0x5a, 0x3a, 0x41, 0x5f, 0xea };
const uint8_t cB[] = { 0x09, 0x5d, 0xc0, 0x40, 0x03, 0x55, 0xcc, 0x23,
                       0x3f, 0xde, 0x74, 0x37, 0x81, 0x18, 0x15, 0xb3,
                       0xc1, 0x52, 0x4a, 0xae, 0x80, 0xfd, 0x4e, 0x68,
                       0x10, 0xcf, 0x53, 0x1c, 0xf1, 0x1d, 0x20, 0xe3 };

#define ASSERT_RET(x)                                                          \
  do {                                                                         \
    ASSERT_EQ(x, 0);                                                           \
  } while (0)
#define EXPECT_RET(x)                                                          \
  do {                                                                         \
    EXPECT_EQ(x, 0);                                                           \
  } while (0)

TEST_F(Spake2Plus, CalculatePublicA)
{
  // =========================
  // Check that X = x*P + w0*M
  // =========================
  mbedtls_mpi x, w0;
  mbedtls_mpi_init(&x);
  mbedtls_mpi_init(&w0);
  uint8_t cmpbuf[128];
  size_t cmplen;

  ASSERT_RET(mbedtls_mpi_read_binary(&x, bytes_x, sizeof(bytes_x)));
  ASSERT_RET(mbedtls_mpi_read_binary(&w0, bytes_w0, sizeof(bytes_w0)));

  mbedtls_ecp_point X, pubA;
  mbedtls_ecp_point_init(&X);
  mbedtls_ecp_point_init(&pubA);
  // pubA = x*P (P is the generator group element, mbedtls uses G)
  ASSERT_RET(mbedtls_ecp_mul(&grp, &pubA, &x, &grp.G, mbedtls_ctr_drbg_random,
                             &ctr_drbg_ctx));

  // X = pubA + w0*M
  ASSERT_RET(oc_spake_calc_pA(&X, &pubA, &w0));
  ASSERT_RET(mbedtls_ecp_point_write_binary(
    &grp, &X, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));

  // check the value of X is correct
  ASSERT_TRUE(0 == memcmp(bytes_X, cmpbuf, cmplen));
  mbedtls_mpi_free(&x);
  mbedtls_mpi_free(&w0);
}

TEST_F(Spake2Plus, CalculatePublicB)
{
  // =========================
  // Check that Y = y*P + w0*N
  // =========================
  mbedtls_mpi y, w0;
  mbedtls_mpi_init(&y);
  mbedtls_mpi_init(&w0);
  uint8_t cmpbuf[128];
  size_t cmplen;

  ASSERT_RET(mbedtls_mpi_read_binary(&w0, bytes_w0, sizeof(bytes_w0)));

  ASSERT_RET(mbedtls_mpi_read_binary(&y, bytes_y, sizeof(bytes_y)));

  mbedtls_ecp_point Y, pubB;
  mbedtls_ecp_point_init(&Y);
  mbedtls_ecp_point_init(&pubB);
  // pubB = y*P
  ASSERT_RET(mbedtls_ecp_mul(&grp, &pubB, &y, &grp.G, mbedtls_ctr_drbg_random,
                             &ctr_drbg_ctx));

  // Y = pubB + w0*N
  ASSERT_RET(oc_spake_calc_pB(&Y, &pubB, &w0));
  ASSERT_RET(mbedtls_ecp_point_write_binary(
    &grp, &Y, MBEDTLS_ECP_PF_UNCOMPRESSED, &cmplen, cmpbuf, sizeof(cmpbuf)));
  // check the value of Y is correct
  ASSERT_TRUE(memcmp(bytes_Y, cmpbuf, cmplen) == 0);

  mbedtls_mpi_free(&y);
  mbedtls_mpi_free(&w0);
}

TEST_F(Spake2Plus, CalculateSecretA)
{
  mbedtls_mpi w0, w1, x;
  mbedtls_ecp_point X;
  uint8_t Ka_Ke[32];

  mbedtls_mpi_init(&w0);
  mbedtls_mpi_init(&w1);
  mbedtls_mpi_init(&x);
  mbedtls_ecp_point_init(&X);

  ASSERT_RET(mbedtls_mpi_read_binary(&w0, bytes_w0, sizeof(bytes_w0)));
  ASSERT_RET(mbedtls_mpi_read_binary(&w1, bytes_w1, sizeof(bytes_w1)));
  ASSERT_RET(mbedtls_mpi_read_binary(&x, bytes_x, sizeof(bytes_x)));
  ASSERT_RET(mbedtls_ecp_point_read_binary(&grp, &X, bytes_X, sizeof(bytes_X)));

  ASSERT_RET(calc_transcript_initiator(&w0, &w1, &x, &X, bytes_Y, Ka_Ke, true));

  // bummer, test vector aint workin
  // tomorrow - look at transcript and compaaaaaaaaaaaaaaaaaaaaaaaare
  EXPECT_TRUE(memcmp(Ka, Ka_Ke, 16) == 0);
  EXPECT_TRUE(memcmp(Ke, Ka_Ke + 16, 16) == 0);

  mbedtls_mpi_free(&w0);
  mbedtls_mpi_free(&w1);
  mbedtls_mpi_free(&x);
  mbedtls_ecp_point_free(&X);
}

#if 0
int
oc_spake_test_vector()
{
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
               sizeof(Ka), (const unsigned char *)"ConfirmationKeys",
               strlen("ConfirmationKeys"), KcA_KcB, 32);

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
#endif