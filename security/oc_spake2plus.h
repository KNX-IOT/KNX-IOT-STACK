#ifndef OC_SPAKE2PLUS_H
#define OC_SPAKE2PLUS_H

#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"

/**
 * @brief Initialize Spake2+
 *
 * @return int 0 on success
 */
int oc_spake_init(void);

/**
 * @brief Deinitialize Spake2+
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
int oc_spake_parameter_exchange(oc_string_t *rnd, oc_string_t *salt, int *it);

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
 * @brief Calculate the w0 & w1 constants
 *
 * Uses PBKDF2 with SHA256 & HMAC to calculate a 40-byte output which is
 * converted into w0 and w1.
 *
 * @param pw the null-terminated password
 * @param salt 32-byte array containing the salt
 * @param it the number of iterations to perform within PBKDF2
 * @param w0 the w0 constant as defined by SPAKE2+. Must be initialized by the
 * caller.
 * @param L the L constant as defined by SPAKE2+. Must be initialized by the
 * caller.
 * @return int 0 on success, mbedtls error code on failure
 */
int oc_spake_calc_w0_L(const char *pw, size_t len_salt, const uint8_t *salt,
                       int it, mbedtls_mpi *w0, mbedtls_ecp_point *L);

#endif // OC_SPAKE2PLUS_H