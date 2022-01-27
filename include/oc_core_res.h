/*
// Copyright (c) 2016 Intel Corporation
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
#ifndef OC_CORE_RES_H
#define OC_CORE_RES_H

#include "oc_ri.h"
#include "oc_device_mode.h"
#include "oc_knx.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief callback for initializing the platform
 *
 */
typedef void (*oc_core_init_platform_cb_t)(void *data);

/**
 * @brief callback for adding a device
 *
 */
typedef void (*oc_core_add_device_cb_t)(void *data);

/**
 * @brief platform information
 *
 */
typedef struct oc_platform_info_t
{
  oc_uuid_t pi;                                ///< the platform identifier
  oc_string_t mfg_name;                        ///< manufacturer name
  oc_core_init_platform_cb_t init_platform_cb; ///< callback function
  void *data; ///< user data for the callback function
} oc_platform_info_t;

/**

 */
typedef struct oc_knx_version_info_t
{
  int major; ///< major version number
  int minor; ///< minor version number
  int third; ///< third version number
} oc_knx_version_info_t;

/**
 * @brief device information
 *
 */
typedef struct oc_device_info_t
{
  oc_uuid_t di;     ///< device identifier
  oc_uuid_t piid;   ///< Permanent Immutable ID
  oc_string_t name; ///< name of the device
  oc_string_t icv;  ///< specification version
  oc_string_t dmv;  ///< data model version

  oc_string_t serialnumber;  ///< knx serial number
  oc_knx_version_info_t hwv; ///< knx hardware version
  oc_knx_version_info_t fwv; ///< fwv firmware version number
  oc_string_t hwt;           ///< knx hardware type
  oc_string_t model;         ///< knx model
  int ia;                    ///< knx ia Device individual address
  oc_string_t hostname;      ///< knx host name
  oc_string_t iid;           ///< knx idd (installation id)
  bool pm;                   ///< knx programming mode
  oc_lsm_state_t lsm;        ///< knx lsm states

  oc_device_mode_t device_mode; ///< device mode (programming, normal operation)
  oc_core_add_device_cb_t add_device_cb; ///< callback when device is changed
  void *data;                            ///< user data
} oc_device_info_t;

/**
 * @brief initialize the core functionality
 *
 */
void oc_core_init(void);

/**
 * @brief shutdown the core functionality
 *
 */
void oc_core_shutdown(void);

/**
 * @brief initialize the platform
 *
 * @param mfg_name the manufacturer name
 * @param init_cb the callback
 * @param data  the user data
 * @return oc_platform_info_t* the platform information
 */
oc_platform_info_t *oc_core_init_platform(const char *mfg_name,
                                          oc_core_init_platform_cb_t init_cb,
                                          void *data);

/**
 * @brief Add new device to the platform
 *
 * @param uri the URI of the device
 * @param rt the device type of the device
 * @param name the friendly name
 * @param spec_version specification version
 * @param data_model_version  data model version
 * @param add_device_cb callback
 * @param data supplied user data
 * @return oc_device_info_t* the device information
 */
//oc_device_info_t *oc_core_add_new_device(const char *uri, const char *rt,
//                                         const char *name,
//                                         const char *spec_version,
//                                         const char *data_model_version,
//                                         oc_core_add_device_cb_t add_device_cb,
//                                         void *data);

oc_device_info_t *oc_core_add_device(const char *name, const char *version,
                                     const char *base, const char *serialnumber,
                                     oc_core_add_device_cb_t add_device_cb,
                                     void *data);

/**
 * @brief set the firmware version
 *
 * @param device_index the device index
 * @param major the xxx number of xxx.yyy.zzz
 * @param minor the yyy number of xxx.yyy.zz
 * @param minor2 the zzz number of xxx.yyy.zzz
 * @return int error status, 0 = OK
 */
int oc_core_set_device_fwv(int device_index, int major, int minor, int minor2);

/**
 * @brief sets the hardware version number
 *
 * @param device_index the device index
 * @param major the xxx number of xxx.yyy.zzz
 * @param minor the yyy number of xxx.yyy.zz
 * @param minor2 the zzz number of xxx.yyy.zzz
 * @return int  error status, 0 = OK
 */
int oc_core_set_device_hwv(int device_index, int major, int minor, int minor2);

/**
 * @brief sets the internal address
 *
 * @param device_index the device index
 * @param ia the internal address
 * @return int error status, 0 = OK
 */
int oc_core_set_device_ia(int device_index, int ia);

/**
 * @brief sets the hardware type (string)
 *
 * @param device_index the device index
 * @param hardwaretype the hardware type
 * @return int error status, 0 = OK
 */
int oc_core_set_device_hwt(int device_index, const char *hardwaretype);

/**
 * @brief sets the programming mode (boolean)
 *
 * @param device_index the device index
 * @param pm the programming mode
 * @return int error status, 0 = OK
 */
int oc_core_set_device_pm(int device_index, bool pm);

/**
 * @brief sets the model (string)
 *
 * @param device_index the device index
 * @param model the device model
 * @return int error status, 0 = OK
 */
int oc_core_set_device_model(int device_index, const char *model);

/**
 * @brief sets the host name (string)
 *
 * @param device_index the device index
 * @param hostname the host name
 * @return int error status, 0 = OK
 */
int oc_core_set_device_hostname(int device_index, const char *hostname);

/**
 * @brief sets the iid (string)
 *
 * @param device_index the device index
 * @param iid the KNX installation id
 * @return int error status, 0 = OK
 */
int oc_core_set_device_iid(int device_index, const char *iid);

/**
 * @brief retrieve the amount of devices
 *
 * @return size_t the amount of devices
 */
size_t oc_core_get_num_devices(void);

/**
 * @brief retrieve the id (uuid) of the device
 *
 * @param device the device index
 * @return oc_uuid_t* the device id
 */
oc_uuid_t *oc_core_get_device_id(size_t device);

/**
 * @brief retrieve the device info from the device index
 *
 * @param device the device index
 * @return oc_device_info_t* the device info
 */
oc_device_info_t *oc_core_get_device_info(size_t device);

/**
 * @brief retrieve the platform information
 *
 * @return oc_platform_info_t* the platform information
 */
oc_platform_info_t *oc_core_get_platform_info(void);

/**
 * @brief encode the interfaces with the cbor (payload) encoder
 *
 * @param parent the cbor encoder
 * @param iface_mask the interfaces (as bit mask)
 */
void oc_core_encode_interfaces_mask(CborEncoder *parent,
                                    oc_interface_mask_t iface_mask);

/**
 * @brief retrieve the resource by type (e.g. index) on a specific device
 *
 * @param type the index of the resource
 * @param device the device index
 * @return oc_resource_t* the resource handle
 */
oc_resource_t *oc_core_get_resource_by_index(int type, size_t device);

/**
 * @brief retrieve the resource by uri
 *
 * @param uri the URI
 * @param device the device index
 * @return oc_resource_t* the resource handle
 */
oc_resource_t *oc_core_get_resource_by_uri(const char *uri, size_t device);

/**
 * @brief store the URI as a string
 *
 * @param s_uri source string
 * @param d_uri destination (to be allocated) to store the URI
 */
void oc_store_uri(const char *s_uri, oc_string_t *d_uri);

/**
 * @brief populate resource
 * mainly used for creation of core resources
 *
 * @param core_resource the resource index
 * @param device_index the device index
 * @param uri the URI for the resource
 * @param iface_mask interfaces (as mask) to be implemented on the resource
 * @param default_interface the default interface
 * @param properties the properties (as mask)
 * @param get_cb get callback function
 * @param put_cb put callback function
 * @param post_cb post callback function
 * @param delete_cb delete callback function
 * @param num_resource_types amount of resource types, listed as variable
 * arguments after this argument
 * @param ...
 */
void oc_core_populate_resource(int core_resource, size_t device_index,
                               const char *uri, oc_interface_mask_t iface_mask,
                               oc_interface_mask_t default_interface,
                               int properties, oc_request_callback_t get_cb,
                               oc_request_callback_t put_cb,
                               oc_request_callback_t post_cb,
                               oc_request_callback_t delete_cb,
                               int num_resource_types, ...);

/**
 * @brief populate resource for link-format responses
 * mainly used for creation of core resources
 *
 * @param core_resource the resource index
 * @param device_index the device index
 * @param uri the URI for the resource
 * @param iface_mask interfaces (as mask) to be implemented on the resource
 * @param content_format the content type that should be listed as ct in
 * link-format responses
 * @param properties the properties (as mask)
 * @param get_cb get callback function
 * @param put_cb put callback function
 * @param post_cb post callback function
 * @param delete_cb delete callback function
 * @param num_resource_types amount of resource types, listed as variable
 * arguments after this argument
 * @param ...
 */
void oc_core_lf_populate_resource(
  int core_resource, size_t device_index, const char *uri,
  oc_interface_mask_t iface_mask, oc_content_format_t content_format,
  int properties, oc_request_callback_t get_cb, oc_request_callback_t put_cb,
  oc_request_callback_t post_cb, oc_request_callback_t delete_cb,
  int num_resource_types, ...);

/**
 * @brief filter if the query parameters of the request contains the resource
 * (determined by resource type "rt")
 * including wild carts
 *
 * @param resource the resource to look for
 * @param request the request to scan
 * @return true resource type (or wild card) is in the request
 * @return false resource type is not in the request
 */
bool oc_filter_resource_by_rt(oc_resource_t *resource, oc_request_t *request);

/**
 * @brief filter if the query parameters of the request contains the resource
 * (determined by resource type "if")
 * including wild carts
 *
 * @param resource the resource to look for
 * @param request the request to scan
 * @return true interface type of the resource is in the request
 * @return false interface type of the resource is not in the request
 */
bool oc_filter_resource_by_if(oc_resource_t *resource, oc_request_t *request);

/**
 * @brief frame the interface mask in the response, as string in the uri
 *
 * @param iface_mask The interface masks to frame
 * @return int 0 = success
 */
int oc_frame_interfaces_mask_in_response(oc_interface_mask_t iface_mask);

/**
 * @brief determine if a resource is a Device Configuration Resource
 *
 * @param resource the resource
 * @param device the device index to which the resource belongs too
 * @return true is DCR resource
 * @return false is not DCR resource
 */
bool oc_core_is_DCR(oc_resource_t *resource, size_t device);

/**
 * @brief determine if a resource is Security Vertical Resource
 *
 * @param resource the resource
 * @param device the device index to which the resource belongs too
 * @return true is SRV resource
 * @return false is not SVR resource
 */
bool oc_core_is_SVR(oc_resource_t *resource, size_t device);

/**
 * @brief determine if a resource is a vertical resource
 *
 * @param resource the resource
 * @param device the device index to which the resource belongs too
 * @return true : is vertical resource
 * @return false : is not a vertical resource
 */
bool oc_core_is_vertical_resource(oc_resource_t *resource, size_t device);

/**
 * set the latency (lat) property in eps of oic.wk.res resource.
 * The latency is implemented globally e.g. for all the resource instances.
 * The default behavior is that if nothing is set (e.g. value is 0) the lat
 * property will not be framed in the eps property. Setting the value on 0 will
 * cause that the lat property will not be framed in the eps property.
 * @param[in] latency the latency in seconds
 */
void oc_core_set_latency(int latency);

/**
 * retrieves the latency (lat) property in eps of the oic.wk.res resource.
 * the lat value is implemented globally for the stack
 * @return
 *  - the latency in seconds
 */
int oc_core_get_latency(void);

/**
 * @brief return the number of registered devices
 *
 * @return size_t The number of registered devices
 */
size_t oc_number_of_devices();

#ifdef __cplusplus
}
#endif

#endif /* OC_CORE_RES_H */
