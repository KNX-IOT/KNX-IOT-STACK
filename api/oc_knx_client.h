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
/**
  @file
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
 * @param oscore_id optional OSCORE id for the handshake. Set to NULL if unused
 * @return int success full start up of the handshake
 */
int oc_initiate_spake(oc_endpoint_t *endpoint, char *password, char *oscore_id);

typedef void (*oc_s_mode_response_cb_t)(char *url, oc_rep_t *rep,
                                        oc_rep_t *rep_value);

/**
  @defgroup doc_module_tag_s_mode_server s-mode server
  S-mode server side support functions.

  This module contains the receiving side of the s-mode functionality.
  The received s-mode messages are routed to the appropriate POST methods of the
  data point. However since not all data is in the s-mode message the POST
  method needs to retrieve the data from the s-mode messsage differently than
  for an normal CoAP post message (the message payload is constructed
  differently).

  @{
*/

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
 * @brief checks if the request is a redirected request from /.knx or /p
 * when that happend, extra information can be in the CBOR object
 *
 * @param request the request to be checked
 * @return true
 * @return false
 */
bool oc_is_redirected_request(oc_request_t *request);

/**
  @defgroup doc_module_tag_s_mode_client s-mode client
  S-mode Client side support functions.

  This module contains the sending side of the s-mode functionality.
  The s-mode messages are send from the device that implements a resource with
  the CoAP GET functionality. The s-mode functions will retrieve the data values
  and place it in the s-mode message. The s-mode message will only be send to
  the groups that are listed in the Group Object Table with the appropriate
  flags.

  @{
*/

/**
 * @brief parses out the value of the s-mode request.
 *
 * @param request the request
 * @return oc_rep_t* the rep
 */
oc_rep_t *oc_s_mode_get_value(oc_request_t *request);

/** @} */ // end of doc_module_tag_s_mode_server

/**
 * @brief sends an s-mode message
 * the value comes from the GET of the resource indicated by the resource_url
 * The uri the multicast address with group id and installation id incorporated
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
 * @brief sends out an s-mode read request.
 * The read request has no data value
 *
 * Note: function does not check the flags on the resources
 *
 * @see oc_do_s_mode_with_scope
 * @param group_address the group address to invoke a read on
 */
void oc_do_s_mode_read(size_t group_address);

/**
 * @brief sends (transmits) an s-mode message
 * the value comes from the GET of the resource indicated by the resource_url
 * the path is ".knx"
 * the sia (sender individual address) is taken from the device
 * the ga is coming from the group address table that is listing the resource
 * url (path) the url of the resource to obtain the value from.
 *
 * only the first group address is used to send the s-mode message
 * for the recipient table all entries are used to send the unicast
 * communication.
 *
 * Note: function does not check the flags on the resource
 *
 * @param scope the multi-cast scope
 * @param resource_url URI of the resource (e.g. implemented on the device that
 * is calling this function)
 * @param rp the "st" value to send e.g. "w" | "rp" | "r"
 */
void oc_do_s_mode_with_scope(int scope, char *resource_url, char *rp);

/** @} */ // end of doc_module_tag_s_mode_client

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_CLIENT_INTERNAL_H */
