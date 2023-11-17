/*
// Copyright (c) 2016-2019 Intel Corporation
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
  @brief Main API of the stack for client and server.
  @file
*/

/**
  \mainpage KNX IoT Point API Stack

  The file \link oc_api.h \endlink is the main entry for all
  server and client related stack functions.

  An KNX IOT Point API device contains:

  - initialization functionality
  - \ref doc_module_tag_server_side Server exposing data points
  - \ref doc_module_tag_client_state Client sending s-mode commands

  The Stack implements functionality like:

  - the CoAP client and server
  - OSCORE security
  - .well-known/core discovery
  - Table implementation:
    - Group object table
    - Credential table (e.g. auth/at entries)
    - Recipient table
    - functionality to handle the s-mode objects & transmission flags.


  Therefore an KNX IoT Point API application exist of:

  - Code for each specific data points (handling GET/POST)
  - own code to talk to hardware
  - Device specific (functional specific) callbacks
     - reset \ref oc_reset_t
     - restart \ref oc_restart_t
     - software update
     - setting host name  \ref oc_hostname_t
  - main loop

  Examples of functional devices :
  - lsab_minimal_all.c an example that implements Functional Block LSAB
  - lssb_minimal_all.c an example that implements Functional Block LSSB

  ## handling of transmission flags

  - Case 1 (write data):
    - Received from bus: -st w, any ga
    - receiver does: c flags = w -> overwrite object value
  - Case 2 (update data):
    - Received from bus: -st rp, any ga
    - receiver does: c flags = u -> overwrite object value
  - Case 3 (inform change):
    - sender: updated object value + cflags = t
    - Sent: -st w, sending association (1st assigned ga)
      Note: this will be done when Case 1 & Case 2 have updated a value.
  - Case 4 (request & respond):
    - sender: c flags = r
    - Received from bus: -st r
    - Sent: -st rp, sending association (1st assigned ga)
  - Case 5 (update at start up):
    - sender: c flags = i
    - After device restart (power up)
    - Sent: -st r, sending association (1st assigned ga)
*/

#ifndef OC_API_H
#define OC_API_H

#include "messaging/coap/oc_coap.h"
#include "oc_buffer_settings.h"
#include "oc_knx.h"
#include "oc_rep.h"
#include "oc_ri.h"
#include "oc_client_state.h"
#include "oc_signal_event_loop.h"
#include "port/oc_storage.h"
#include "api/oc_knx_client.h"
#include "api/oc_knx_swu.h"

#include "oc_programming_mode.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief maximum URL length (as specified by KNX)
 *
 */
#define OC_MAX_URL_LENGTH (30)

/**
 * Call back handlers that are invoked in response to oc_main_init()
 *
 * @see oc_main_init
 */
typedef struct
{
  /**
   * Device initialization callback that is invoked to initialize the platform
   * and device(s).
   *
   * At a minimum the platform should be initialized and at least one device
   * added.
   *
   *  - oc_init_platform()
   *  - oc_add_device()
   *
   * Multiple devices can be added by making multiple calls to oc_add_device().
   *
   * Other actions may be taken in the init handler
   *  - Set up an interrupt handler oc_activate_interrupt_handler()
   *  - Initialize application specific variables
   *
   * @return
   *  - 0 to indicate success initializing the application
   *  - value less than zero to indicate failure initializing the application
   *
   * @see oc_activate_interrupt_handler
   * @see oc_add_device
   * @see oc_init_platform
   */
  int (*init)(void);

  /**
   * Function to signal the event loop
   * so that incoming events are being processed.
   *
   * @see oc_main_poll
   */
  void (*signal_event_loop)(void);

#ifdef OC_SERVER
  /**
   * Resource registration callback.
   *
   * Callback is invoked after the device initialization callback.
   *
   * Use this callback to add resources to the devices added during the device
   * initialization.  This where the properties and callbacks associated with
   * the resources are typically done.
   *
   * Note: Callback is only invoked when OC_SERVER macro is defined.
   *
   * Example:
   * ```
   * static void register_resources(void)
   * {
   *   oc_resource_t *bswitch = oc_new_resource(NULL, "/switch", 1, 0);
   *   oc_resource_bind_resource_type(bswitch, "urn:knx:dpa.417.61");
   *   oc_resource_bind_dpt(bswitch, "urn:knx:dpt.switch");
   *   oc_resource_bind_resource_interface(bswitch, OC_IF_A);
   *   oc_resource_set_discoverable(bswitch, true);
   *   oc_resource_set_request_handler(bswitch, OC_GET, get_switch, NULL);
   *   oc_resource_set_request_handler(bswitch, OC_PUT, put_switch, NULL);
   *   oc_resource_set_request_handler(bswitch, OC_POST, post_switch, NULL);
   *   oc_add_resource(bswitch);
   * }
   * ```
   *
   * @see init
   * @see oc_new_resource
   * @see oc_resource_bind_resource_interface
   * @see oc_resource_bind_resource_type
   * @see oc_resource_bind_dpt
   * @see oc_resource_make_public
   * @see oc_resource_set_discoverable
   * @see oc_resource_set_observable
   * @see oc_resource_set_periodic_observable
   * @see oc_resource_set_request_handler
   * @see oc_add_resource
   */
  void (*register_resources)(void);
#endif /* OC_SERVER */

#ifdef OC_CLIENT
  /**
   * Callback invoked when the stack is ready to issue discovery requests.
   *
   * Callback is invoked after the device initialization callback.
   *
   * Example:
   * ```
   * static void issue_requests(void)
   * {
   *   oc_do_ip_discovery("dpa.321.51", &discovery, NULL);
   * }
   * ```
   *
   * @see init
   * @see oc_do_ip_discovery
   * @see oc_do_ip_discovery_at_endpoint
   * @see oc_do_site_local_ipv6_discovery
   * @see oc_do_realm_local_ipv6_discovery
   */
  void (*requests_entry)(void);
#endif /* OC_CLIENT */
} oc_handler_t;

/**
 * Callback invoked during oc_init_platform(). The purpose is to add any
 * additional platform properties that are not supplied to oc_init_platform()
 * function call.
 *
 * Example:
 * ```
 * static int app_init(void)
 * {
 *   int ret = oc_init_platform("My Platform",
 *      set_additional_platform_properties, NULL);
 *   ret |= oc_add_device("my_name", "1.0.0", "//", "000005", NULL, NULL);
 * }
 * ```
 *
 * @param[in] data context pointer that comes from the oc_add_device() function
 *
 * @see oc_add_device
 * @see oc_set_custom_device_property
 */
typedef void (*oc_init_platform_cb_t)(void *data);

/**
 * Callback invoked during oc_add_device(). The purpose is to add any additional
 * device properties that are not supplied to oc_add_device() function call.
 *
 * Example:
 * ```
 * static void set_device_custom_property(void *data)
 * {
 *   (void)data;
 *   oc_set_custom_device_property(purpose, "desk lamp");
 * }
 *
 * static int app_init(void)
 * {
 *   int ret = oc_init_platform("My Platform", NULL, NULL);
 *   ret |= oc_add_device("my_name", "1.0.0", "//", "000005", NULL, NULL);
 *   return ret;
 * }
 * ```
 *
 * @param[in] data context pointer that comes from the oc_init_platform()
 * function
 *
 * @see oc_add_device
 * @see oc_set_custom_device_property
 */
typedef void (*oc_add_device_cb_t)(void *data);

/**
 * Register and call handler functions responsible for controlling the
 * stack.
 *
 * This will initialize the stack.
 *
 * Before initializing the stack, a few setup functions may need to be called
 * before calling oc_main_init those functions are:
 *
 * - oc_set_con_res_announced()
 * - oc_set_factory_presets_cb()
 * - oc_set_max_app_data_size()
 * - oc_storage_config()
 *
 * Not all of the listed functions must be called before calling oc_main_init.
 *
 * @param[in] handler struct containing pointers callback handler functions
 *                    responsible for controlling the application
 * @return
 *  - `0` if stack has been initialized successfully
 *  - a negative number if there is an error in stack initialization
 *
 * @see oc_set_con_res_announced
 * @see oc_set_factory_presets_cb
 * @see oc_set_max_app_data_size
 * @see oc_storage_config
 */
int oc_main_init(const oc_handler_t *handler);

/**
 * poll to process tasks
 *
 * @return
 *  - time for the next poll event
 */
oc_clock_time_t oc_main_poll(void);

/**
 * Shutdown and free all stack related resources
 */
void oc_main_shutdown(void);

/**
 * Callback invoked by the stack initialization to perform any
 * "factory settings", e.g., this may be used to load a manufacturer
 * certificate.
 *
 *
 */
typedef void (*oc_factory_presets_cb_t)(size_t device, void *data);

/**
 * Set the factory presets callback.
 *
 * The factory presets callback is called by the stack to enable per-device
 * presets.
 *
 * @note oc_set_factory_presets_cb() must be called before oc_main_init().
 *
 * @param[in] cb oc_factory_presets_cb_t function pointer to be called
 * @param[in] data context pointer that is passed to the oc_factory_presets_cb_t
 *                 the pointer must be a valid pointer till after oc_main_init()
 *                 call completes.
 */
void oc_set_factory_presets_cb(oc_factory_presets_cb_t cb, void *data);

/**
 * Callback invoked by the stack initialization to perform any
 * application reset.
 *
 * @param[in] device the device index
 * @param[in] reset_value reset value per KNX
 * @param[in] data the user supplied data
 *
 */
typedef void (*oc_reset_cb_t)(size_t device, int reset_value, void *data);

/**
 * Set the reset callback.
 *
 * The reset callback is called by the stack to enable per-device
 * reset on application level.
 *
 * @note oc_set_reset_cb() must be called before oc_main_init().
 *
 * @param[in] cb oc_reset_cb_t function pointer to be called
 * @param[in] data context pointer that is passed to the oc_reset_cb_t
 *                 the pointer must be a valid pointer till after oc_main_init()
 *                 call completes.
 */
void oc_set_reset_cb(oc_reset_cb_t cb, void *data);

/**
 * Callback invoked by the stack to invoke a restart
 *
 * @param[in] device the device index
 * @param[in] data the user supplied data
 *
 */
typedef void (*oc_restart_cb_t)(size_t device, void *data);

/**
 * Set the restart callback.
 *
 * The restart callback is called by the stack to enable per-device
 * reset on application level.
 *
 * Implemented by the stack:
 *  - reset of the programming mode (e.g. turn it off)
 *
 * @note The restart function is not restarting the device.
 *
 * @note oc_set_restart_cb() must be called before oc_main_init().
 *
 * @param[in] cb oc_restart_cb_t function pointer to be called
 * @param[in] data context pointer that is passed to the oc_restart_cb_t
 *                 the pointer must be a valid pointer till after oc_main_init()
 *                 call completes.
 */
void oc_set_restart_cb(oc_restart_cb_t cb, void *data);

/**
 * Callback invoked by the stack to set the host name
 *
 * @param[in] device the device index
 * @param[in] host_name the host name to be set
 * @param[in] data the user supplied data
 *
 */
typedef void (*oc_hostname_cb_t)(size_t device, oc_string_t host_name,
                                 void *data);

/**
 * Set the host name callback.
 *
 * The host name callback is called by the stack when the host name needs to be
 * set
 *
 * @note oc_set_hostname_cb() must be called before oc_main_init().
 *
 * @param[in] cb oc_hostname_cb_t function pointer to be called
 * @param[in] data context pointer that is passed to the oc_restart_cb_t
 *                 the pointer must be a valid pointer till after oc_main_init()
 *                 call completes.
 */
void oc_set_hostname_cb(oc_hostname_cb_t cb, void *data);

/**
 * Set the programming mode callback
 * NOTE: It is the responsibility of this callback (if registered), to
 * set the programming mode of the device via a call to
 * oc_knx_device_set_programming_mode();
 *
 * @param[in] device the device index
 * @param[in] programming_mode whether to set the programming mode to true or
 * false
 * @param[in] data the user supplied data
 *
 */
typedef void (*oc_programming_mode_cb_t)(size_t device, bool programming_mode,
                                         void *data);

/**
 * Set the programming mode callback
 *
 * The programming mode callback is called by the stack to enable per-device
 * setting of the programming mode on application level.
 *
 * @note oc_set_programming_mode_cb() must be called before oc_main_init().
 *
 * @param[in] cb oc_programming_mode_cb_t function pointer to be called
 * @param[in] data context pointer that is passed to the
 * oc_programming_mode_cb_t the pointer must be a valid pointer till after
 * oc_main_init() call completes.
 */
void oc_set_programming_mode_cb(oc_programming_mode_cb_t cb, void *data);

/**
 * Add an a device to the stack.
 *
 * This function is typically called as part of the stack initialization
 * process from inside the `init` callback handler.
 *
 * The `oc_add_device` function may be called as many times as needed.
 * Each call will add a new device to the stack with its own port address.
 * Each device is automatically assigned a number starting with zero and
 * incremented by one each time the function is called. This number is not
 * returned therefore it is important to know the order devices are added.
 *
 * Example:
 * ```
 * //app_init is an instance of the `init` callback handler.
 * static int app_init(void)
 * {
 *   int ret = oc_init_platform("Refrigerator", NULL, NULL);
 *   ret |= oc_add_device("my_name", "1.0", "//",
 *                        "0123456", NULL, NULL);

 *   return ret;
 * }
 * ```
 *
 * @param[in] name the user readable name of the device
 * @param[in] version The api version e.g. "1.0.0"
 * @param[in] base the base url e.g. "/"
 * @param[in] serial_number the serial number of the device
 * @param[in] add_device_cb callback function invoked during oc_add_device().
 * The purpose is to add additional device properties that are not supplied to
 * oc_add_device() function call.
 * @param[in] data context pointer that is passed to the oc_add_device_cb_t
 *
 * @return
 *   - `0` on success
 *   - `-1` on failure
 *
 * @see init
 */
int oc_add_device(const char *name, const char *version, const char *base,
                  const char *serial_number, oc_add_device_cb_t add_device_cb,
                  void *data);

/**
 * Set custom device property
 *
 * The purpose is to add additional device properties that are not supplied to
 * oc_add_device() function call. This function will likely only be used inside
 * the oc_add_device_cb_t().
 *
 * @param[in] prop the name of the custom property being added to the device
 * @param[in] value the value of the custom property being added to the device
 *
 * @see oc_add_device_cb_t for example code using this function
 * @see oc_add_device
 */
#define oc_set_custom_device_property(prop, value)                             \
  oc_rep_set_text_string(root, prop, value)

/**
 * Initialize the platform.
 *
 * This function is typically called as part of the stack initialization
 * process from inside the `init` callback handler.
 *
 * @param[in] mfg_name the name of the platform manufacture
 * @param[in] init_platform_cb callback function invoked during
 *                             oc_init_platform(). The purpose is to add
 *                             additional device properties that are not
 *                             supplied to oc_init_platform() function call.
 * @param[in] data context pointer that is passed to the oc_init_platform_cb_t
 *
 * @return
 *   - `0` on success
 *   - `-1` on failure
 *
 * @see init
 * @see oc_init_platform_cb_t
 */
int oc_init_platform(const char *mfg_name,
                     oc_init_platform_cb_t init_platform_cb, void *data);

/**
 * Set custom platform property.
 *
 * The purpose is to add additional platform properties that are not supplied to
 * oc_init_platform() function call. This function will likely only be used
 * inside the oc_init_platform_cb_t().
 *
 * @param[in] prop the name of the custom property being added to the platform
 * @param[in] value the value of the custom property being added to the platform
 *
 * @see oc_init_platform_cb_t for example code using this function
 * @see oc_init_platform
 */
#define oc_set_custom_platform_property(prop, value)                           \
  oc_rep_set_text_string(root, prop, value)

/* Server side */
/**
  @defgroup doc_module_tag_server_side Server side
  Group of server support functions.

  # standardized data points

  The standardized functions are implemented.
  The following groups are implemented:
  - / dev / x
  - / .well-known / core
  - / fp / x
  - / fb / x

  all functions generate the core-link or CBOR formatted responses.

  # application specific data points

  Applications have to define the functions (GET and PUT) for the application
  level data points. Applications have to define for each instance these
  functions. The functions are registered with the device and will be called
  when the other devices are interacting with it.

  see for more details the examples.

  @{
*/
/**
 * Allocate and populate a new oc_resource_t.
 *
 * Resources are the primary interface between code and real world devices.
 *
 * Each resource has a Uniform Resource Identifier (URI) that identifies it.
 * All resources **must** specify one or more Resource Types to be considered a
 * valid resource. The number of Resource Types is specified by the
 * `num_resource_types` the actual Resource Types are added later using the
 * oc_resource_bind_resource_type() function.
 *
 * Many properties associated with a resource are set or modified after the
 * new resource has been created.
 *
 * The resource is not added to the device till oc_add_resource() is called.
 *
 * Example:
 * ```
 * static void register_resources(void)
 * {
 *   oc_resource_t *bswitch = oc_new_resource("light switch", "/switch", 1, 0);
 *   oc_resource_bind_resource_type(bswitch, "urn:knx:dpa.417.61");
 *   oc_resource_bind_dpt(bswitch, "urn:knx:dpt.switch");
 *   oc_resource_bind_resource_interface(bswitch, OC_IF_A);
 *   oc_resource_set_observable(bswitch, true);
 *   oc_resource_set_discoverable(bswitch, true);
 *   oc_resource_set_request_handler(bswitch, OC_GET, get_switch, NULL);
 *   oc_resource_set_request_handler(bswitch, OC_POST, post_switch, NULL);
 *   oc_resource_set_request_handler(bswitch, OC_PUT, put_switch, NULL);
 *   oc_add_resource(bswitch);
 * }
 * ```
 *
 * @param[in] name the name of the new resource this will set the property `n`
 * @param[in] uri the Uniform Resource Identifier for the resource
 * @param[in] num_resource_types the number of Resource Types that will be
 *                               added/bound to the resource
 * @param[in] device index of the logical device the resource will be added to
 *
 * @see oc_resource_bind_resource_interface
 * @see oc_resource_bind_resource_type
 * @see oc_resource_bind_dpt
 * @see oc_process_baseline_interface
 * @see oc_resource_set_discoverable
 * @see oc_resource_set_periodic_observable
 * @see oc_resource_set_request_handler
 */
oc_resource_t *oc_new_resource(const char *name, const char *uri,
                               uint8_t num_resource_types, size_t device);

/**
 * Add the supported interface(s) to the resource.
 *
 * Resource interfaces specify how the code is able to interact with the
 * resource
 *
 * The `iface_mask` is bitwise OR of the interfaces
 *
 * @param[in] resource the resource that the interface(s) will be added to
 * @param[in] iface_mask a bitwise ORed list of all interfaces supported by the
 *                       resource.
 * @see oc_interface_mask_t
 */
void oc_resource_bind_resource_interface(oc_resource_t *resource,
                                         oc_interface_mask_t iface_mask);

/**
 * Add a Resource Type "rt" property to the resource.
 *
 * All resources require at least one Resource Type. The number of Resource
 * Types the resource contains is declared when the resource it created using
 * oc_new_resource() function.
 *
 * Multi-value "rt" Resource means a resource with multiple Resource Types. i.e.
 * oc_resource_bind_resource_type() is called multiple times for a single
 * resource. When using a Multi-value Resource the different resources
 * properties must not conflict.
 *
 * @param[in] resource the resource that the Resource Type will be set on
 * @param[in] type the Resource Type to add to the Resource Type "rt" property
 *
 * @see oc_new_resource
 * @see oc_device_bind_resource_type
 */
void oc_resource_bind_resource_type(oc_resource_t *resource, const char *type);

/**
 * @brief set the content type on the resource
 *
 * @param resource the resource
 * @param content_type the content type
 */
void oc_resource_bind_content_type(oc_resource_t *resource,
                                   oc_content_format_t content_type);

/**
 * Add a Data Point Type "dpt" property to the resource.
 *
 * @param[in] resource the resource that the Data Point Type will be set on
 * @param[in] dpt the Data Point Type to add to the Data Point Type "dpt"
 * property
 *
 * @see oc_new_resource
 * @see oc_device_bind_resource_type
 */
void oc_resource_bind_dpt(oc_resource_t *resource, const char *dpt);

/**
 * Expose unsecured coap:// endpoints (in addition to secured coaps://
 * endpoints) for this resource in /well-known/core
 *
 * @note While the resource may advertise unsecured endpoints, the resource
 *       shall remain inaccessible until the hosting device is configured with
 *       an anon-clear Access Control Entry (ACE).
 *
 * @param[in] resource the resource to make public
 *
 * @see oc_new_resource
 */
void oc_resource_make_public(oc_resource_t *resource);

/**
 * Specify if a resource can be found using .well-known/core discover
 * mechanisms.
 *
 * @param[in] resource to specify as discoverable or non-discoverable
 * @param[in] state if true the resource will be discoverable if false the
 *                  resource will be non-discoverable
 *
 * @see oc_new_resource for example code using this function
 */
void oc_resource_set_discoverable(oc_resource_t *resource, bool state);

/**
 * Specify that a resource should notify clients when a property has been
 * modified.
 *
 * @note this function can be used to make a periodic observable resource
 *       unobservable.
 *
 * @param[in] resource the resource to specify the observability
 * @param[in] state true to make resource observable, false to make resource
 *                  unobservable
 *
 * @see oc_new_resource to see example code using this function
 * @see oc_resource_set_periodic_observable
 */
void oc_resource_set_observable(oc_resource_t *resource, bool state);

/**
 * The resource will periodically notify observing clients of is property
 * values.
 *
 * The oc_resource_set_observable() function can be used to turn off a periodic
 * observable resource.
 *
 * Setting a `seconds` frequency of zero `0` is invalid and will result in an
 * invalid resource.
 *
 * @param[in] resource the resource to specify the periodic observability
 * @param[in] seconds the frequency in seconds that the resource will send out
 *                    an notification of is property values.
 */
void oc_resource_set_periodic_observable(oc_resource_t *resource,
                                         uint16_t seconds);

/**
 * Specify a request_callback for GET, PUT, POST, and DELETE methods
 *
 * All resources must provide at least one request handler to be a valid
 * resource.
 *
 * method types:
 * - `OC_GET` the `oc_request_callback_t` is responsible for returning the
 * current value of all of the resource properties.
 * - `OC_PUT` the `oc_request_callback_t` is responsible for updating one or
 * more of the resource properties.
 * - `OC_POST` the `oc_request_callback_t` is responsible for updating one or
 * more of the resource properties. The callback may also be responsible for
 *         creating new resources.
 * - `OC_DELETE` the `oc_request_callback_t` is responsible for deleting a
 * resource
 *
 * @note Some methods may never by invoked based on the resources Interface as
 *       well as the provisioning permissions of the client.
 *
 * @param[in] resource the resource the callback handler will be registered to
 * @param[in] method specify if type method the callback is responsible for
 *                   handling
 * @param[in] callback the callback handler that will be invoked when a the
 *                     method is called on the resource.
 * @param[in] user_data context pointer that is passed to the
 *                      oc_request_callback_t. The pointer must remain valid as
 *                      long as the resource exists.
 *
 * @see oc_new_resource to see example code using this function
 */
void oc_resource_set_request_handler(oc_resource_t *resource,
                                     oc_method_t method,
                                     oc_request_callback_t callback,
                                     void *user_data);

/**
 * @brief sets the callback properties for set properties and get properties
 *
 * @param resource the resource for the callback data
 * @param get_properties callback function for retrieving the properties
 * @param get_props_user_data the user data for the get_properties callback
 * function
 * @param set_properties callback function for setting the properties
 * @param set_props_user_data the user data for the set_properties callback
 * function
 */
void oc_resource_set_properties_cbs(oc_resource_t *resource,
                                    oc_get_properties_cb_t get_properties,
                                    void *get_props_user_data,
                                    oc_set_properties_cb_t set_properties,
                                    void *set_props_user_data);

/**
 * @brief set a resource to a specific function block instance
 * default is instance 0, if there is just 1 instance of the
 * function block this function does not have to be called.
 *
 * @param resource the resource
 * @param instance the instance id.
 */
void oc_resource_set_function_block_instance(oc_resource_t *resource,
                                             uint8_t instance);

/**
 * Add a resource to the stack.
 *
 * The resource will be validated then added to the stack.
 *
 * @param[in] resource the resource to add to the stack
 *
 * @return
 *  - true: the resource was successfully added to the stack.
 *  - false: the resource can not be added to the stack.
 */
bool oc_add_resource(oc_resource_t *resource);

/**
 * Remove a resource from the stack and delete the resource.
 *
 * Any resource observers will automatically be removed.
 *
 * This will free the memory associated with the resource.
 *
 * @param[in] resource the resource to delete
 *
 * @return
 *  - true: when the resource has been deleted and memory freed.
 *  - false: there was an issue deleting the resource.
 */
bool oc_delete_resource(oc_resource_t *resource);

/**
 * Schedule a callback to remove a resource.
 *
 * @param[in] resource the resource to delete
 */
void oc_delayed_delete_resource(oc_resource_t *resource);

/**
 * This resets the query iterator to the start of the URI query parameter
 *
 * This is used together with oc_iterate_query_get_values() or
 * oc_iterate_query() to iterate through query parameter of a URI that are part
 * of an `oc_request_t`
 */
void oc_init_query_iterator(void);

/**
 * Iterate through the URI query parameters and get each key=value pair
 *
 * Before calling oc_iterate_query() the first time oc_init_query_iterator()
 * must be called to reset the query iterator to the first query parameter.
 *
 * @note the char pointers returned are pointing to the string location in the
 *       query string.  Do not rely on a null terminator to find the end of the
 *       string since there may be additional query parameters.
 *
 * Example:
 * ```
 * char *value = NULL;
 * int value_len = -1;
 * char *key
 * oc_init_query_iterator();
 * while (oc_iterate_query(request, &key, &key_len, &value, &value_len) > 0) {
 *   printf("%.*s = %.*s\n", key_len, key, query_value_len, query_value);
 * }
 * ```
 *
 * @param[in] request the oc_request_t that contains the query parameters
 * @param[out] key pointer to the location of the key of the key=value pair
 * @param[out] key_len the length of the key string
 * @param[out] value pointer the location of the value string assigned to the
 *             key=value pair
 * @param[out] value_len the length of the value string
 *
 * @return
 *   - The position in the query string of the next key=value string pair
 *   - `-1` if there are no additional query parameters
 */
int oc_iterate_query(oc_request_t *request, char **key, size_t *key_len,
                     char **value, size_t *value_len);

/**
 * Iterate though the URI query parameters for a specific key.
 *
 * Before calling oc_iterate_query_get_values() the first time
 * oc_init_query_iterator() must be called to reset the query iterator to the
 * first query parameter.
 *
 * @note The char pointer returned is pointing to the string location in the
 *       query string. Do not rely on a null terminator to find the end of the
 *       string since there may be additional query parameters.
 *
 * Example:
 * ```
 * bool more_query_params = false;
 * const char* expected_value = "world"
 * char *value = NULL;
 * int value_len = -1;
 * oc_init_query_iterator();
 * do {
 * more_query_params = oc_iterate_query_get_values(request, "hello",
 *                                                 &value, &value_len);
 *   if (rt_len > 0) {
 *     printf("Found %s = %.*s\n", "hello", value_len, value);
 *   }
 * } while (more_query_params);
 * ```
 *
 * @param[in] request the oc_request_t that contains the query parameters
 * @param[in] key the key being searched for
 * @param[out] value pointer to the value string for to the key=value pair
 * @param[out] value_len the length of the value string
 *
 * @return True if there are more query parameters to iterate through
 */
bool oc_iterate_query_get_values(oc_request_t *request, const char *key,
                                 char **value, int *value_len);

/**
 * Get a pointer to the start of the value in a URL query parameter key=value
 * pair.
 *
 * @note The char pointer returned is pointing to the string location in the
 *       query string. Do not rely on a null terminator to find the end of the
 *       string since there may be additional query parameters.
 *
 * @param[in] request the oc_request_t that contains the query parameters
 * @param[in] key the key being searched for
 * @param[out] value pointer to the value string assigned to the key
 *
 * @return
 *   - The position in the query string of the next key=value string pair
 *   - `-1` if there are no additional query parameters
 */
int oc_get_query_value(oc_request_t *request, const char *key, char **value);

/**
 * Checks if a query parameter 'key' exist in the URL query parameter
 *
 * @param[in] request the oc_request_t that contains the query parameters
 * @param[in] key the key being searched for
 *
 * @return
 *   - 1 exist
 *   - -1 does not exist
 */
int oc_query_value_exists(oc_request_t *request, const char *key);

/**
 * Checks if a query parameter are available
 *
 * @param[in] request the oc_request_t that contains the query parameters
 *
 * @return
 *  - False no queries available
 *  - True queries available
 */
bool oc_query_values_available(oc_request_t *request);

/**
 * Called after the response to a GET, PUT, POST or DELETE call has been
 * prepared completed, will respond with CBOR.
 *
 * The function oc_send_response is called at the end of a
 * oc_request_callback_t to inform the caller about the status of the requested
 * action.
 *
 *
 * @param[in] request the request being responded to
 * @param[in] response_code the status of the response
 *
 * @see oc_request_callback_t
 * @see oc_ignore_request
 * @see oc_indicate_separate_response
 */
void oc_send_response(oc_request_t *request, oc_status_t response_code);

/**
 * @brief Called after the response to a GET, PUT, POST or DELETE call has been
 * prepared completed. will respond with CBOR as content type.
 *
 * The function oc_send_response is called at the end of a
 * oc_request_callback_t to inform the caller about the status of the requested
 * action.
 *
 * Note that OC_STATUS_BAD_REQUEST for multicast will not send a response (e.g.
 * threated as OC_IGNORE)
 *
 * @param request the request being responded to
 * @param response_code the status of the response
 */
void oc_send_cbor_response(oc_request_t *request, oc_status_t response_code);

/**
 * @brief Called after the response to a GET, PUT, POST or DELETE call has been
 * prepared completed. will respond with CBOR.
 *
 * The function oc_send_response is called at the end of a
 * oc_request_callback_t to inform the caller about the status of the requested
 * action.
 *
 * Note that OC_STATUS_BAD_REQUEST for multicast will not send a response (e.g.
 * threated as OC_IGNORE)
 *
 * @param request the request being responded to
 * @param response_code the status of the response
 * @param payload_size the payload size of the response
 */
void oc_send_cbor_response_with_payload_size(oc_request_t *request,
                                             oc_status_t response_code,
                                             size_t payload_size);

/**
 * @brief Called after the response to a GET, PUT, POST or DELETE call has been
 * prepared completed. will respond with JSON.
 *
 * The function oc_send_response is called at the end of a
 * oc_request_callback_t to inform the caller about the status of the requested
 * action.
 *
 * Note that OC_STATUS_BAD_REQUEST for multicast will not send a response (e.g.
 * threated as OC_IGNORE)
 *
 * @param request the request being responded to
 * @param response_code the request being responded to
 */
void oc_send_json_response(oc_request_t *request, oc_status_t response_code);

/**
 * @brief Called after the response to a GET, PUT, POST or DELETE call has been
 * prepared completed. will respond with LINK-FORMAT.
 *
 * Note that OC_STATUS_BAD_REQUEST for multicast will not send a response (e.g.
 * threated as OC_IGNORE)
 *
 * @param request the request being responded to
 * @param response_code the request being responded to
 * @param response_length the framed response length
 */
void oc_send_linkformat_response(oc_request_t *request,
                                 oc_status_t response_code,
                                 size_t response_length);

/**
 * @brief Called after the response to a GET, PUT, POST or DELETE call has been
 * prepared completed. will respond without setting the content format.
 *
 * Example usecase: When the response has empty payload.
 *
 * @param request the request being responded to
 * @param response_code the request being responded to
 */
void oc_send_response_no_format(oc_request_t *request,
                                oc_status_t response_code);

/**
 * @brief retrieve the payload from the request, no processing
 *
 * @param request the request
 * @param payload the payload of the request
 * @param size the size in bytes of the payload
 * @param content_format the content format of the payload
 * @return true
 * @return false
 */
bool oc_get_request_payload_raw(oc_request_t *request, const uint8_t **payload,
                                size_t *size,
                                oc_content_format_t *content_format);

/**
 * @brief send the request, no processing
 *
 * @param request the request to send
 * @param payload the payload for the request
 * @param size the payload size
 * @param content_format the content format
 * @param response_code the response code to send
 */
void oc_send_response_raw(oc_request_t *request, const uint8_t *payload,
                          size_t size, oc_content_format_t content_format,
                          oc_status_t response_code);

/**
 * @brief retrieve the response payload, without processing
 *
 * @param response the response
 * @param payload the payload of the response
 * @param size the size of the payload
 * @param content_format the content format of the payload
 * @return true - retrieved payload
 * @return false
 */
bool oc_get_response_payload_raw(oc_client_response_t *response,
                                 const uint8_t **payload, size_t *size,
                                 oc_content_format_t *content_format);

/**
 * @brief send a diagnostic payload
 *
 * @param request the request
 * @param msg the message in ASCII
 * @param msg_len the length of the message
 * @param response_code the CoAP response code
 */
void oc_send_diagnostic_message(oc_request_t *request, const char *msg,
                                size_t msg_len, oc_status_t response_code);

/**
 * @brief retrieve the diagnostic payload from a response
 *
 * @param response the response to get the diagnostic payload from
 * @param msg the diagnostic payload
 * @param size the size of the diagnostic payload
 * @return true - retrieved payload
 * @return false
 */
bool oc_get_diagnostic_message(oc_client_response_t *response, const char **msg,
                               size_t *size);

/**
 * Ignore the request
 *
 * The GET, PUT, POST or DELETE requests can be ignored. For example a
 * oc_request_callback_t may only want to respond to multi-cast requests. Thus
 * any request that is not over multi-cast endpoint could be ignored.
 *
 * Using `oc_ignore(request)` is preferred over
 * `oc_send_response(request, OC_IGNORE)` since it does not attempt to fill the
 * response buffer before sending the response.
 *
 * @param[in] request the request being responded to
 *
 * @see oc_request_callback_t
 * @see oc_send_response
 */
void oc_ignore_request(oc_request_t *request);

/**
 * Respond to an incoming request asynchronously.
 *
 * If for some reason the response to a request would take a
 * long time or is not immediately available, then this function may be used
 * defer responding to the request.
 *
 * Example:
 * ```
 * static oc_separate_response_t sep_response;
 *
 * static oc_event_callback_retval_t
 * handle_separate_response(void *data)
 * {
 * if (sep_response.active) {
 *   oc_set_separate_response_buffer(&sep_response);
 *   printf("Handle separate response for GET handler:\n");
 *   oc_rep_begin_root_object();
 *   oc_rep_set_boolean(root, value, true);
 *   oc_rep_set_int(root, dimmingSetting, 75);
 *   oc_rep_end_root_object();
 *   oc_send_separate_response(&sep_response, OC_STATUS_OK);
 * }
 * return OC_EVENT_DONE;
 * }
 *
 * static void
 * get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
 *             void *user_data)
 * {
 *   printf("GET handler:\n");
 *   oc_indicate_separate_response(request, &sep_response);
 *   oc_set_delayed_callback(NULL, &handle_separate_response, 10);
 * }
 * ```
 * @param[in] request the request that will be responded to as a separate
 *                    response
 * @param[in] response instance of an internal struct that is used to track the
 *                     state of the separate response.
 *
 * @see oc_set_separate_response_buffer
 * @see oc_send_separate_response
 */
void oc_indicate_separate_response(oc_request_t *request,
                                   oc_separate_response_t *response);

/**
 * Set a response buffer for holding the response payload.
 *
 * When a deferred response is ready, pass in the same `oc_separate_response_t`
 * that was handed to oc_indicate_separate_response() for delaying the
 * initial response.
 *
 * @param[in] handle instance of the oc_separate_response_t that was passed to
 *                   the oc_indicate_separate_response() function
 *
 * @see oc_indicate_separate_response
 * @see oc_send_separate_response
 */
void oc_set_separate_response_buffer(oc_separate_response_t *handle);

/**
 * Called to send the deferred response to a GET, PUT, POST or DELETE request.
 *
 * The function oc_send_separate_response is called to initiate transfer of the
 * response.
 *
 * @param[in] handle instance of the internal struct that was passed to
                     oc_indicate_separate_response()
 * @param[in] response_code the status of the response
 *
 * @see oc_indicate_separate_response
 * @see oc_send_separate_response
 * @see oc_send_response
 * @see oc_ignore_request
 */
void oc_send_separate_response(oc_separate_response_t *handle,
                               oc_status_t response_code);

/**
 * Called to send the deferred response to a GET, PUT, POST or DELETE request,
 * with an empty payload.
 *
 * The function oc_send_empty_separate_response is called to initiate transfer
 of the
 * response.
 *
 * @param[in] handle instance of the internal struct that was passed to
                     oc_indicate_separate_response()
 * @param[in] response_code the status of the response
 *
 * @see oc_indicate_separate_response
 * @see oc_send_separate_response
 * @see oc_send_response
 * @see oc_ignore_request
 */
void oc_send_empty_separate_response(oc_separate_response_t *handle,
                                     oc_status_t response_code);

/**
 * Notify all observers of a change to a given resource's property
 *
 * @note no need to call oc_notify_observers about resource changes that
 *       result from a PUT, or POST oc_request_callback_t.
 *
 * @param[in] resource the oc_resource_t that has a modified property
 *
 * @return
 *  - the number observers notified on success
 *  - `0` on failure could also mean no registered observers
 */
int oc_notify_observers(oc_resource_t *resource);

#ifdef __cplusplus
}
#endif
/** @} */ // end of doc_module_tag_server_side

/**
  @defgroup doc_module_tag_client_state Client side
  Client side support functions.

  This module contains functions to communicate to a KNX server for an Client.

  ## multicast

  The multicast communication is for:
  - Discovery

  The multicast Discovery is issued is on CoAP .well-known/core
  The s-mode communication is performed at the (specific) group addresses.


  ## unicast communication

  The following functions can be used to communicate on CoAP level e.g. issuing:
  - GET
  - PUT
  - POST
  - DELETE
  functions.
  The functions are secured with OSCORE.

  @{
*/
#include "oc_client_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Discover all servers that have a resource type using the site-local scope
 *
 * The discovery request will make a multi-cast request to the IPv6 ``scope``
 * multi-cast address scope. The address scope is the domain in which the
 * multi-cast discovery packet should be propagated.
 *
 * Read RFC4291 and RFC7346 for more information about IPv6 Reference Scopes.
 *
 * @param[in] uri_query the query to be added the .well-known/core URI.
 * @param[in] scope  the scope of the request, for example: 0x2
 * @param[in] handler the oc_discovery_all_handler_t that will be called once a
 *                    server containing the resource type is discovered
 * @param[in] user_data context pointer that is passed to the
 *                      oc_discovery_handler_t.
 *
 * @return true on success
 */
bool oc_do_wk_discovery_all(const char *uri_query, int scope,
                            oc_discovery_all_handler_t handler,
                            void *user_data);

/**
 * @brief link format parser, retrieve the number of entries in a response
 *
 * @param payload The link-format response
 * @param payload_len The length of the response
 * @return int amount of entries
 */
int oc_lf_number_of_entries(const char *payload, int payload_len);

/**
 * @brief link format parser, retrieve the URL of an entry.
 *
 * @param payload The link-format response
 * @param payload_len The length of the response
 * @param entry The index of entries, starting with 0.
 * @param uri The pointer to store the URI
 * @param uri_len The length of the URI
 * @return int 1 success full
 */
int oc_lf_get_entry_uri(const char *payload, int payload_len, int entry,
                        const char **uri, int *uri_len);

/**
 * @brief link format parser, retrieve a parameter value
 *
 * @param payload The link-format response
 * @param payload_len The length of the response
 * @param entry The index of entries, starting with 0.
 * @param param The query parameter, e.g. "rt"
 * @param p_out The pointer to store the value, e.g. "blah"
 * @param p_len The length of the URI
 * @return int 1 success full
 */
int oc_lf_get_entry_param(const char *payload, int payload_len, int entry,
                          const char *param, const char **p_out, int *p_len);

/**
 * @brief issues a get request with accept-content CBOR
 *
 * @param uri the uri to be used
 * @param endpoint the endpoint of the device
 * @param query the query
 * @param handler the callback handler
 * @param qos the qos type confirmable / not confirmable
 * @param user_data the user data
 * @return true
 * @return false
 *
 * @see oc_do_get_ex
 */
bool oc_do_get(const char *uri, oc_endpoint_t *endpoint, const char *query,
               oc_response_handler_t handler, oc_qos_t qos, void *user_data);

/**
 * @brief Issue a GET request to obtain the current value of all properties a
 * resource
 *
 * Example:
 * ```
 * static bool value;
 *
 * static void
 * get_light(oc_client_response_t *data)
 * {
 *   PRINT("GET_light:\n");
 *   oc_rep_t *rep = data->payload;
 *   while (rep != NULL) {
 *     PRINT("key %s, value ", oc_string_checked(rep->name));
 *     switch (rep->type) {
 *     case OC_REP_BOOL:
 *       PRINT("%d\n", rep->value.boolean);
 *       value = rep->value.boolean;
 *       break;
 *     default:
 *       break;
 *     }
 *     rep = rep->next;
 *   }
 * }
 * //the server uri and server endpoint obtained from oc_discovery_handler_t
 * // as a result of an oc_do_ip_discovery call
 * oc_do_get_ex(server_uri, server_ep, NULL, &get_switch, LOW_QOS,
 *   APPLICATION_CBOR, APPLICATION_CBOR, NULL);
 * ```
 *
 * @param[in] uri the uri of the resource
 * @param[in] endpoint the endpoint of the server
 * @param[in] query a query parameter that will be sent to the server's
 *                  oc_request_callback_t.
 * @param[in] handler function invoked once the client has received the servers
 *                    response to the GET request
 * @param[in] qos the quality of service current options are HIGH_QOS or LOW_QOS
 * @param[in] content The content format of the request payload
 * @param[in] accept  The content format of the response payload
 * @param[in] user_data context pointer that will be sent to the
 *                      oc_response_handler_t
 *
 * @return True if the client successfully dispatched the CoAP GET request
 */
bool oc_do_get_ex(const char *uri, oc_endpoint_t *endpoint, const char *query,
                  oc_response_handler_t handler, oc_qos_t qos,
                  oc_content_format_t content, oc_content_format_t accept,
                  void *user_data);

/**
 * Issue a DELETE request to delete a resource
 *
 * @param[in] uri The uri of the resource
 * @param[in] endpoint The endpoint of the server
 * @param[in] query a query parameter that will be sent to the server's
 *                  oc_request_callback_t.
 * @param[in] handler The function invoked once the client has received the
 * servers response to the DELETE request
 * @param[in] qos The quality of service current options are HIGH_QOS or LOW_QOS
 * @param[in] user_data The context pointer that will be sent to the
 *                      oc_response_handler_t
 *
 * @return True if the client successfully dispatched the CoAP DELETE request
 */
bool oc_do_delete(const char *uri, oc_endpoint_t *endpoint, const char *query,
                  oc_response_handler_t handler, oc_qos_t qos, void *user_data);

/**
 * Prepare the stack to issue a PUT request
 *
 * After oc_init_put has been called a CoAP message can be built using
 * `oc_rep_*` functions. Then oc_do_put is called to dispatch the CoAP request.
 *
 * Example:
 * ```
 *
 * static void
 * put_switch(oc_client_response_t *data)
 * {
 *   if (data->code == OC_STATUS_CHANGED)
 *     printf("PUT response: CHANGED\n");
 *   else
 *     printf("PUT response code %d\n", data->code);
 * }
 *
 * if (oc_init_put(server_uri, server_ep, NULL, &put_switch, LOW_QOS, NULL)) {
 *   oc_rep_begin_root_object();
 *   oc_rep_set_boolean(root, value, true);
 *   oc_rep_end_root_object();
 *   if (oc_do_put(APPLICATION_CBOR, APPLICATION_CBOR))
 *     printf("Sent PUT request\n");
 *   else
 *     printf("Could not send PUT request\n");
 * } else
 *   printf("Could not init PUT request\n");
 * ```
 * @param[in] uri the uri of the resource
 * @param[in] endpoint the endpoint of the server
 * @param[in] query a query parameter that will be sent to the server's
 *                  oc_request_callback_t.
 * @param[in] handler function invoked once the client has received the servers
 *                    response to the PUT request
 * @param[in] qos the quality of service current options are HIGH_QOS or LOW_QOS
 * @param[in] user_data context pointer that will be sent to the
 *                      oc_response_handler_t
 *
 * @return True if the client successfully prepared the CoAP PUT request
 *
 * @see oc_do_put_ex
 * @see oc_init_post
 */
bool oc_init_put(const char *uri, oc_endpoint_t *endpoint, const char *query,
                 oc_response_handler_t handler, oc_qos_t qos, void *user_data);

/**
 * @brief Dispatch the CoAP PUT request wiht content type and accept type CBOR
 *
 * @return true
 * @return false
 *
 * @see oc_do_put_ex
 */
bool oc_do_put(void);

/**
 * @brief Dispatch the CoAP PUT request
 *
 * @param content The content format of the request payload
 * @param accept  The content format of the response payload
 * @return true if the client successfully dispatched the CoAP request
 * @return false
 *
 * @see oc_init_put
 */
bool oc_do_put_ex(oc_content_format_t content, oc_content_format_t accept);

/**
 * Prepare the stack to issue a POST request
 *
 * After oc_init_post has been called a CoAP message can be built using
 * `oc_rep_*` functions. Then oc_do_post is called to dispatch the CoAP request.
 *
 * Example:
 * ```
 *
 * static void
 * post_switch(oc_client_response_t *data)
 * {
 *   if (data->code == OC_STATUS_CHANGED)
 *     printf("POST response: CHANGED\n");
 *   else
 *     printf("POST response code %d\n", data->code);
 * }
 *
 * if (oc_init_post(server_uri, server_ep, NULL, &put_switch, LOW_QOS, NULL)) {
 *   oc_rep_begin_root_object();
 *   oc_rep_set_boolean(root, value, true);
 *   oc_rep_end_root_object();
 *   if (oc_do_put(APPLICATION_CBOR, APPLICATION_CBOR))
 *     printf("Sent POST request\n");
 *   else
 *     printf("Could not send POST request\n");
 * } else
 *   printf("Could not init POST request\n");
 * ```
 * @param[in] uri the uri of the resource
 * @param[in] endpoint the endpoint of the server
 * @param[in] query a query parameter that will be sent to the server's
 *                  oc_request_callback_t.
 * @param[in] handler function invoked once the client has received the servers
 *                     response to the POST request
 * @param[in] qos the quality of service current options are HIGH_QOS or LOW_QOS
 * @param[in] user_data context pointer that will be sent to the
 *                      oc_response_handler_t
 *
 * @return True if the client successfully prepared the CoAP PUT request
 *
 * @see oc_do_post
 * @see oc_init_put
 */
bool oc_init_post(const char *uri, oc_endpoint_t *endpoint, const char *query,
                  oc_response_handler_t handler, oc_qos_t qos, void *user_data);

/**
 * @brief Dispatch the CoAP POST request wiht content type and accept type CBOR
 *
 * @return true
 * @return false
 *
 * @see oc_do_post_ex
 */
bool oc_do_post(void);

/**
 * @brief  Dispatch the CoAP POST request
 *
 * @param content The content format of the request payload
 * @param accept  The content format of the response payload
 * @return true if the client successfully dispatched the CoAP POST request
 * @return false
 *
 * @see oc_init_post
 */
bool oc_do_post_ex(oc_content_format_t content, oc_content_format_t accept);

/**
 * Dispatch a GET request with the CoAP Observe option to subscribe for
 * notifications from a resource.
 *
 * The oc_response_handler_t will be invoked each time upon receiving a
 * notification.
 *
 * The handler will continue to be invoked till oc_stop_observe() is called.
 *
 * @param[in] uri the uri of the resource
 * @param[in] endpoint the endpoint of the server
 * @param[in] query a query parameter that will be sent to the server's
 *                  oc_request_callback_t.
 * @param[in] handler function invoked once the client has received the servers
 *                     response to the POST request
 * @param[in] qos the quality of service current options are HIGH_QOS or LOW_QOS
 * @param[in] user_data context pointer that will be sent to the
 *                      oc_response_handler_t
 *
 * @return True if the client successfully dispatched the CaAP observer request
 */
bool oc_do_observe(const char *uri, oc_endpoint_t *endpoint, const char *query,
                   oc_response_handler_t handler, oc_qos_t qos,
                   void *user_data);

/**
 * Unsubscribe for notifications from a resource.
 *
 * @param[in] uri the uri of the resource being observed
 * @param[in] endpoint the endpoint of the server
 *
 * @return True if the client successfully dispatched the CaAP stop observer
 *         request
 */
bool oc_stop_observe(const char *uri, oc_endpoint_t *endpoint);

/**
 * stop the multicast update (e.g. do not handle the responses)
 *
 * @param[in] response the response that should not be handled.
 */
void oc_stop_multicast(oc_client_response_t *response);

/**
 * @brief initialize the multicast update
 *
 * @param mcast the multicast address to be used
 * @param uri the uri to be used
 * @param query the query of uri
 * @return true
 * @return false
 */
bool oc_init_multicast_update(oc_endpoint_t *mcast, const char *uri,
                              const char *query);

/**
 * @brief initiate the multi-cast update
 *
 * @return true
 * @return false
 */
bool oc_do_multicast_update(void);

/**
 * Free a list of endpoints from the oc_endpoint_t
 *
 * note: oc_endpoint_t is a linked list. This will walk the list an free all
 * endpoints found in the list. Even if the list only consists of a single
 * endpoint.
 *
 * @param[in,out] endpoint the endpoint list to free
 */
void oc_free_server_endpoints(oc_endpoint_t *endpoint);

/**
 * @brief close the tls session on the indicated endpoint
 *
 * @param endpoint endpoint indicating a session
 */
void oc_close_session(oc_endpoint_t *endpoint);

#ifdef OC_TCP
/**
 * @brief send CoAP ping over the TCP connection
 *
 * @param custody custody on/off
 * @param endpoint endpoint to be used
 * @param timeout_seconds timeout for the ping
 * @param handler the response handler
 * @param user_data the user data to be conveyed to the response handler
 * @return true
 * @return false
 */
bool oc_send_ping(bool custody, oc_endpoint_t *endpoint,
                  uint16_t timeout_seconds, oc_response_handler_t handler,
                  void *user_data);
#endif    /* OC_TCP */
/** @} */ // end of doc_module_tag_client_state

/**  */
/**
  @defgroup doc_module_tag_common_operations Common operations

  This section contains common operations that can be used to schedule
  callbacks.

  @{
*/

/**
 * Schedule a callback to be invoked after a set number of seconds.
 *
 * @param[in] cb_data user defined context pointer that is passed to the
 *                    oc_trigger_t callback
 * @param[in] callback the callback invoked after the set number of seconds
 * @param[in] seconds the number of seconds to wait till the callback is invoked
 */
void oc_set_delayed_callback(void *cb_data, oc_trigger_t callback,
                             uint16_t seconds);

/**
 * Schedule a callback to be invoked after a set number of miliseconds.
 *
 * @param[in] cb_data user defined context pointer that is passed to the
 *                    oc_trigger_t callback
 * @param[in] callback the callback invoked after the set number of seconds
 * @param[in] miliseconds the number of miliseconds to wait till the callback is
 * invoked
 */
void oc_set_delayed_callback_ms(void *cb_data, oc_trigger_t callback,
                                uint16_t miliseconds);

/**
 * used to cancel a delayed callback
 * @param[in] cb_data the user defined context pointer that was passed to the
 *                   oc_sed_delayed_callback() function
 * @param[in] callback the delayed callback that is being removed
 */
void oc_remove_delayed_callback(void *cb_data, oc_trigger_t callback);

/** API for setting handlers for interrupts */

#define oc_signal_interrupt_handler(name)                                      \
  do {                                                                         \
    oc_process_poll(&(name##_interrupt_x));                                    \
    _oc_signal_event_loop();                                                   \
  } while (0)

/** activate the interrupt handler */
#define oc_activate_interrupt_handler(name)                                    \
  (oc_process_start(&(name##_interrupt_x), 0))

/** define the interrupt handler */
#define oc_define_interrupt_handler(name)                                      \
  void name##_interrupt_x_handler(void);                                       \
  OC_PROCESS(name##_interrupt_x, "");                                          \
  OC_PROCESS_THREAD(name##_interrupt_x, ev, data)                              \
  {                                                                            \
    (void)data;                                                                \
    OC_PROCESS_POLLHANDLER(name##_interrupt_x_handler());                      \
    OC_PROCESS_BEGIN();                                                        \
    while (oc_process_is_running(&(name##_interrupt_x))) {                     \
      OC_PROCESS_YIELD();                                                      \
    }                                                                          \
    OC_PROCESS_END();                                                          \
  }                                                                            \
  void name##_interrupt_x_handler(void)
/** @} */ // end of doc_module_tag_common_operations
#ifdef __cplusplus
}
#endif

#endif /* OC_API_H */
