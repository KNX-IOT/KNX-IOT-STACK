/*
// Copyright (c) 2016-2019 Intel Corporation
// Copyright (c) 2021 Cascoda Ltd.
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
  @brief resource internals
  @file
*/
#ifndef OC_RI_H
#define OC_RI_H

#include "oc_config.h"
#include "oc_endpoint.h"
#include "oc_rep.h"
#include "oc_uuid.h"
#include "util/oc_etimer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CoAP methods
 *
 */
typedef enum {
  OC_GET = 1, /**< GET */
  OC_POST,    /**< POST*/
  OC_PUT,     /**< PUT*/
  OC_DELETE,  /**< DELETE*/
  OC_FETCH    /**< FETCH*/
} oc_method_t;

/**
 * @brief resource properties (bit mask)
 *
 */
typedef enum {
  OC_DISCOVERABLE = (1 << 0), /**< discoverable */
  OC_OBSERVABLE = (1 << 1),   /**< observable */
  OC_SECURE = (1 << 4),       /**< secure */
  OC_PERIODIC = (1 << 6),     /**< periodical update */
  OC_SECURE_MCAST = (1 << 8)  /**< secure multi cast (OSCORE) */
} oc_resource_properties_t;

/**
 * @brief CoAP status codes
 *
 * Note: can be translated to HTTP or CoAP.
 *
 * @see oc_status_code for translation to to the CoAP status codes
 */
typedef enum {
  OC_STATUS_OK = 0,                   /**< OK 2.00*/
  OC_STATUS_CREATED,                  /**< Created 2.01*/
  OC_STATUS_CHANGED,                  /**< Changed 2.04*/
  OC_STATUS_DELETED,                  /**< Deleted 2.02 */
  OC_STATUS_NOT_MODIFIED,             /**< Not Modified (VALID 2.03) */
  OC_STATUS_BAD_REQUEST,              /**< Bad Request 4.00*/
  OC_STATUS_UNAUTHORIZED,             /**< Unauthorized 4.01*/
  OC_STATUS_BAD_OPTION,               /**< Bad Option 4.02*/
  OC_STATUS_FORBIDDEN,                /**< Forbidden 4.03*/
  OC_STATUS_NOT_FOUND,                /**< Not Found 4.04*/
  OC_STATUS_METHOD_NOT_ALLOWED,       /**< Method Not Allowed 4.05*/
  OC_STATUS_NOT_ACCEPTABLE,           /**< Not Acceptable 4.06 */
  OC_STATUS_REQUEST_ENTITY_TOO_LARGE, /**< Request Entity Too Large 4.13*/
  OC_STATUS_UNSUPPORTED_MEDIA_TYPE,   /**< Unsupported Media Type 4.15*/
  OC_STATUS_INTERNAL_SERVER_ERROR,    /**< Internal Server Error 5.00 */
  OC_STATUS_NOT_IMPLEMENTED,          /**< Not Implemented 5.01*/
  OC_STATUS_BAD_GATEWAY,              /**< Bad Gateway 5.02*/
  OC_STATUS_SERVICE_UNAVAILABLE,      /**< Service Unavailable 5.03*/
  OC_STATUS_GATEWAY_TIMEOUT,          /**< Gateway Timeout 5.04*/
  OC_STATUS_PROXYING_NOT_SUPPORTED,   /**< Proxying not supported 5.05 */
  __NUM_OC_STATUS_CODES__,
  OC_IGNORE,      /**< Ignore: do not respond to request */
  OC_PING_TIMEOUT /**< Ping Time out */
} oc_status_t;

/**
 * @brief payload content formats
 *
 * https://www.iana.org/assignments/core-parameters/core-parameters.xhtml#rd-parameters
 *
 */
typedef enum {
  TEXT_PLAIN = 0,                    /**< text/plain */
  TEXT_XML = 1,                      /**< text/xml */
  TEXT_CSV = 2,                      /**< text/csv */
  TEXT_HTML = 3,                     /**< text/html */
  IMAGE_GIF = 21,                    /**< image/gif - not used */
  IMAGE_JPEG = 22,                   /**< image/jpeg - not used */
  IMAGE_PNG = 23,                    /**< image/png - not used */
  IMAGE_TIFF = 24,                   /**< image/tiff - not used */
  AUDIO_RAW = 25,                    /**< audio/raw - not used */
  VIDEO_RAW = 26,                    /**< video/raw - not used */
  APPLICATION_LINK_FORMAT = 40,      /**< application/link-format */
  APPLICATION_XML = 41,              /**< application/xml */
  APPLICATION_OCTET_STREAM = 42,     /**< application/octet-stream */
  APPLICATION_RDF_XML = 43,          /**< application - not used */
  APPLICATION_SOAP_XML = 44,         /**< application/soap - not used */
  APPLICATION_ATOM_XML = 45,         /**< application - not used */
  APPLICATION_XMPP_XML = 46,         /**< application - not used */
  APPLICATION_EXI = 47,              /**< application/exi */
  APPLICATION_FASTINFOSET = 48,      /**< application */
  APPLICATION_SOAP_FASTINFOSET = 49, /**< application */
  APPLICATION_JSON = 50,             /**< application/json */
  APPLICATION_X_OBIX_BINARY = 51,    /**< application - not used */
  APPLICATION_CBOR = 60,             /**< application/cbor */
  APPLICATION_SENML_JSON = 110,      /**< application/senml+json */
  APPLICATION_SENSML_JSON = 111,     /**< application/sensml+json */
  APPLICATION_SENML_CBOR = 112,      /**< application/senml+cbor */
  APPLICATION_SENSML_CBOR = 113,     /**< application/sensml+cbor */
  APPLICATION_SENML_EXI = 114,       /**< application/senml-exi */
  APPLICATION_SENSML_EXI = 115,      /**< application/sensml-exi */
  APPLICATION_PKCS7_SGK =
    280, /**< application/pkcs7-mime; smime-type=server-generated-key */
  APPLICATION_PKCS7_CO =
    281, ////**<< application/pkcs7-mime; smime-type=certs-only */
  APPLICATION_PKCS7_CMC_REQUEST =
    282, /**< application/pkcs7-mime; smime-type=CMC-Request */
  APPLICATION_PKCS7_CMC_RESPONSE =
    283, /**< application/pkcs7-mime; smime-type=CMC-Response */
  APPLICATION_PKCS8 = 284,                /**< application/pkcs8 */
  APPLICATION_CRATTRS = 285,              /**< application/csrattrs */
  APPLICATION_PKCS10 = 286,               /**< application/pkcs10 */
  APPLICATION_PKIX_CERT = 287,            /**< application/pkix-cert */
  APPLICATION_VND_OCF_CBOR = 10000,       /**< application/vnd.ocf+cbor */
  APPLICATION_OSCORE = 10001,             /**< application/oscore */
  APPLICATION_VND_OMA_LWM2M_TLV = 11542,  /**< application/vnd.oma.lwm2m+tlv */
  APPLICATION_VND_OMA_LWM2M_JSON = 11543, /**< application/vnd.oma.lwm2m+json */
  APPLICATION_VND_OMA_LWM2M_CBOR = 11544, /**< application/vnd.oma.lwm2m+cbor */
  CONTENT_NONE = 99999                    /**< no content format */
} oc_content_format_t;

/**
 * @brief separate response type
 *
 */
typedef struct oc_separate_response_s oc_separate_response_t;

/**
 * @brief response buffer type
 *
 */
typedef struct oc_response_buffer_s oc_response_buffer_t;

/**
 * @brief response type
 *
 */
typedef struct oc_response_t
{
  oc_separate_response_t *separate_response; /**< separate response */
  oc_response_buffer_t *response_buffer;     /**< response buffer */
} oc_response_t;

/**
 * @brief interface masks
 * security access scopes defined as interfaces
 * note that scope = 1 is not used.
 */
typedef enum {
  OC_IF_NONE = 0,        /**< no interface defined */
  OC_IF_I = (1 << 1),    /**< if.i  (2)*/
  OC_IF_O = (1 << 2),    /**< if.o (4)*/
  OC_IF_G = (1 << 3),    /**< if.g.s.[ga] (8) */
  OC_IF_C = (1 << 4),    /**< if.c (16) */
  OC_IF_P = (1 << 5),    /**< if.p (32)*/
  OC_IF_D = (1 << 6),    /**< if.d (64)*/
  OC_IF_A = (1 << 7),    /**< if.a (128)*/
  OC_IF_S = (1 << 8),    /**< if.s (256)*/
  OC_IF_LI = (1 << 9),   /**< if.ll (512)*/
  OC_IF_B = (1 << 10),   /**< if.b (1024) */
  OC_IF_SEC = (1 << 11), /**< if.sec (2048)*/
  OC_IF_SWU = (1 << 12), /**< if.swu (4096)*/
  OC_IF_PM = (1 << 13),  /**< if.pm (8192)*/
  OC_IF_M = (1 << 14)    /**< if.m (manufacturer) (16384) */
} oc_interface_mask_t;

#define OC_MAX_IF_MASKS 14

/**
 * @brief Get the interface string object
 * Note: should be called with a single interface as mask only
 * @param mask the interface mask (access scope)
 * @return const char* the interface as string e.g. "if.i"
 */
const char *get_interface_string(oc_interface_mask_t mask);

/**
 * @brief Get the method name object
 *
 * @param method the input method
 * @return const char* the method as string e.g. "GET"
 */
const char *get_method_name(oc_method_t method);

/**
 * @brief total interfaces in the interface mask
 * Note calculates the if.g.s.a only 1
 * @param iface_mask the interface mask
 * @return int the amount of interfaces in the mask
 */
int oc_total_interface_in_mask(oc_interface_mask_t iface_mask);

/**
 * @brief sets all interfaces in the mask in the string array
 *
 * @param iface_mask the interface mask
 * @param nr_entries expected number of entries in the array
 * @param interface_array the string array to place the individual interface
 * names in
 * @return int the amount of interfaces filled in the interface_arry
 */
int oc_get_interface_in_mask_in_string_array(oc_interface_mask_t iface_mask,
                                             int nr_entries,
                                             oc_string_array_t interface_array);

/**
 * @brief prints all interfaces in the mask to stdout
 *
 * @param iface_mask the interface mask
 * names in
 */
void oc_print_interface(oc_interface_mask_t iface_mask);

/**
 * @brief core resource numbers
 *
 */
typedef enum {
  OC_DEV_SN = 0, /**< Device serial number */
  OC_DEV_HWV,    /**< Hardware version */
  OC_DEV_FWV,    /**< Firmware version */
  OC_DEV_HWT,   /**< The hardware type is a manufacture specific id for a device
                     type (MaC uses this id for compatibility checks) */
  OC_DEV_MODEL, /**< Device model */
  OC_DEV_IA,    /**< Device individual address */
  OC_DEV_HOSTNAME,          /**< Device host name for DNS resolution. */
  OC_DEV_IID,               /**< KNX installation ID */
  OC_DEV_PM,                /**< Programming Mode */
  OC_DEV_IPV6,              /**< IPV6 */
  OC_DEV_SA,                /**< /dev/sa subnet address */
  OC_DEV_DA,                /**< /dev/da device address */
  OC_DEV_PORT,              /**< /dev/port the coap port number */
  OC_DEV_MPORT,             /**< /dev/mport the multicast port number */
  OC_DEV_MID,               /**< /dev/mid the manufacturer id */
  OC_DEV,                   /**< core link */
  OC_APP,                   /**< application id (list) */
  OC_APP_X,                 /**< application id entry */
  OC_KNX_SPAKE,             /**< spake */
  OC_KNX_IDEVID,            /**< IDevID */
  OC_KNX_LDEVID,            /**< LDevID */
  OC_KNX_LSM,               /**< load state machine */
  OC_KNX_DOT_KNX,           /**< .knx resource */
  OC_KNX_G,                 /**< g (renamed) .knx resource */
  OC_KNX_FINGERPRINT,       /**< FINGERPRINT value of loaded contents */
  OC_KNX_IA,                /**< .well-known / knx / ia */
  OC_KNX_OSN,               /**< .well-known / knx / osn */
  OC_KNX,                   /**< .well-known / knx */
  OC_KNX_FP_G,              /**< FP/G */
  OC_KNX_FP_G_X,            /**< FP/G/X */
  OC_KNX_FP_P,              /**< FP/P */
  OC_KNX_FP_P_X,            /**< FP/P/X */
  OC_KNX_FP_R,              /**< FP/R */
  OC_KNX_FP_R_X,            /**< FP/R/X */
  OC_KNX_P,                 /**< P */
  OC_KNX_F,                 /**< /f */
  OC_KNX_F_X,               /**< /f/X */
  OC_KNX_SWU_PROTOCOL,      /**< software update protocol */
  OC_KNX_SWU_MAXDEFER,      /**< swu max defer */
  OC_KNX_SWU_METHOD,        /**< sw method */
  OC_KNX_LASTUPDATE,        /**< sw last update */
  OC_KNX_SWU_RESULT,        /**< sw result */
  OC_KNX_SWU_STATE,         /**< sw state */
  OC_KNX_SWU_UPDATE,        /**< sw update */
  OC_KNX_SWU_PKGV,          /**< sw package version */
  OC_KNX_SWU_PKGCMD,        /**< sw package command*/
  OC_KNX_SWU_PKGBYTES,      /**< sw package bytes*/
  OC_KNX_SWU_PKGQURL,       /**< sw query url */
  OC_KNX_SWU_PKGNAMES,      /**< sw package names*/
  OC_KNX_SWU_PKG,           /**< sw package */
  OC_KNX_SWU,               /**< swu top level */
  OC_KNX_P_OSCORE_REPLWDO,  /**< oscore replay window*/
  OC_KNX_P_OSCORE_OSNDELAY, /**< oscore osn delay*/
  OC_KNX_F_OSCORE,          /**< oscore/f */
  OC_KNX_A_SEN,             /**< a/sen resource */
  OC_KNX_AUTH,              /**< auth list all sub resources */
  OC_KNX_AUTH_AT,           /**< auth/at resource listing auth/at/X */
  OC_KNX_AUTH_AT_X,         /**< auth/at/X resources */
  OC_KNX_FP_GM,             /**< FP/GM */
  OC_KNX_FP_GM_X,           /**< FP/GM/X */
  /* List of resources on a logical device: start */
  WELLKNOWNCORE /**< well-known/core resource */
  /* List of resources on a logical device: end */
} oc_core_resource_t;

#define OC_NUM_CORE_RESOURCES_PER_DEVICE (1 + WELLKNOWNCORE)

typedef struct oc_resource_s oc_resource_t;

/**
 * @brief request information structure
 *
 */
typedef struct oc_request_t
{
  oc_endpoint_t *origin;     /**< origin of the request */
  oc_resource_t *resource;   /**< resource structure */
  const char *query;         /**< query (as string) */
  size_t query_len;          /**< query length */
  const char *uri_path;      /**< path (as string) */
  size_t uri_path_len;       /**< path length */
  oc_rep_t *request_payload; /**< request payload structure */
  const uint8_t *_payload;   /**< payload of the request */
  size_t _payload_len;       /**< payload size */
  oc_content_format_t
    content_format; /**< content format (of the payload in the request) */
  oc_content_format_t
    accept; /**< accept header, e.g the format to be returned on the request */
  oc_response_t *response; /**< pointer to the response */
} oc_request_t;

/**
 * @brief request callback
 *
 */
typedef void (*oc_request_callback_t)(oc_request_t *, oc_interface_mask_t,
                                      void *);

/**
 * @brief request handler type
 *
 */
typedef struct oc_request_handler_s
{
  oc_request_callback_t cb;
  void *user_data;
} oc_request_handler_t;

/**
 * @brief set properties callback
 *
 */
typedef bool (*oc_set_properties_cb_t)(oc_resource_t *, oc_rep_t *, void *);

/**
 * @brief get properties callback
 *
 */
typedef void (*oc_get_properties_cb_t)(oc_resource_t *, oc_interface_mask_t,
                                       void *);

/**
 * @brief properties callback structure
 *
 */
typedef struct oc_properties_cb_t
{
  union {
    oc_set_properties_cb_t set_props;
    oc_get_properties_cb_t get_props;
  } cb;
  void *user_data;
} oc_properties_cb_t;

/**
 * @brief resource structure
 *
 */
struct oc_resource_s
{
  struct oc_resource_s *next;          /**< next resource */
  size_t device;                       /**< device index */
  oc_string_t name;                    /**< name of the resource (e.g. "n") */
  oc_string_t uri;                     /**< uri of the resource */
  oc_string_array_t types;             /**< "rt" types of the resource */
  oc_string_t dpt;                     /**< dpt of the resource */
  oc_interface_mask_t interfaces;      /**< supported interfaces */
  oc_content_format_t content_type;    /**< the content format that the resource
                                            supports, e.g. only 1 at the moment */
  oc_resource_properties_t properties; /**< properties (as bit mask) */
  oc_request_handler_t get_handler;    /**< callback for GET */
  oc_request_handler_t put_handler;    /**< callback for PUT */
  oc_request_handler_t post_handler;   /**< callback for POST */
  oc_request_handler_t delete_handler; /**< callback for DELETE */
  oc_properties_cb_t get_properties;   /**< callback for get properties */
  oc_properties_cb_t set_properties;   /**< callback for set properties */
  uint8_t num_observers;               /**< amount of observers */
  uint16_t observe_period_seconds;     /**< observe period in seconds */
  uint8_t fb_instance; /**< function block instance, default = 0 */
};

typedef struct oc_link_s oc_link_t;

/**
 * @brief callback return values
 *
 */
typedef enum {
  OC_EVENT_DONE = 0, /**< callback done, e.g. don't call again */
  OC_EVENT_CONTINUE  /**< callbacks continue */
} oc_event_callback_retval_t;

typedef oc_event_callback_retval_t (*oc_trigger_t)(void *);

/**
 * @brief event callback
 *
 */
typedef struct oc_event_callback_s
{
  struct oc_event_callback_s *next; /**< next callback */
  struct oc_etimer timer;           /**< timer */
  oc_trigger_t callback;            /**< callback to be invoked */
  void *data;                       /**< data for the callback */
} oc_event_callback_t;

/**
 * @brief initialize the resource implementation handler
 *
 */
void oc_ri_init(void);

/**
 * @brief shut down the resource implementation handler
 *
 */
void oc_ri_shutdown(void);

/**
 * @brief add timed event callback
 *
 * @param cb_data the timed event callback info
 * @param event_callback the callback
 * @param ticks time in ticks
 */
void oc_ri_add_timed_event_callback_ticks(void *cb_data,
                                          oc_trigger_t event_callback,
                                          oc_clock_time_t ticks);

/**
 * @brief add timed event callback in seconds
 * *
 * @param cb_data the timed event callback info
 * @param event_callback the callback
 * @param seconds time in seconds
 */
#define oc_ri_add_timed_event_callback_seconds(cb_data, event_callback,        \
                                               seconds)                        \
  do {                                                                         \
    oc_ri_add_timed_event_callback_ticks(cb_data, event_callback,              \
                                         (oc_clock_time_t)(seconds) *          \
                                           (oc_clock_time_t)OC_CLOCK_SECOND);  \
  } while (0)

/**
 * @brief remove the timed event callback
 *
 * @param cb_data the timed event callback info
 * @param event_callback the callback
 */
void oc_ri_remove_timed_event_callback(void *cb_data,
                                       oc_trigger_t event_callback);

/**
 * @brief convert the (internal) status code to coap status as integer
 *
 * @param key the application level key of the code
 * @return int the CoAP status code
 */
int oc_status_code(oc_status_t key);

/**
 * @brief checks if the accept header is correct
 * note that if the accept header is not there, this check is a pass
 * @param request the request
 * @param accept the content type of the resource
 * @return true content type is ok
 * @return false content type is not ok
 */
bool oc_check_accept_header(oc_request_t *request, oc_content_format_t accept);

/**
 * @brief retrieve the resource by uri and device index
 *
 * @param uri the uri of the resource
 * @param uri_len the length of the uri
 * @param device the device index
 * @return oc_resource_t* the resource structure
 */
oc_resource_t *oc_ri_get_app_resource_by_uri(const char *uri, size_t uri_len,
                                             size_t device);

/**
 * @brief retrieve list of resources
 *
 * @return oc_resource_t* the resource list
 */
oc_resource_t *oc_ri_get_app_resources(void);

#ifdef OC_SERVER
/**
 * @brief allocate a resource structure
 *
 * @return oc_resource_t*
 */
oc_resource_t *oc_ri_alloc_resource(void);
/**
 * @brief add resource to the system
 *
 * @param resource the resource to be added to the list of application resources
 * @return true success
 * @return false failure
 */
bool oc_ri_add_resource(oc_resource_t *resource);

/**
 * @brief remove the resource from the list of application resources
 *
 * @param resource the resource to be removed from the list of application
 * resources
 * @return true success
 * @return false failure
 */
bool oc_ri_delete_resource(oc_resource_t *resource);
#endif /* OC_SERVER */

/**
 * @brief free the properties of the resource
 *
 * @param resource the resource
 */
void oc_ri_free_resource_properties(oc_resource_t *resource);

/**
 * @brief retrieve the query value at the nth position
 *
 * @param query the input query
 * @param query_len the query length
 * @param key the key
 * @param key_len the length of the key
 * @param value the value belonging to the key
 * @param value_len the length of the value
 * @param n the position to query
 * @return int the position of the next key value pair in the query or NULL
 */
int oc_ri_get_query_nth_key_value(const char *query, size_t query_len,
                                  char **key, size_t *key_len, char **value,
                                  size_t *value_len, size_t n);

/**
 * @brief retrieve the value of the query parameter "key"
 *
 * @param query the input query
 * @param query_len the query length
 * @param key the wanted key
 * @param value the returned value
 * @return int the length of the value
 */
int oc_ri_get_query_value(const char *query, size_t query_len, const char *key,
                          char **value);

/**
 * @brief checks if key exist in query
 *
 * @param[in] query the query to inspect
 * @param[in] query_len the length of the query
 * @param[in] key the key to be checked if exist, key is null terminated
 * @return int -1 = not exist
 */
int oc_ri_query_exists(const char *query, size_t query_len, const char *key);

/**
 * @brief check if the nth key exists
 *
 * @param query the query to inspect
 * @param query_len the length of the query
 * @param key the key to be checked if exist, key is not null terminated
 * @param key_len the key length
 * @param n
 * @return int
 */
int oc_ri_query_nth_key_exists(const char *query, size_t query_len, char **key,
                               size_t *key_len, size_t n);

/**
 * @brief retrieve the interface mask from the interface name
 *
 * @param iface the interface (e.g. "if=if.s")
 * @param if_len the interface length
 * @return oc_interface_mask_t the mask value of the interface
 */
oc_interface_mask_t oc_ri_get_interface_mask(char *iface, size_t if_len);

/**
 * @brief checks if the resource is valid
 *
 * @param resource The resource to be tested
 * @return true valid
 * @return false not valid
 */
bool oc_ri_is_app_resource_valid(oc_resource_t *resource);

/**
 * @brief create a new request from the old request
 * is used internally only for redirection of:
 * - .knx
 * - p
 *
 * @param new_request the original request
 * @param request the new request
 * @param response_buffer the dummy response buffer for the new request
 * @param response_obj the dummy response object
 * @return true new request valid
 * @return false new request invalid
 */
bool oc_ri_new_request_from_request(oc_request_t new_request,
                                    oc_request_t request,
                                    oc_response_buffer_t response_buffer,
                                    oc_response_t response_obj);

#ifdef __cplusplus
}
#endif

#endif /* OC_RI_H */
