/*
// Copyright (c) 2022-2023 Cascoda Ltd
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
  @brief client code for the device (s-mode)
  @file

  compile flag:
  - OC_USE_MULTICAST_SCOPE_2
    also sends the multicast group events with scope =2
    this is needed when the devices are running on the same PC
*/
#ifndef OC_KNX_CLIENT_INTERNAL_H
#define OC_KNX_CLIENT_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief the spake response callback type
 * e.g. function prototype that is called when the spake handshake is finished
 *
 * @param error 0 = ok
 * @param serial_number the serial number of the device on the other side
 * @param oscore_id the oscore identifier (bytes)
 * @param oscore_id_size the size in bytes of the oscore identifier
 * @param secret the negotiated secret (bytes)
 * @param secret_size the size in bytes of the secret
 */
typedef void (*oc_spake_cb_t)(int error, char *serial_number, char *oscore_id,
                              int oscore_id_size, uint8_t *secret,
                              int secret_size);

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
 * NOTE: recipient id in HEX string (e.g. null terminated)
 *
 * @param endpoint the endpoint of the device to be used
 * @param password the spake password to be used
 * @param recipient_id the recipient id (HEX string)
 * @return int success full start up of the handshake
 */
int oc_initiate_spake(oc_endpoint_t *endpoint, char *password,
                      char *recipient_id);

/**
 * @brief initiate the spake handshake
 *
 * NOTE: After the success full handshake the OSCORE context should have:
 * - SID : serial number as byte array
 * - RID : the recipient ID as given input
 *
 * @param endpoint the endpoint of the device to be used
 * @param serial_number the serial number of the device, to put back in the
 * callback, this is a string, e.g. SN as HEX string e.g. "00FA10010701"
 * @param password the spake password to be used
 * @param recipient_id the recipient ID id for the resulting OSCORE context
 * (byte string)
 * @param recipient_id_len length of the recipient ID byte string
 * @return int success full start up of the handshake
 */
int oc_initiate_spake_parameter_request(oc_endpoint_t *endpoint,
                                        char *serial_number, char *password,
                                        char *recipient_id,
                                        size_t recipient_id_len);

typedef void (*oc_s_mode_response_cb_t)(char *url, oc_rep_t *rep,
                                        oc_rep_t *rep_value);

/**
  @defgroup doc_module_tag_s_mode_server s-mode server
  S-mode server side support functions.

  This module contains the receiving side of the s-mode functionality.
  The received s-mode messages are routed to the appropriate POST methods of the
  data point. However since not all data is in the s-mode message the POST
  method needs to retrieve the data from the s-mode message differently than
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
 * @brief checks if the request is a redirected request from /k or /p
 * when that happened, extra information can be in the CBOR object
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
 * @brief sends out an s-mode read request.
 * The read request has no data value
 *
 * Note: function does not check the flags on the resources
 *
 * @see oc_do_s_mode_with_scope
 * @param group_address the group address to invoke a read on
 */
void oc_do_s_mode_read(int64_t group_address);

/**
 * @brief sends (transmits) an s-mode message
 * the value comes from the GET of the resource indicated by the resource_url
 * the path is "k"
 * the sia (sender individual address) is taken from the device
 * the ga is coming from the group address table that is listing the resource
 * url (path) the url of the resource to obtain the value from.
 *
 * only the first group address is used to send the s-mode message
 * for the recipient table all entries are used to send the unicast
 * communication.
 *
 * Note: function does check the T flag on the resource
 *       if the T flag is not set, then the message is NOT send.
 *
 * @param scope the multi-cast scope
 * @param resource_url URI of the resource (e.g. implemented on the device that
 * is calling this function)
 * @param rp the "st" value to send e.g. "w" | "rp" | "r"
 */
void oc_do_s_mode_with_scope(int scope, const char *resource_url, char *rp);

/**
 * @brief sends (transmits) an s-mode message
 * the value comes from the GET of the resource indicated by the resource_url
 * the path is "k"
 * the sia (sender individual address) is taken from the device
 * the ga is coming from the group address table that is listing the resource
 * url (path) the url of the resource to obtain the value from.
 *
 * only the first group address is used to send the s-mode message
 * for the recipient table all entries are used to send the unicast
 * communication.
 *
 * Note: function does NOT check the T flag on the resource
 *      e.g. always send the s-mode message
 *      used in case the rp value = "rp", e.g.sending a response on read ("r")
 *
 * @param scope the multi-cast scope
 * @param resource_url URI of the resource (e.g. implemented on the device that
 * is calling this function)
 * @param rp the "st" value to send e.g. "w" | "rp" | "r"
 */
void oc_do_s_mode_with_scope_no_check(int scope, const char *resource_url,
                                      char *rp);

/** @} */ // end of doc_module_tag_s_mode_client

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_CLIENT_INTERNAL_H */
