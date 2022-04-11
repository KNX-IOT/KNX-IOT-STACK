/*
// Copyright (c) 2021-2022 Cascoda Ltd
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
Group of exported functions, so that these functions can be used with a DLL or
shared object.

# request data
The request data is in CBOR, e.g. the input data to a request (POST/PUT) has to
be created in advance before calling the functions.

# response data
The response data is in text, e.g. converted from CBOR into JSON or in
link-format

# usage with python

The python file knx_python.py is the counter part that is using the dll/shared
object. The python code uses ets_start() ets_poll() and ets_stop() in an thread
from python.


@{
*/

/**
 * @brief The spake callback function type
 *
 * @param sn the serial number of the device
 * @param state The state of the device
 * @param secret the negotiated master secret
 * @param secret_size the negotiated master secret size
 * @param oscore_id the OSCORE id (from the input of the initiator)
 */
typedef void (*spakeCB)(char *sn, int state, char *secret, int secret_size,
                        char *oscore_id);

/**
 * @brief The changed callback function type
 *  NOT USED
 */
typedef void (*changedCB)(char *uuid, char *state, char *event);

/**
 * @brief The discovery callback function type
 *
 * @param payload_size the size of the payload in bytes
 * @param payload the ASCII payload e.g. link-formatted data
 */
typedef void (*discoveryCB)(int payload_size, char *payload);

/**
 * @brief the resource callback function type
 * NOT USED
 */
typedef void (*resourceCB)(char *anchor, char *uri, char *types,
                           char *interfaces);

/**
 * @brief The gateway callback function type
 *
 * @param sender_ip_address the sender address
 * @param payload_size the size of the payload in bytes
 * @param payload the ASCII payload as JSON
 */
typedef void (*gatewayCB)(char *sender_ip_address, int payload_size,
                          char *payload);

/**
 * @brief The client callback function
 * When one of the CoAP methods is called for interacting with an KNX device
 * this callback is being called with the expected results.
 *
 * @see ets_cbor_get
 * @see ets_cbor_get_unsecured
 * @see ets_linkformat_get
 * @see ets_linkformat_get_unsecured
 * @see ets_cbor_post
 * @see ets_cbor_put
 * @see ets_cbor_delete
 *
 * @param sn the serial number of the device
 * @param status The state of the device
 * @param r_format the return format
 * @param r_id the r_id (string), e.g. info that has been set on the call
 * @param url the called url
 * @param payload_size the size of the payload in bytes
 * @param payload the ASCII payload e.g. the core link formatted data
 */
typedef void (*clientCB)(char *sn, int status, char *r_format, char *r_id,
                         char *url, int payload_size, char *payload);

/**
 * @brief Returns the application max data size, e.g. data size for each call
 *
 * @return max data size
 */
kisCS_EXPORT long ets_get_max_app_data_size(void);

/**
 * @brief Sets the maximum data size, e.g. the data size for each call
 *
 * @param data_size the data size
 */
kisCS_EXPORT void ets_set_max_app_data_size(int data_size);

/**
 * @brief Install the changed callback
 *
 * @param changedCB the changed callback
 */
kisCS_EXPORT void ets_install_changedCB(changedCB changedCB);

/**
 * @brief Install the resource callback (not yet used)
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
 * @brief install the gateway callback
 * This function is called when an s-mode message is received.
 * e.g. on all received s-mode messages.
 * e.g. on all received s-mode messages.
 * @param gatewayCB the gateway callback
 */
kisCS_EXPORT void ets_install_gatewayCB(gatewayCB gatewayCB);

/**
 * @brief issue a GET request with expected content type CBOR
 *
 * @see clientCB
 * @param sn the serial number of the device
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string), e.g. info that will be returned by the
 * callback
 */
kisCS_EXPORT void ets_cbor_get(char *sn, char *uri, char *query, char *r_id);

/**
 * @brief issue a GET request with expected content type CBOR (unsecured)
 *
 * @see clientCB
 * @param sn the serial number of the device
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string), e.g. info that will be returned by the
 * callback
 */
kisCS_EXPORT void ets_cbor_get_unsecured(char *sn, char *uri, char *query,
                                         char *r_id);

/**
 * @brief issue a GET request with expected content type LINK-FORMAT
 *
 * @see clientCB
 * @param sn the serial number of the device
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string), e.g. info that will be returned by the
 * callback
 */
kisCS_EXPORT void ets_linkformat_get(char *sn, char *uri, char *query,
                                     char *r_id);

/*
 * @brief issue a GET request with expected content type LINK-FORMAT unsecured
 *
 * @see clientCB
 * @param sn the serial number of the device
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string), e.g. info that will be returned by the
 * callback
 */
kisCS_EXPORT void ets_linkformat_get_unsecured(char *sn, char *uri, char *query,
                                               char *r_id);

/**
 * @brief issue a POST request, content type CBOR
 *
 * @see clientCB
 * @param sn the serial number of the device
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string), e.g. info that will be returned by the
 * callback
 * @param size the size of the data
 * @param data the request data (in cbor)
 */
kisCS_EXPORT void ets_cbor_post(char *sn, char *uri, char *query, char *r_id,
                                int size, char *data);

/**
 * @brief issue a PUT request, content type CBOR
 *
 * @see clientCB
 * @param sn the serial number of the device
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string), e.g. info that will be returned by the
 * callback
 * @param size the size of the data
 * @param data the request data (in cbor)
 */
kisCS_EXPORT void ets_cbor_put(char *sn, char *uri, char *query, char *r_id,
                               int size, char *data);

/**
 * @brief issue a DELETE request, content type CBOR
 *
 * @see clientCB
 * @param sn the serial number of the device
 * @param uri the local path
 * @param query the query
 * @param r_id the r_id (string), e.g. info that will be returned by the
 * callback
 */
kisCS_EXPORT void ets_cbor_delete(char *sn, char *uri, char *query, char *r_id);

/**
 * @brief initiate the spake handshake
 * the result of the handshake will be returned via the spa
 *
 * @param sn the serial number of the device
 * @param password the password of the device
 * @param oscore_id the OSCORE id for the spake handshake
 */
kisCS_EXPORT void ets_initiate_spake(char *sn, char *password, char *oscore_id);

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
 * @brief configure the stack to listen to group addresses.
 * the group addresses for s-mode commands are defined per:
 * - scope (2 local, 5 site local)
 * - ga_max, starting from group address 1.
 * - iid, installation id
 *
 * @param scope the multicast scope
 * @param ga_max the group address maximum, e.g. all values between 1 and ga_max
 * will be registered
 * @param iid the installation identifier
 * @return kisCS_EXPORT
 */
kisCS_EXPORT void ets_listen_s_mode(int scope, int ga_max, int iid);

/**
 * @brief reset the this client
 * 
 * @return kisCS_EXPORT 
 */
kisCS_EXPORT void ets_reset_ets();

/**
 * @brief error to string
 * 
 * @param error_code the stack error code returned by a callback.
 * @return string : error code as a human readable string
 */
kisCS_EXPORT char *ets_error_to_string(int error_code);

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
 *
 *  This function starts the library in a thread.
 *  thread should be stopped with ets_exit()
 */
kisCS_EXPORT int ets_main(void);

/**
 * @brief stops the C library
 *
 * @see ets_main
 * @param signal the signal to stop
 */
kisCS_EXPORT void ets_exit(int signal);

/**
 * @brief start the ETS server
 * The initialization of the KNX server code
 * This function should by followed by a loop that calls ets_poll()
 * The clean up can be done when the polling is finished by calling ets_stop()
 * Example:
 * ```
 * ets_start("1234");
 * while (true) {
 *   ets_poll();
 * }
 * ets_stop();
 * ```
 *
 * @see ets_stop
 * @see ets_poll
 *
 * @param serial_number the serial number of the KNX device
 * @return 0 == success
 */
kisCS_EXPORT int ets_start(char *serial_number);

/**
 * @brief stop the ETS server
 *  e.g. free up the allocated resources when the ETS server has been started
 * with ets_start()
 * @see ets_start
 * @see ets_poll
 * @return 0 == success
 */
kisCS_EXPORT int ets_stop(void);

/**
 * @brief ets poll
 * e.g. the tick to do processing on the KNX stack
 * should be called at a regular pace, e.g. each milisecond.
 * @see ets_start
 * @see ets_stop
 * @return 0 == success
 */
kisCS_EXPORT int ets_poll(void);

/** @} */ // end of doc_module_tag_dll

#ifdef __cplusplus
}
#endif

#endif /* OC_PYTHON_H */
