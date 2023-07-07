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
/**
  @brief security: spake2plus implementation
  @file
*/

#ifndef OC_SPAKE2PLUS_H
#define OC_SPAKE2PLUS_H

#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"
#include "oc_helpers.h"
#include "oscore_constants.h"

enum { kPubKeySize = 65 };

typedef struct
{
  mbedtls_mpi w0;
  mbedtls_ecp_point L;
  mbedtls_mpi y;
  mbedtls_ecp_point pub_y;
  uint8_t Ka_Ke[32];
} spake_data_t;

#define SPAKE_CONTEXT "knxpase"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Spake2+
 *
 * @return int 0 on success
 */
int oc_spake_init(void);

/**
 * @brief De-initialize Spake2+
 *
 * @return int 0 on success
 */
int oc_spake_free(void);

/**
 * @brief Verify the implementation of Spake2+ using the test vectors defined
 * within the spec.
 *
 * @return int 0 on successful self-test
 */
int oc_spake_test_vector();

/**
 * @brief Generate the fields needed for the PASE Parameter Exchange frame type.
 *
 * @ref oc_spake_init() must be called before this function can be used.
 *
 * @param rnd Random number
 * @param salt The salt to be used for PBKDF2
 * @param it The number of iterations to be used for PBKDF2
 * @return int 0 on success, mbedtls error code on failure
 */
int oc_spake_parameter_exchange(uint8_t rnd[32], uint8_t salt[32], int *it);

/**
 * @brief Get the pre-loaded fields needed for PASE and SPAKE
 *
 * @ref oc_spake_set_parameters() must be used to set these values
 *
 * @param rnd Random number
 * @param salt The salt to be used fo PBKDF2
 * @param it The number of iterations to be used for PBKDF2
 * @param w0 omega0 value for SPAKE2+
 * @param L L ecp point for SPAKE2+
 * @return int 0 on success
 */
int oc_spake_get_parameters(uint8_t rnd[32], uint8_t salt[32], int *it,
                            mbedtls_mpi *w0, mbedtls_ecp_point *L);

/**
 * @brief Set the pre-loaded fields needed for PASE and SPAKE
 *
 * @param rnd Random number
 * @param salt The salt to be used fo PBKDF2
 * @param it The number of iterations to be used for PBKDF2
 * @param w0 omega0 value for SPAKE2+
 * @param L L ecp point for SPAKE2+
 * @return int 0 on success
 */
int oc_spake_set_parameters(uint8_t rnd[32], uint8_t salt[32], int it,
                            mbedtls_mpi w0, mbedtls_ecp_point L);

/**
 * @brief get the PBKDF params for OC SPAKE
 *
 * @param rnd Random number
 * @param salt the salt to be used for PBKDF2
 * @param it The number of iterations to be used for PBKDF2
 * @return int 0 on success, mbedtls error code on failure
 */
int oc_spake_get_pbkdf_params(uint8_t rnd[32], uint8_t salt[32], int *it);

/**
 * @brief get the W0 and L values for SPAKE exchange
 *
 * Uses PBKDF2 with SHA256 & HMAC to calculate a 40-byte output which is
 * converted into w0 and w1.
 *
 * @param pw the null-terminated password
 * @param salt 32-byte array containing the salt
 * @param it the number of iterations to perform within PBKDF2
 * @param w0 the w0 parameter as defined by SPAKE2+. Must be initialized by the
 * caller.
 * @param L the L parameter as defined by SPAKE2+. Must be initialized by the
 * caller.
 * @return int 0 on success, mbedtls error code on failure
 */
int oc_spake_get_w0_L(const char *pw, size_t len_salt, const uint8_t *salt,
                      int it, mbedtls_mpi *w0, mbedtls_ecp_point *L);

/**
 * @brief Get the currently set Spake2+ password
 *
 * @return Null-terminated string holding the password
 */
const char *oc_spake_get_password();

/**
 * @brief Set the Spake2+ password
 *
 * @param new_pass Null-terminated string containing the password
 */
void oc_spake_set_password(char *new_pass);

/**
 * @brief Generate a 16-byte number, suitable for use as a masterkey within
 * OSCORE secure communication.
 *
 * oc_spake_init() MUST be called before this function can be used. If it is not
 * called, the RNG context will be uninitialised & this function should return
 * an error.
 *
 * @param array Array into which the masterkey will be written. Must be of
 * length OSCORE_KEY_LEN
 * @return int Zero on success, negative MBEDTLS error code on failure.
 */
int oc_gen_masterkey(uint8_t array[OSCORE_KEY_LEN]);

/**
 * @brief Calculate the w0 & L parameter
 *
 * Uses PBKDF2 with SHA256 & HMAC to calculate a 40-byte output which is
 * converted into w0 and w1.
 *
 * @param pw the null-terminated password
 * @param salt 32-byte array containing the salt
 * @param it the number of iterations to perform within PBKDF2
 * @param w0 the w0 parameter as defined by SPAKE2+. Must be initialized by the
 * caller.
 * @param L the L parameter as defined by SPAKE2+. Must be initialized by the
 * caller.
 * @return int 0 on success, mbedtls error code on failure
 */
int oc_spake_calc_w0_L(const char *pw, size_t len_salt, const uint8_t *salt,
                       int it, mbedtls_mpi *w0, mbedtls_ecp_point *L);

/**
 * @brief Calculate the w0 & w1 parameter
 *
 * Uses PBKDF2 with SHA256 & HMAC to calculate a 40-byte output which is
 * converted into w0 and w1.
 *
 * @param pw the null-terminated password
 * @param salt 32-byte array containing the salt
 * @param it the number of iterations to perform within PBKDF2
 * @param w0 the w0 parameter as defined by SPAKE2+. Must be initialized by the
 * caller.
 * @param w1 the w1 parameter as defined by SPAKE2+. Must be initialized by the
 * caller.
 * @return int 0 on success, mbedtls error code on failure
 */
int oc_spake_calc_w0_w1(const char *pw, size_t len_salt, const uint8_t *salt,
                        int it, mbedtls_mpi *w0, mbedtls_mpi *w1);

/**
 * @brief Generate an ECP keypair to be used within the Spake2+ handshake
 *
 * @param y The private part. Do not leak.
 * @param pub_y The public part.
 * @return int 0 on success, mbedtls error code on failure
 */
int oc_spake_gen_keypair(mbedtls_mpi *y, mbedtls_ecp_point *pub_y);

/**
 * @brief Calculate the Public Share of Party A, the Management Client
 *
 * @param pA Output public share
 * @param pubA Public key generated by Party A
 * @param w0 the w0 parameter, derived using the out-of-band secret
 */
int oc_spake_calc_pA(mbedtls_ecp_point *pA, const mbedtls_ecp_point *pubA,
                     const mbedtls_mpi *w0);

/**
 * @brief Calculate the Public Share of Party B, the KNX device
 *
 * @param pB Output public share
 * @param pubB Public key generated by Party B
 * @param w0 the w0 parameter, derived using the out-of-band secret
 */
int oc_spake_calc_pB(mbedtls_ecp_point *pB, const mbedtls_ecp_point *pubB,
                     const mbedtls_mpi *w0);

int oc_spake_encode_pubkey(mbedtls_ecp_point *P, uint8_t out[kPubKeySize]);

/**
 * @brief Calculate the shared secret on the Responder side (the KNX server)
 *
 * @param spake_data SPAKE2+ data structure, after receipt of Credential
 * Exchange message. The output of this function is spake_data.Ka_Ke
 * @param X_enc The X parameter (pA) encoded as binary data
 * @param Y The Y parameter (pB) encoded as an ECP point
 * @return int 0 on success, mbedtls error code on failure
 */
int oc_spake_calc_transcript_responder(spake_data_t *spake_data,
                                       const uint8_t X_enc[kPubKeySize],
                                       mbedtls_ecp_point *Y);

/**
 * @brief Calculate the shared secret on the Initiator side (the Management
 * Client)
 *
 * @param w0 The w0 parameter, derived from the password
 * @param w1 The w1 parameter, derived from the password
 * @param x The private key generated by the MaC
 * @param X The public key corresponding to x
 * @param Y_enc The public share of the Responder, encoded as binary data
 * @param Ka_Ke The output shared secret
 * @return int 0 on success, mbedtls error code on failure
 */
int oc_spake_calc_transcript_initiator(mbedtls_mpi *w0, mbedtls_mpi *w1,
                                       mbedtls_mpi *x, mbedtls_ecp_point *X,
                                       const uint8_t Y_enc[kPubKeySize],
                                       uint8_t Ka_Ke[32]);

int oc_spake_calc_cA(uint8_t *Ka_Ke, uint8_t cA[32],
                     uint8_t bytes_Y[kPubKeySize]);

int oc_spake_calc_cB(uint8_t *Ka_Ke, uint8_t cB[32],
                     uint8_t bytes_X[kPubKeySize]);

void oc_spake_print_point(mbedtls_ecp_point *p);

void oc_spake_print_mpi(mbedtls_mpi *m);

#ifdef __cplusplus
}
#endif

#endif // OC_SPAKE2PLUS_H