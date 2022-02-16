/*
// Copyright (c) 2022 Cascoda Ltd
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

#ifndef OC_KNX_CLIENT_INTERNAL_H
#define OC_KNX_CLIENT_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*oc_spake_cb_t)(int error, uint8_t *secret, int secret_size);

/**
 * @brief set the spake response callback
 * e.g. function is called when the spake handshake is finished
 *
 * @param my_func the callback function
 * @return true function set
 * @return false function set failed
 */
bool oc_set_spake_response_cb(oc_spake_cb_t my_func);

/**
 * @brief initiate the spake handshake
 *
 * @param endpoint the endpoint of the device to be used
 * @param password the spake password to be used
 * @return int success full start up of the handshake
 */
int oc_initiate_spake(oc_endpoint_t *endpoint, char* password);

typedef void (*oc_s_mode_response_cb_t)(char *url, oc_rep_t *rep,
                                        oc_rep_t *rep_value);

/**
 * @brief set the s-mode response callback
 * e.g. function is called when a s-mode response is coming back
 *
 * @param my_func the callback function
 * @return true function set
 * @return false function set failed
 */
bool oc_set_s_mode_response_cb(oc_s_mode_response_cb_t my_func);

/**
 * @brief retrieve the callback function
 *
 * @return oc_s_mode_response_cb_t the callback function that has been set
 */
oc_s_mode_response_cb_t oc_get_s_mode_response_cb();

/**
 * @brief checks if the request is a redirected request from .knx
 * all messages to .knx are s-mode messages with an encapsulating payload
 *
 * @param request the request to be checked
 * @return true
 * @return false
 */
bool oc_is_s_mode_request(oc_request_t *request);

/**
 * @brief parses out the value of the s-mode request.
 *
 * @param request the request
 * @return oc_rep_t* the rep
 */
oc_rep_t *oc_s_mode_get_value(oc_request_t *request);

/**
 * @brief sends an s-mode message
 * the value comes from the GET of the resource indicated by the resource_url
 * The uri is hard coded to use ALL CoAP nodes (TODO).
 * the path is ".knx"
 * the sia (sender individual address) is taken from the device
 * the ga is coming from the group address table that is listing the resource
 * url (path) if more than one entry in the group object table, then all group
 * address are used to send the POST request too.
 *
 * @param resource_url URI of the resource (e.g. implemented on the device that
 * is calling this function)
 * @param rp the "st" value to send e.g. "w" | "rp" | "r"
 */
void oc_do_s_mode(char *resource_url, char *rp);

/**
 * @brief sends an s-mode message
 * the value comes from the GET of the resource indicated by the resource_url
 * The uri is hard coded to use ALL CoAP nodes (TODO).
 * the path is ".knx"
 * the sia (sender individual address) is taken from the device
 * the ga is coming from the group address table that is listing the resource
 * url (path) if more than one entry in the group object table, then all group
 * address are used to send the POST request too.
 *
 * @param scope the multi-cast scope
 * @param resource_url URI of the resource (e.g. implemented on the device that
 * is calling this function)
 * @param rp the "st" value to send e.g. "w" | "rp" | "r"
 */
void oc_do_s_mode_with_scope(int scope, char *resource_url, char *rp);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_CLIENT_INTERNAL_H */
