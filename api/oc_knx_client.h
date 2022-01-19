/*
// Copyright (c) 2021 Cascoda Ltd
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


  

typedef void (*oc_s_mode_response_cb_t)(char *url, oc_rep_t *rep,
                                            oc_rep_t *rep_value);

bool oc_set_s_mode_response_cb(oc_s_mode_response_cb_t my_func);

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
 * the value comes from the GET of the resource
 * The uri is hard coded to use ALL CoAP nodes
 * the path is .knx
 * the sia (sender individual adress) is taken from the device
 * the ga is coming from the group address table that is listing the resource
 * url (path) if more than one entry in the group object table, then all group
 * address are used to send the POST request too.
 *
 * @param resource_url URI of the resource
 * @param rp the "st" value to send e.g. "w" | "rp" | "r"
 */
void oc_do_s_mode(char *resource_url, char *rp);

typedef void (*oc_discover_ia_cb_t)(int ia, oc_endpoint_t *endpoint);

int oc_knx_client_get_endpoint_from_ia(int ia, oc_discover_ia_cb_t my_func);


#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_CLIENT_INTERNAL_H */
