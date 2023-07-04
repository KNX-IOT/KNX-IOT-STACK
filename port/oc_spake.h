/*
// Copyright (c) 2023 Cascoda Ltd.
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

#ifndef OC_SPAKE_H
#define OC_SPAKE_H

#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"
#include "oc_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

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
int oc_spake_get_parameters(uint8_t rnd[32], uint8_t salt[32], int *it, mbedtls_mpi *w0, mbedtls_ecp_point *L);

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
int oc_spake_set_parameters(uint8_t rnd[32], uint8_t salt[32], int it, mbedtls_mpi w0, mbedtls_ecp_point L);


#ifdef __cplusplus
}
#endif

#endif // OC_SPAKE_H