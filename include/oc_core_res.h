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
  @brief KNX mandatory resources implementation.
  @file
*/
#ifndef OC_CORE_RES_H
#define OC_CORE_RES_H

#include "oc_ri.h"
#include "oc_programming_mode.h"
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
  oc_string_t mfg_name;                        /**< manufacturer name */
  oc_core_init_platform_cb_t init_platform_cb; /**< callback function */
  void *data; /**< user data for the callback function */
} oc_platform_info_t;

/**
 * @brief version information
 * e.g. [major, minor, patch]
 */
typedef struct oc_knx_version_info_t
{
  int major; /**< major version number */
  int minor; /**< minor version number */
  int patch; /**< patch version number */
} oc_knx_version_info_t;

/**
 * @brief device information
 *
 * This structure contains
 */
typedef struct oc_device_info_t
{
  oc_string_t serialnumber;  /**< knx serial number */
  oc_knx_version_info_t hwv; /**< knx hardware version */
  oc_knx_version_info_t fwv; /**< fwv firmware version */
  oc_knx_version_info_t ap;  /**< fwv application version */
  oc_string_t hwt; /**< knx hardware type, should not be larger than 6 chars */
  oc_string_t model;    /**< knx model */
  oc_string_t hostname; /**< knx host name */
  uint32_t mid;         /**< knx manufactorer ID */
  uint64_t fid;         /**< knx fabric id */
  uint32_t ia;          /**< knx ia Device individual address */
  uint64_t iid;         /**< knx iid (installation id) */
  uint32_t port;        /**< coap port number */
  uint32_t mport;       /**< multicast port number */
  bool pm;              /**< knx programming mode */
  oc_lsm_state_t lsm_s; /**< knx lsm states */
  oc_core_add_device_cb_t add_device_cb; /**< callback when device is changed */
  void *data;                            /**< user data */
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
 * @brief Add device to the platform
 *
 * @param name the name of the device
 * @param version the version of the KNX spec
 * @param base the base url
 * @param serial_number the serial number of the device
 * @param add_device_cb device callback
 * @param data the supplied user data
 * @return oc_device_info_t* the device structure
 */
oc_device_info_t *oc_core_add_device(const char *name, const char *version,
                                     const char *base,
                                     const char *serial_number,
                                     oc_core_add_device_cb_t add_device_cb,
                                     void *data);

/**
 * @brief set the firmware version
 *
 * @param device_index the device index
 * @param major the xxx number of xxx.yyy.zzz
 * @param minor the yyy number of xxx.yyy.zz
 * @param patch the zzz number of xxx.yyy.zzz
 * @return int error status, 0 = OK
 */
int oc_core_set_device_fwv(size_t device_index, int major, int minor,
                           int patch);

/**
 * @brief sets the hardware version number
 *
 * @param device_index the device index
 * @param major the xxx number of xxx.yyy.zzz
 * @param minor the yyy number of xxx.yyy.zz
 * @param patch the zzz number of xxx.yyy.zzz
 * @return int  error status, 0 = OK
 */
int oc_core_set_device_hwv(size_t device_index, int major, int minor,
                           int patch);
/**
 * @brief sets the application version number
 *
 * @param device_index the device index
 * @param major the xxx number of xxx.yyy.zzz
 * @param minor the yyy number of xxx.yyy.zz
 * @param patch the zzz number of xxx.yyy.zzz
 * @return int  error status, 0 = OK
 */
int oc_core_set_device_ap(size_t device_index, int major, int minor, int patch);

/**
 * @brief sets the manufacturer id
 *
 * @param device_index the device index
 * @param mid the manufacturer id
 * @return int error status, 0 = OK
 */
int oc_core_set_device_mid(size_t device_index, uint32_t mid);

/**
 * @brief sets the internal address
 *
 * @param device_index the device index
 * @param ia the internal address
 * @return int error status, 0 = OK
 */
int oc_core_set_device_ia(size_t device_index, uint32_t ia);

/**
 * @brief sets the manufacturer id
 *
 * @param device_index the device index
 * @param mid  the manufacturer id
 *
 * @return int error status, 0 = OK
 */
int oc_core_set_mid(size_t device_index, int32_t mid);

/**
 * @brief sets and stores the internal address
 *
 * @param device_index the device index
 * @param ia the internal address
 * @return int error status, 0 = OK
 */
int oc_core_set_and_store_device_ia(size_t device_index, uint32_t ia);

/**
 * @brief sets the hardware type (string)
 * input string should not be larger than 6,
 * note that if the input is larger than 6, it will be truncated to 6 chars
 *
 * @param device_index the device index
 * @param hardware_type the hardware type
 * @return int error status, 0 = OK
 */
int oc_core_set_device_hwt(size_t device_index, const char *hardware_type);

/**
 * @brief sets the programming mode (boolean)
 *
 * @param device_index the device index
 * @param pm the programming mode
 * @return int error status, 0 = OK
 */
int oc_core_set_device_pm(size_t device_index, bool pm);

/**
 * @brief sets the model (string)
 *
 * @param device_index the device index
 * @param model the device model
 * @return int error status, 0 = OK
 */
int oc_core_set_device_model(size_t device_index, const char *model);

/**
 * @brief sets the host name (string)
 *
 * @param device_index the device index
 * @param host_name the host name
 * @return int error status, 0 = OK
 */
int oc_core_set_device_hostname(size_t device_index, const char *host_name);

/**
 * @brief sets the installation identifier (iid) (unsigned int)
 *
 * @param device_index the device index
 * @param iid the KNX installation id
 * @return int error status, 0 = OK
 */
int oc_core_set_device_iid(size_t device_index, uint64_t iid);

/**
 * @brief sets the installation identifier (iid) and store it
 *
 * @param device_index the device index
 * @param iid the KNX installation id
 * @return int error status, 0 = OK
 */
int oc_core_set_and_store_device_iid(size_t device_index, uint64_t iid);

/**
 * @brief sets the fabric identifier (fid)
 *
 * @param device_index the device index
 * @param fid the fabric id
 * @return int error status, 0 = OK
 */
int oc_core_set_device_fid(size_t device_index, uint64_t fid);

/**
 * @brief sets the installation identifier (iid) (unsigned int)
 *
 * @param device_index the device index
 * @return The KNX installation id
 */
uint64_t oc_core_get_device_iid(size_t device_index);

/**
 * @brief retrieve the amount of devices
 *
 * @return size_t the amount of devices
 */
size_t oc_core_get_num_devices(void);

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
 * @brief Ensure that the given URI starts with a forward slash
 *
 * @param uri the URI to check
 */
void oc_check_uri(const char *uri);

/**
 * @brief populate core resource
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
 * @param ... Resource types, passed as zero-terminated strings. In order
 * to save memory, the maximum length of each resource type is 32 bytes.
 */
void oc_core_populate_resource(int core_resource, size_t device_index,
                               const char *uri, oc_interface_mask_t iface_mask,
                               oc_content_format_t content_format,
                               int properties, oc_request_callback_t get_cb,
                               oc_request_callback_t put_cb,
                               oc_request_callback_t post_cb,
                               oc_request_callback_t delete_cb,
                               int num_resource_types, ...);

/**
 * @brief bind dpt to a core resource
 *
 * @param core_resource the resource index
 * @param device_index the device index
 * @param dpt the DPT value of the resource
 */
void oc_core_bind_dpt_resource(int core_resource, size_t device_index,
                               const char *dpt);

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
 * example: full tag if= ":if.i"
 * this function frames ":if.i" (truncated) or "urn:knx:if.i"
 * @param iface_mask The interface masks to frame
 * @param truncated 1 = do not frame "urn:knx"
 * @return int 0 = success
 */
int oc_frame_interfaces_mask_in_response(oc_interface_mask_t iface_mask,
                                         int truncated);

#ifdef __cplusplus
}
#endif

#endif /* OC_CORE_RES_H */
