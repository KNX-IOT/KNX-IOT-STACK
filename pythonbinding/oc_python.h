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
/**
  @file
*/
#ifndef OC_PYTHON_H
#define OC_PYTHON_H

#ifdef DOXYGEN
// Force doxygen to document to ignore kisCS_EXPORT
#define kisCS_EXPORT
#endif

#ifdef WIN32
// use the (generated) DLL export macros
#include "kisCS_Export.h"
#else
// Force linux to ingore the kisCS_EXPORT
#define kisCS_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
@defgroup doc_module_tag_dll DLL
Group of exported functions, so that these functions can be used with a dll or
shared object.

@{
*/

/**
 * @brief the spake callback function type
 *
 */
typedef void (*spakeCB)(char *sn, int state, char *secret, int secret_size);

/**
 * @brief The changed callback function type
 *
 */
typedef void (*changedCB)(char *uuid, char *state, char *event);

/**
 * @brief The discovery callback function type
 *
 */
typedef void (*discoveryCB)(int payload_size, char *payload);

/**
 * @brief the resource callback function type
 *
 */
typedef void (*resourceCB)(char *anchor, char *uri, char *types,
                           char *interfaces);

/**
 * @brief the client callback function type
 *
 */
typedef void (*clientCB)(char *sn, int status, char *r_format, char *r_id,
                         char *url, int payload_size, char *payload);

/**
 * @brief returns the application max data size, e.g. data size for each call
 *
 * @return max data size
 */
kisCS_EXPORT long ets_get_max_app_data_size(void);

/**
 * @brief sets the maximum data size, e.g. the data size for each call
 *
 * @param data_size the data size
 */
kisCS_EXPORT void ets_set_max_app_data_size(int data_size);

/**
 * @brief install the changed callback
 *
 * @param changedCB the changed callback
 */
kisCS_EXPORT void ets_install_changedCB(changedCB changedCB);

/**
 * @brief install the resource callback (not yet used)
 *
 * @param resourceCB the resource callback
 */
kisCS_EXPORT void ets_install_resourceCB(resourceCB resourceCB);

/**
 * @brief install the client callback
 * This function is called for a response of any GET/PUT/POST/DELETE request.
 * @param clientCB the client callback
 */
kisCS_EXPORT void ets_install_clientCB(clientCB clientCB);

/**
 * @brief install the discovery callback
 * This function is called for a response on a discovery request.
 * @param discoveryCB the discovery callback
 */
kisCS_EXPORT void ets_install_discoveryCB(discoveryCB discoveryCB);

/**
 * @brief install the spake callback
 * This function is called when the spake handshake is finished (or failed)
 * @param spakeCB the spake callback
 */
kisCS_EXPORT void ets_install_spakeCB(spakeCB spakeCB);

/**
 * @brief issue a GET request with expected content type CBOR
 *
 * @param sn the serial number of the device (is unique?)
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string)
 */
kisCS_EXPORT void ets_cbor_get(char *sn, char *uri, char *query, char *r_id);

/**
 * @brief issue a GET request with expected content type CBOR (unsecured)
 *
 * @param sn the serial number of the device (is unique)
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string)
 */
kisCS_EXPORT void ets_cbor_get_unsecured(char *sn, char *uri, char *query,
                                         char *r_id);

/**
 * @brief issue a GET request with expected content type LINK-FORMAT
 *
 * @param sn the serial number of the device (is unique?)
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string)
 */
kisCS_EXPORT void ets_linkformat_get(char *sn, char *uri, char *query,
                                     char *r_id);

/*
 * @brief issue a GET request with expected content type LINK-FORMAT unsecured
 *
 * @param sn the serial number of the device (is unique)
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string)
 */
kisCS_EXPORT void ets_linkformat_get_unsecured(char *sn, char *uri, char *query,
                                               char *r_id);

/**
 * @brief issue a POST request, content type CBOR
 *
 * @param sn the serial number of the device (is unique?)
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string)
 * @param size the size of the data
 * @param data the request data (in cbor)
 */
kisCS_EXPORT void ets_cbor_post(char *sn, char *uri, char *query, char *r_id,
                                int size, char *data);

/**
 * @brief issue a PUT request, content type CBOR
 *
 * @param sn the serial number of the device (is unique?)
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string)
 * @param size the size of the data
 * @param data the request data (in cbor)
 */
kisCS_EXPORT void ets_cbor_put(char *sn, char *uri, char *query, char *r_id,
                               int size, char *data);

/**
 * @brief issue a DELETE request, content type CBOR
 *
 * @param sn the serial number of the device (is unique?)
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string)
 */
kisCS_EXPORT void ets_cbor_delete(char *sn, char *uri, char *query, char *r_id);

/**
 * @brief initiate the spake handshake
 *
 * @param sn the serial number of the device (is unique?)
 * @param password the password of the device
 */
kisCS_EXPORT void ets_initiate_spake(char *sn, char *password);

/**
 * @brief sends an s-mode message
 *
 * @param scope the multicast scope, [2,3,5]
 * @param sia the sender address
 * @param ga the group address
 * @param iid the installation id
 * @param st the service type ["r","w","rp"]
 * @param value_type the value type [0=boolean, 1=integer, 2=float]
 * @param value the value (as string)
 */
kisCS_EXPORT void ets_issue_requests_s_mode(int scope, int sia, int ga, int iid,
                                            char *st, int value_type,
                                            char *value);

/**
 * @brief discover KNX devices on the network
 *  e.g. issues a request with query param: rt=urn:knx:dpa.*
 *
 * @param scope the scope (2 or 5 site local)
 */
kisCS_EXPORT void ets_discover_devices(int scope);

/**
 * @brief discover KNX devices on the network
 * function can be used for discovery with serial number:
 * - ?ep=urn:knx:sn.[serial-number] :device with specific serial-number
 * - ?if=urn:knx:ia.[Individual Address] :device with specific Individual
 * Address
 * - ?if=urn:knx:if.pm  :devices in programming mode
 * - ?if=urn:knx:if.o : devices with specific interface (e.g. if.o)
 * - ?d=urn:knx:g.s.[ga] : devices belong to a specific group address
 *
 * @param scope the scope (2 or 5 site local)
 * @param query the query
 */
kisCS_EXPORT void ets_discover_devices_with_query(int scope, const char *query);

/**
 * @brief retrieve the amount of discovered devices
 *
 * @return number of discovered devices
 */
kisCS_EXPORT int ets_get_nr_devices(void);

/**
 * @brief retrieve the serial number of the device
 *
 * @param index the index in the list of discovered devices
 * @return serial number as string
 */
kisCS_EXPORT char *ets_get_sn(int index);

/**
 * @brief starts the C library.
 */
kisCS_EXPORT int ets_main(void);

/**
 * @brief stops the C library
 *
 * @param signal the signal to stop
 */
kisCS_EXPORT void ets_exit(int signal);

/** @} */ // end of doc_module_tag_dll

#ifdef __cplusplus
}
#endif

#endif /* OC_PYTHON_H */
