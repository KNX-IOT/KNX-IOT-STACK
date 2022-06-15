/*
// Copyright (c) 2018 Intel Corporation
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
 * @brief public key infrastructure (PKI) functions
 * @file
 *
 * public key infrastructure (PKI) functions
 *
 * Collection of functions used to add public key infrastructure (PKI)
 * support to devices.
 *
 * This is work in progress
 */
#ifndef OC_PKI_H
#define OC_PKI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * defined security profiles
 *
 * Security Profiles differentiate devices based on requirements.
 *
 */
typedef enum {
  OC_SP_BASELINE = 1 << 1, ///< The Baseline Security Profile
  OC_SP_BLACK = 1 << 2,    ///< The Black Security Profile
  OC_SP_BLUE = 1 << 3,     ///< The Blue Security Profile
  OC_SP_PURPLE = 1 << 4    ///< The Purple Security Profile
} oc_sp_types_t;

/**
 * Add the manufactures PKI identity certificate.
 *
 * @param[in] device index of the logical device the identity certificate
 *                   belongs to
 * @param[in] cert pointer to a string containing a PEM encoded identity
 *                 certificate
 * @param[in] cert_size the size of the `cert` string
 * @param[in] key the PEM encoded private key associated with this certificate
 * @param[in] key_size the size of the `key` string
 *
 * @return
 *  - the credential ID of the /oic/sec/cred entry containing the certificate
 *    chain
 *  - `-1` on failure
 */
int oc_pki_add_mfg_cert(size_t device, const unsigned char *cert,
                        size_t cert_size, const unsigned char *key,
                        size_t key_size);

/**
 * Add an intermediate manufacture CA certificate.
 *
 * @param[in] device index of the logical device the certificate chain belongs
 * to
 * @param[in] credid the credential ID of the /oic/sec/cred entry containing the
 *                   end-entity certificate
 * @param[in] cert pointer to a string containing a PEM encoded certificate
 * @param[in] cert_size the size of the `cert` string
 *
 * @return
 *   - the credential ID of the /oic/sec/cred entry containing the certificate
 *     chain
 *   - `-1` on failure
 */
int oc_pki_add_mfg_intermediate_cert(size_t device, int credid,
                                     const unsigned char *cert,
                                     size_t cert_size);

/**
 * Add manufacture trust anchor CA
 *
 * @param[in] device index of the logical device the trust anchor CA belongs to
 * @param[in] cert pointer to a string containing a PEM encoded certificate
 * @param[in] cert_size the size of the `cert` string
 *
 * @return
 *  - the credential ID of the /oic/sec/cred entry containing the certificate
 *    chain
 *  - `-1` on failure
 */
int oc_pki_add_mfg_trust_anchor(size_t device, const unsigned char *cert,
                                size_t cert_size);

/**
 * Add trust anchor CA
 *
 * @param[in] device index of the logical device the trust anchor CA belongs to
 * @param[in] cert pointer to a string containing a PEM encoded certificate
 * @param[in] cert_size the size of the `cert` strung
 *
 * @return
 *  - the credential ID of the /oic/sec/cred entry containing the certificate
 *    chain
 *  - `-1` on failure
 */
int oc_pki_add_trust_anchor(size_t device, const unsigned char *cert,
                            size_t cert_size);

/**
 * Set the Security Profile
 *
 * The Security Specification defines several Security Profiles that can be
 * selected based on the security requirements of different verticals such as
 * such as industrial, health care, or smart home.
 *
 * There are currently five types of Security Profiles.
 *
 *
 * @param[in] device index of the logical device the security profile is be set
 * on
 * @param[in] supported_profiles a bitwise OR list of oc_sp_types_t that are
 *                               supported by the device. The current_profile
 *                               value may be changed to one of the other
 *                               supported_profiles during the onboarding
 *                               process.
 * @param[in] current_profile the currently selected security profile
 * @param[in] mfg_credid the credential ID of the entry containing
 *                       the manufactures end-entity certificate
 */
void oc_pki_set_security_profile(size_t device,
                                 oc_sp_types_t supported_profiles,
                                 oc_sp_types_t current_profile, int mfg_credid);
#ifdef __cplusplus
}
#endif
#endif /* OC_PKI_H */
