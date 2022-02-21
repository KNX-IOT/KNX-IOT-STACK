#include "gtest/gtest.h"
#include "mbedtls/md.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/pkcs5.h"

#include "security/oc_spake2plus.h"

class Spake2Plus : public ::testing::Test {
	virtual void SetUp() { oc_spake_init() ;};

	virtual void TearDown() { oc_spake_free() ;};
};



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
  /*
  uint8_t bytes_L[] = { 0x04, 0x95, 0x64, 0x5c, 0xfb, 0x74, 0xdf, 0x6e, 0x58,
                        0xf9, 0x74, 0x8b, 0xb8, 0x3a, 0x86, 0x62, 0x0b, 0xab,
                        0x7c, 0x82, 0xe1, 0x07, 0xf5, 0x7d, 0x68, 0x70, 0xda,
                        0x8c, 0xbc, 0xb2, 0xff, 0x9f, 0x70, 0x63, 0xa1, 0x4b,
                        0x64, 0x02, 0xc6, 0x2f, 0x99, 0xaf, 0xcb, 0x97, 0x06,
                        0xa4, 0xd1, 0xa1, 0x43, 0x27, 0x32, 0x59, 0xfe, 0x76,
                        0xf1, 0xc6, 0x05, 0xa3, 0x63, 0x97, 0x45, 0xa9, 0x21,
                        0x54, 0xb9 };
                        */

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