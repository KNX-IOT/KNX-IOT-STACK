/*
// Copyright(c) 2021 Cascoda Ltd.
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

#include "oc_python.h"
#include "oc_api.h"
#include "oc_core_res.h"
#include "port/oc_clock.h"
#include "api/oc_knx_fp.h"
#include "api/oc_knx_gm.h"
#include "port/oc_connectivity.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)

#include <unistd.h>
#include <pthread.h>
#else
#error "Unsupported OS"
#endif
#include <signal.h>
#include <stdio.h>

#include <string.h>

#define MAX_NUM_DEVICES (50)
#define MAX_NUM_RESOURCES (100)
#define MAX_NUM_RT (50)
#define MAX_URI_LENGTH (30)
#define MAX_SERIAL_NUM_LENGTH (20)

char g_serial_number[MAX_SERIAL_NUM_LENGTH]; /**< the serial number to be set on
                                                the stack */

typedef struct device_handle_t
{
  struct device_handle_t *next;                     /**< next in the list */
  char device_serial_number[MAX_SERIAL_NUM_LENGTH]; /**< serial number of the
                                                       device*/
  char device_name[64]; /**< the device name (not used) */
  char ip_address[100]; /**< the ip address (ipv6 as string) */
  oc_endpoint_t ep;     /**< the endpoint to talk to the device */
} device_handle_t;

/* Pool of device handles */
OC_MEMB(device_handles, device_handle_t, MAX_OWNED_DEVICES);
/* List of known owned devices */
OC_LIST(discovered_devices);

#if defined(_WIN32)
static HANDLE event_thread;
static CRITICAL_SECTION app_sync_lock;
static CONDITION_VARIABLE cv;
static CRITICAL_SECTION cs;

/* OS specific definition for lock/unlock */
#define ets_mutex_lock(m) EnterCriticalSection(&m)
#define ets_mutex_unlock(m) LeaveCriticalSection(&m)

#elif defined(__linux__)
static pthread_t event_thread;
static pthread_mutex_t app_sync_lock;
static pthread_mutex_t mutex;
static pthread_cond_t cv;

/* OS specific definition for lock/unlock */
#define ets_mutex_lock(m) pthread_mutex_lock(&m)
#define ets_mutex_unlock(m) pthread_mutex_unlock(&m)

static struct timespec ts;
#endif
static int quit = 0;

// -----------------------------------------------------------------------------
// forward declarations

static void signal_event_loop(void);

// -----------------------------------------------------------------------------

/**
 * structure with the callbacks
 *
 */
typedef struct py_cb_struct
{
  volatile changedCB changedFCB;
  volatile resourceCB resourceFCB;
  volatile clientCB clientFCB;
  volatile discoveryCB discoveryFCB;
  volatile spakeCB spakeFCB;
  volatile gatewayCB gatewayFCB;
} py_cb_struct_t;

/**
 * structure with user data
 *
 */
typedef struct user_struct_t
{
  char url[200];
  char sn[200];
  char r_id[200];
} user_struct_t;

/**
 * global declaration of the callback
 *
 */
struct py_cb_struct my_CBFunctions;

// -----------------------------------------------------------------------------

/**
 * Function to return response strings
 *
 */
static inline char *
stringFromResponse(int code)
{
  static char *strings[] = { "STATUS_OK",
                             "STATUS_CREATED"
                             "STATUS_CHANGED",
                             "STATUS_DELETED",
                             "STATUS_NOT_MODIFIED",
                             "STATUS_BAD_REQUEST",
                             "STATUS_UNAUTHORIZED",
                             "STATUS_BAD_OPTION",
                             "STATUS_FORBIDDEN",
                             "STATUS_NOT_FOUND",
                             "STATUS_METHOD_NOT_ALLOWED",
                             "STATUS_NOT_ACCEPTABLE",
                             "STATUS_REQUEST_ENTITY_TOO_LARGE",
                             "STATUS_UNSUPPORTED_MEDIA_TYPE",
                             "STATUS_INTERNAL_SERVER_ERROR",
                             "STATUS_NOT_IMPLEMENTED",
                             "STATUS_BAD_GATEWAY",
                             "STATUS_SERVICE_UNAVAILABLE",
                             "STATUS_GATEWAY_TIMEOUT",
                             "STATUS_PROXYING_NOT_SUPPORTED",
                             "__NUM_STATUS_CODES__",
                             "IGNORE",
                             "PING_TIMEOUT" };
  if (code < 21) {
    return strings[code];
  } else {
    return " unknown error";
  }
}

/**
 * function to print the returned cbor as JSON
 *
 */
void
print_rep(oc_rep_t *rep, bool pretty_print)
{
  char *json;
  size_t json_size;
  json_size = oc_rep_to_json(rep, NULL, 0, pretty_print);
  json = (char *)malloc(json_size + 1);
  oc_rep_to_json(rep, json, json_size + 1, pretty_print);
  printf("%s\n", json);
  free(json);
}

// -----------------------------------------------------------------------------

/**
 * function to install callbacks
 *
 */
void
ets_install_changedCB(changedCB changedCB)
{
  PRINT("[C]install_changedCB %p\n", changedCB);
  my_CBFunctions.changedFCB = changedCB;
}

/**
 * function to install resource callbacks
 *
 */
void
ets_install_resourceCB(resourceCB resourceCB)
{
  PRINT("[C]install_resourceCB: %p\n", resourceCB);
  my_CBFunctions.resourceFCB = resourceCB;
}
/**
 * function to install client callbacks, called from python
 *
 */
void
ets_install_clientCB(clientCB clientCB)
{
  PRINT("[C]install_clientCB: %p\n", clientCB);
  my_CBFunctions.clientFCB = clientCB;
}

/**
 * function to install discovery callbacks, called from python
 *
 */
void
ets_install_discoveryCB(discoveryCB discoveryCB)
{
  PRINT("[C]install_discoveryCB: %p\n", discoveryCB);
  my_CBFunctions.discoveryFCB = discoveryCB;
}

/**
 * function to install spake callbacks, called from python
 *
 */
void
ets_install_spakeCB(spakeCB spakeCB)
{
  PRINT("[C]install_spakeCB: %p\n", spakeCB);
  my_CBFunctions.spakeFCB = spakeCB;
}

void
internal_gw_cb(size_t device_index, char *sender_ip_address,
               oc_group_object_notification_t *s_mode_message, void *data)
{
  (void)data;
  char buffer[300];

  PRINT("[c]internal_gw_cb %d from %s\n", (int)device_index, sender_ip_address);
  PRINT("   ga  = %d\n", s_mode_message->ga);
  PRINT("   sia = %d\n", s_mode_message->sia);
  PRINT("   st  = %s\n", oc_string(s_mode_message->st));
  PRINT("   val = %s\n", oc_string(s_mode_message->value));

  oc_s_mode_notification_to_json(buffer, 300, *s_mode_message);
  if (my_CBFunctions.gatewayFCB) {
    my_CBFunctions.gatewayFCB(sender_ip_address, 300, buffer);
  }
}

void
ets_install_gatewayCB(gatewayCB gatewayCB)
{
  PRINT("[C]install_gatewayCB: %p\n", gatewayCB);
  // set the python callback, e.g. do the conversion from s-mode message to json
  my_CBFunctions.gatewayFCB = gatewayCB;

  // set the gateway callback on the stack
  oc_set_gateway_cb(internal_gw_cb, NULL);
}

/**
 * function to call the callback to python.
 *
 */
void
inform_python(const char *uuid, const char *state, const char *event)
{
  PRINT("[C]inform_python %p\n", my_CBFunctions.changedFCB);
  if (my_CBFunctions.changedFCB != NULL) {
    PRINT("[C]inform_python CB %p\n", my_CBFunctions.changedFCB);
    my_CBFunctions.changedFCB((char *)uuid, (char *)state, (char *)event);
  }
}

void
inform_resource_python(const char *anchor, const char *uri, const char *types,
                       const char *interfaces)
{
  // PRINT("[C]inform_resource_python %p %s %s [%s]
  // [%s]\n",my_CBFunctions.resourceFCB, anchor, uri, types, interfaces);
  if (my_CBFunctions.resourceFCB != NULL) {
    my_CBFunctions.resourceFCB((char *)anchor, (char *)uri, (char *)types,
                               (char *)interfaces);
  }
}

/**
 * function to call the callback for clients to python.
 *
 */
void
inform_client_python(const char *sn, int status, const char *format,
                     const char *r_id, const char *url, int payload_size,
                     const char *payload)
{
  if (my_CBFunctions.clientFCB != NULL) {
    my_CBFunctions.clientFCB((char *)sn, status, (char *)format, (char *)r_id,
                             (char *)url, payload_size, (char *)payload);
  }
}

/**
 * function to call the callback for discovery to python.
 *
 */
void
inform_discovery_python(int payload_size, const char *payload)
{
  if (my_CBFunctions.discoveryFCB != NULL) {
    my_CBFunctions.discoveryFCB(payload_size, (char *)payload);
  }
}

/**
 * function to call the callback for discovery to python.
 * CFUNCTYPE(None, c_int,  c_char_p, c_char_p, c_int)
 */
void
inform_spake_python(char *sn, int state, char *oscore_id, char *key,
                    int key_size)
{
  PRINT(
    "[C]inform_spake_python %p sn:%s state:%d oscore_id:%s key_size:%d key=[",
    my_CBFunctions.spakeFCB, sn, state, oscore_id, key_size);
  for (int i = 0; i < key_size; i++) {
    PRINT("%02x", (unsigned char)key[i]);
  }
  PRINT("]\n");

  if (my_CBFunctions.spakeFCB != NULL) {
    my_CBFunctions.spakeFCB(sn, state, oscore_id, (uint8_t *)key, key_size);
  }
}

// -----------------------------------------------------------------------------

/**
 * function to convert the sn to the device handle
 *
 */
device_handle_t *
ets_getdevice_from_sn(char *sn)
{
  device_handle_t *device = NULL;
  device = (device_handle_t *)oc_list_head(discovered_devices);

  int i = 0;
  while (device != NULL) {
    if (strcmp(device->device_serial_number, sn) == 0) {
      return device;
    }
    i++;
    device = device->next;
  }
  return NULL;
}

// -----------------------------------------------------------------------------

/**
 * start the application, e.g. the python client
 */
static int
app_init(void)
{
  int ret = oc_init_platform("Cascoda", NULL, NULL);

  if (strlen(g_serial_number) > 0) {
    ret |=
      oc_add_device("py-client", "1.0.0", "//", g_serial_number, NULL, NULL);

  } else {

    ret |= oc_add_device("py-client", "1.0.0", "//", "012349", NULL, NULL);
  }

  /* set the programming mode */
  oc_core_set_device_pm(0, false);

  return ret;
}

// -----------------------------------------------------------------------------

/* Application utility functions */
static device_handle_t *
is_device_in_list(char *sn, oc_list_t list)
{
  device_handle_t *device = (device_handle_t *)oc_list_head(list);
  while (device != NULL) {
    if (strcmp(device->device_serial_number, sn) == 0) {
      return device;
    }
    device = device->next;
  }
  return NULL;
}

// -----------------------------------------------------------------------------

static bool
add_device_to_list(char *sn, const char *device_name, char *ip_address,
                   oc_endpoint_t *ep, oc_list_t list)
{
  device_handle_t *device = is_device_in_list(sn, list);

  if (!device) {
    device = oc_memb_alloc(&device_handles);
    if (!device) {
      return false;
    }
    strcpy(device->device_serial_number, sn);
    PRINT("[C] add_device_to_list adding device %s\n", sn);
    oc_list_add(list, device);
  }
  if (ip_address) {
    strcpy(device->ip_address, ip_address);
  }
  if (ep) {
    // strcpy(device->ip_address, ip_address);
    oc_endpoint_copy(&device->ep, ep);
    // oc_endpoint_list_copy(&device->ep, ep);
  }

  if (device_name) {
    size_t len = strlen(device_name);
    len = (len > 63) ? 63 : len;
    strncpy(device->device_name, device_name, len);
    device->device_name[len] = '\0';
  } else {
    device->device_name[0] = '\0';
  }
  return true;
}

void
empty_device_list(oc_list_t list)
{
  device_handle_t *device = (device_handle_t *)oc_list_pop(list);
  while (device != NULL) {
    oc_memb_free(&device_handles, device);
    device = (device_handle_t *)oc_list_pop(list);
  }
}
/* End of app utility functions */

/* App invocations of APIs */

#define buffer_size (8 * 1024)
char buffer[buffer_size];

void
general_get_cb(oc_client_response_t *data)
{
  oc_status_t status = data->code;
  user_struct_t *my_data = (user_struct_t *)data->user_data;
  if (my_data) {
    PRINT(" [C]general_get_cb: response status:(%d) fmt:(%d) sn:[%s] r_id:[%s] "
          "url:[%s]\n",
          (int)status, data->content_format, my_data->sn, my_data->r_id,
          my_data->url);
  } else {
    PRINT(" [C]general_get_cb: response status %d: %s \n", (int)status,
          stringFromResponse((int)status));
  }
  if (my_data) {
    if (data->content_format == APPLICATION_LINK_FORMAT) {
      ets_mutex_lock(app_sync_lock);
      memset(buffer, 0, buffer_size);
      memcpy(&buffer, (char *)data->_payload, (int)data->_payload_len);

      inform_client_python((char *)my_data->sn, status, "link_format",
                           (char *)my_data->r_id, (char *)my_data->url,
                           (int)data->_payload_len, buffer);
      ets_mutex_unlock(app_sync_lock);
    } else if (data->content_format == APPLICATION_CBOR) {
      ets_mutex_lock(app_sync_lock);
      memset(buffer, 0, buffer_size);
      int json_size =
        py_oc_rep_to_json(data->payload, (char *)&buffer, buffer_size, false);
      inform_client_python((char *)my_data->sn, status, "json",
                           (char *)my_data->r_id, (char *)my_data->url,
                           (int)json_size, (char *)buffer);
      ets_mutex_unlock(app_sync_lock);
    } else {
      PRINT(" [C]informing python with error \n");
      inform_client_python((char *)my_data->sn, status, "error",
                           (char *)my_data->r_id, (char *)my_data->url, status,
                           "");
    }
  }

  if (data->user_data != NULL) {
    free(data->user_data);
  }
}

void
ets_cbor_get(char *sn, char *uri, char *query, char *cbdata)
{
  int ret = -1;
  device_handle_t *device = ets_getdevice_from_sn(sn);

  user_struct_t *new_cbdata;
  new_cbdata = (user_struct_t *)malloc(sizeof(user_struct_t));
  if (new_cbdata != NULL) {
    strcpy(new_cbdata->r_id, cbdata);
    strcpy(new_cbdata->url, uri);
    strcpy(new_cbdata->sn, sn);
  }

  PRINT("  [C]ets_cbor_get: [%s], [%s] [%s] [%s]\n", sn, uri, query, cbdata);
#ifdef OC_OSCORE
  device->ep.flags += OSCORE;
  PRINT("  [C] enable OSCORE encryption\n");
  oc_string_copy_from_char(&device->ep.serial_number, sn);
  PRINT("  [C] ep serial %s\n", oc_string(device->ep.serial_number));
#endif

  ret = oc_do_get_ex(uri, &device->ep, query, general_get_cb, HIGH_QOS,
                     APPLICATION_CBOR, APPLICATION_CBOR, new_cbdata);
  if (ret >= 0) {
    PRINT("  [C]Successfully issued GET request\n");
  } else {
    PRINT("  [C]ERROR issuing GET request\n");
  }
}

void
ets_cbor_get_unsecured(char *sn, char *uri, char *query, char *cbdata)
{
  int ret = -1;
  device_handle_t *device = ets_getdevice_from_sn(sn);

  user_struct_t *new_cbdata;
  new_cbdata = (user_struct_t *)malloc(sizeof(user_struct_t));
  if (new_cbdata != NULL) {
    strcpy(new_cbdata->r_id, cbdata);
    strcpy(new_cbdata->url, uri);
    strcpy(new_cbdata->sn, sn);
  }

  PRINT("  [C]ets_cbor_get_unsecured: [%s], [%s] [%s] [%s]\n", sn, uri, query,
        cbdata);

  /* remove OSCORE flag*/
  device->ep.flags &= OSCORE;
  PRINTipaddr_flags(device->ep);

  ret = oc_do_get_ex(uri, &device->ep, query, general_get_cb, HIGH_QOS,
                     APPLICATION_CBOR, APPLICATION_CBOR, new_cbdata);
  if (ret >= 0) {
    PRINT("  [C]Successfully issued GET request\n");
  } else {
    PRINT("  [C]ERROR issuing GET request\n");
  }
}

void
ets_linkformat_get(char *sn, char *uri, char *query, char *cbdata)
{
  int ret = -1;
  oc_endpoint_t ep;
  device_handle_t *device = ets_getdevice_from_sn(sn);

  PRINT("  [C]ets_linkformat_get: [%s], [%s] [%s] [%s]\n", sn, uri, query,
        cbdata);
#ifdef OC_OSCORE
  PRINT(" [C] enable OSCORE encryption\n");
  device->ep.flags != OSCORE;
  PRINTipaddr_flags(device->ep);
  oc_string_copy_from_char(&device->ep.serial_number, sn);
#endif

  user_struct_t *new_cbdata;
  new_cbdata = (user_struct_t *)malloc(sizeof(user_struct_t));
  if (new_cbdata != NULL) {
    strcpy(new_cbdata->r_id, cbdata);
    strcpy(new_cbdata->url, uri);
    strcpy(new_cbdata->sn, sn);
  }

  oc_endpoint_print(&device->ep);
  if (&ep != NULL) {
    ret = oc_do_get_ex(uri, &device->ep, query, general_get_cb, HIGH_QOS,
                       APPLICATION_LINK_FORMAT, APPLICATION_LINK_FORMAT,
                       new_cbdata);
  }
  if (ret >= 0) {
    PRINT("  [C]Successfully issued GET request\n");
  } else {
    PRINT("  [C]ERROR issuing GET request\n");
  }
}

void
ets_linkformat_get_unsecured(char *sn, char *uri, char *query, char *cbdata)
{
  int ret = -1;
  oc_endpoint_t ep;
  device_handle_t *device = ets_getdevice_from_sn(sn);

  PRINT("  [C]ets_linkformat_get_unsecured: [%s], [%s] [%s] [%s]\n", sn, uri,
        query, cbdata);
  device->ep.flags &= OSCORE;
  PRINTipaddr_flags(device->ep);

  user_struct_t *new_cbdata;
  new_cbdata = (user_struct_t *)malloc(sizeof(user_struct_t));
  if (new_cbdata != NULL) {
    strcpy(new_cbdata->r_id, cbdata);
    strcpy(new_cbdata->url, uri);
    strcpy(new_cbdata->sn, sn);
  }

  oc_endpoint_print(&device->ep);
  if (&ep != NULL) {
    ret = oc_do_get_ex(uri, &device->ep, query, general_get_cb, HIGH_QOS,
                       APPLICATION_LINK_FORMAT, APPLICATION_LINK_FORMAT,
                       new_cbdata);
  }
  if (ret >= 0) {
    PRINT("  [C]Successfully issued unsecured GET request (linkformat)\n");
  } else {
    PRINT("  [C]ERROR issuing GET request\n");
  }
}

// -----------------------------------------------------------------------------

void
ets_cbor_post(char *sn, char *uri, char *query, char *id, int size, char *data)
{
  int ret = -1;
  device_handle_t *device = ets_getdevice_from_sn(sn);

  PRINT("  [C]ets_cbor_post: [%s], [%s] [%s] [%s] %d\n", sn, uri, id, query,
        size);
#ifdef OC_OSCORE
  device->ep.flags |= OSCORE;
  PRINT("  [C] enable OSCORE encryption\n");
  PRINTipaddr_flags(device->ep);
  oc_string_copy_from_char(&device->ep.serial_number, sn);
#endif

  user_struct_t *new_cbdata;
  new_cbdata = (user_struct_t *)malloc(sizeof(user_struct_t));
  if (new_cbdata != NULL) {
    strcpy(new_cbdata->r_id, id);
    strcpy(new_cbdata->url, uri);
    strcpy(new_cbdata->sn, sn);
  }

  if (oc_init_post(uri, &device->ep, NULL, general_get_cb, HIGH_QOS,
                   new_cbdata)) {
    // encode the request data..it should already be cbor
    oc_rep_encode_raw(data, (size_t)size);

    if (oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
      PRINT("  [C]Sent POST request\n");
    } else {
      PRINT("  [C]Could not send POST request\n");
    }
  }
}

// -----------------------------------------------------------------------------

void
ets_cbor_put(char *sn, char *uri, char *query, char *id, int size, char *data)
{
  int ret = -1;
  device_handle_t *device = ets_getdevice_from_sn(sn);

  PRINT("  [C]ets_cbor_put: [%s], [%s] [%s] [%s] %d\n", sn, uri, id, query,
        size);
#ifdef OC_OSCORE
  device->ep.flags |= OSCORE;
  PRINT("  [C] enable OSCORE encryption\n");
  PRINTipaddr_flags(device->ep);
  oc_string_copy_from_char(&device->ep.serial_number, sn);
#endif

  user_struct_t *new_cbdata;
  new_cbdata = (user_struct_t *)malloc(sizeof(user_struct_t));
  if (new_cbdata != NULL) {
    strcpy(new_cbdata->r_id, id);
    strcpy(new_cbdata->url, uri);
    strcpy(new_cbdata->sn, sn);
  }

  if (oc_init_put(uri, &device->ep, NULL, general_get_cb, HIGH_QOS,
                  (char *)new_cbdata)) {
    // encode the request data..it should already be cbor
    oc_rep_encode_raw(data, (size_t)size);

    if (oc_do_put_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
      PRINT("  [C]Sent PUT request\n");
    } else {
      PRINT("  [C]Could not send PUT request\n");
    }
  }
}

// -----------------------------------------------------------------------------

void
ets_cbor_delete(char *sn, char *uri, char *query, char *id)
{
  int ret = -1;
  device_handle_t *device = ets_getdevice_from_sn(sn);

  PRINT("  [C]ets_cbor_delete: [%s], [%s] [%s] [%s]\n", sn, uri, id, query);
#ifdef OC_OSCORE
  device->ep.flags |= OSCORE;
  PRINT("  [C] enable OSCORE encryption\n");
  PRINTipaddr_flags(device->ep);
  oc_string_copy_from_char(&device->ep.serial_number, sn);
#endif

  user_struct_t *new_cbdata;
  new_cbdata = (user_struct_t *)malloc(sizeof(user_struct_t));
  if (new_cbdata != NULL) {
    strcpy(new_cbdata->r_id, id);
    strcpy(new_cbdata->url, uri);
    strcpy(new_cbdata->sn, sn);
  }

  if (oc_do_delete(uri, &device->ep, query, general_get_cb, HIGH_QOS,
                   new_cbdata)) {
    PRINT("  [C]Sent DELETE request\n");
  } else {
    PRINT("  [C]Could not send DELETE request\n");
  }
}

// -----------------------------------------------------------------------------

void
response_get_sn(oc_client_response_t *data)
{
  PRINT("[C]response_get_sn: content format %d  %.*s\n", data->content_format,
        (int)data->_payload_len, data->_payload);
  oc_rep_t *rep = data->payload;
  oc_string_t my_address;
  char *my_sn = NULL;

  oc_endpoint_to_string(data->endpoint, &my_address);

  while (rep != NULL) {
    if ((rep->iname = 1) && (rep->type == OC_REP_STRING)) {
      my_sn = oc_string(rep->value.string);
      PRINT("[C]  get_sn received %s (address) :%s\n", my_sn,
            oc_string(my_address));
    }
    rep = rep->next;
  }

  if (my_sn) {
    add_device_to_list(my_sn, NULL, oc_string(my_address), data->endpoint,
                       discovered_devices);
    inform_python(my_sn, oc_string(my_address), "discovered");
  }
  oc_free_string(&my_address);
}

// -----------------------------------------------------------------------------

static oc_discovery_flags_t
discovery_cb(const char *payload, int len, oc_endpoint_t *endpoint,
             void *user_data)
{
  //(void)anchor;
  (void)user_data;
  (void)endpoint;
  const char *uri;
  int uri_len;
  const char *param;
  int param_len;

  PRINT("[C]DISCOVERY: %.*s\n", len, payload);
  int nr_entries = oc_lf_number_of_entries(payload, len);
  PRINT("[C] entries %d\n", nr_entries);
  int found = 0;

  for (int i = 0; i < nr_entries; i++) {

    oc_lf_get_entry_uri(payload, len, i, &uri, &uri_len);
    PRINT("[C] DISCOVERY URL %.*s\n", uri_len, uri);
    // oc_string_to_endpoint()
    found = oc_lf_get_entry_param(payload, len, i, "rt", &param, &param_len);
    if (found) {
      PRINT("    RT %.*s\n", param_len, param);
    }
    found = oc_lf_get_entry_param(payload, len, i, "if", &param, &param_len);
    if (found) {
      PRINT("    IF %.*s\n", param_len, param);
    }
    found = oc_lf_get_entry_param(payload, len, i, "ct", &param, &param_len);
    if (found) {
      PRINT("    CT %.*s\n", param_len, param);
    }
    found = oc_lf_get_entry_param(payload, len, i, "ep", &param, &param_len);
    if (found) {
      PRINT("    EP %.*s\n", param_len, param);
      char sn[30];
      memset(sn, 0, 30);
      PRINT("    PARAM %.*s \n", param_len, param);
      // ep = urn:knx:sn.0004000
      oc_string_t my_address;
      oc_endpoint_to_string(endpoint, &my_address);
      PRINT("    address: %s\n", oc_string(my_address));
      if (param_len > 11) {

        strncpy(sn, &param[11], param_len - 11);
        PRINT("    SN: %s\n", sn);
        add_device_to_list(sn, NULL, oc_string(my_address), endpoint,
                           discovered_devices);
        inform_python(sn, oc_string(my_address), "discovered");
      }
    }
  }

  inform_discovery_python(len, payload);

  // PRINT("[C] issue get on /dev/sn\n");
  // oc_endpoint_print(endpoint);

  // oc_do_get_ex("/dev/sn", endpoint, NULL, response_get_sn, HIGH_QOS,
  //             APPLICATION_CBOR, APPLICATION_CBOR, endpoint);

  PRINT("[C] DISCOVERY- END\n");
  // return OC_CONTINUE_DISCOVERY;
  return OC_STOP_DISCOVERY;
}

void
ets_discover_devices(int scope)
{
  ets_mutex_lock(app_sync_lock);
  oc_do_wk_discovery_all("rt=urn:knx:dpa.*", scope, discovery_cb, NULL);
  ets_mutex_unlock(app_sync_lock);
  signal_event_loop();
}

void
ets_discover_devices_with_query(int scope, const char *query)
{
  ets_mutex_lock(app_sync_lock);
  oc_do_wk_discovery_all(query, scope, discovery_cb, NULL);
  ets_mutex_unlock(app_sync_lock);
  signal_event_loop();
}

// -----------------------------------------------------------------------------

void
ets_initiate_spake(char *sn, char *password, char *oscore_id)
{
  int ret = -1;
  device_handle_t *device = ets_getdevice_from_sn(sn);

  /* remove OSCORE flag */
  device->ep.flags = IPV6;
  PRINT("  [C] disable OSCORE encryption\n");
  PRINTipaddr_flags(device->ep);

  PRINT("  [C]ets_initiate_spake: [%s] [%s]\n", sn, password);
  if (oc_string_len(device->ep.serial_number) == 0) {
    oc_new_string(&device->ep.serial_number, sn, strlen(sn));
  }
  ret = oc_initiate_spake(&device->ep, password, oscore_id);
  PRINT("  [C]ets_initiate_spake: [%d]-- done\n", ret);
  if (ret == -1) {
    // failure, so unblock python
    inform_spake_python(sn, ret, "", "", 0);
  }
}

void
spake_callback(int error, char *sn, char *oscore_id, uint8_t *secret,
               int secret_size)
{
  inform_spake_python(sn, error, oscore_id, secret, secret_size);
}

// -----------------------------------------------------------------------------

/* send a multicast s-mode message */
void
ets_issue_requests_s_mode(int scope, int sia, int ga, int iid, char *st,
                          int value_type, char *value)
{
  PRINT(" [C] ets_issue_requests_s_mode\n");

  oc_endpoint_t mcast;
  memset(&mcast, 0, sizeof(mcast));
  mcast = oc_create_multicast_group_address(mcast, ga, iid, scope);

  if (oc_init_post("/.knx", &mcast, NULL, NULL, LOW_QOS, NULL)) {
    /*
    { 4: sia, 5: { 6: <st>, 7: <ga>, 1: <value> } }
    */
    oc_rep_begin_root_object();
    oc_rep_i_set_int(root, 4, sia);
    oc_rep_i_set_key(&root_map, 5);
    CborEncoder value_map;
    cbor_encoder_create_map(&root_map, &value_map, CborIndefiniteLength);
    // ga
    oc_rep_i_set_int(value, 7, ga);
    // st M Service type code(write = w, read = r, response = rp) Enum : w, r,
    // rp
    // oc_rep_i_set_text_string(value, 6, oc_string(g_send_notification.st));
    oc_rep_i_set_text_string(value, 6, st);
    if (value_type == 0) {
      // boolean
      bool bool_value = false;
      if (strcmp(value, "true") == 0) {
        bool_value = true;
      }
      if (strcmp(value, "false") == 0) {
        bool_value = false;
      }
      oc_rep_i_set_boolean(value, 1, bool_value);
    }
    if (value_type == 1) {
      // integer
      int int_value = atoi(value);
      oc_rep_i_set_int(value, 1, (int)int_value);
    }
    if (value_type == 2) {
      // float
      double float_value = atof(value);
      oc_rep_i_set_double(value, 1, (double)float_value);
    }
    cbor_encoder_close_container_checked(&root_map, &value_map);
    oc_rep_end_root_object();

    PRINT("S-MODE Payload Size: %d\n", oc_rep_get_encoded_payload_size());
    OC_LOGbytes_OSCORE(oc_rep_get_encoder_buf(),
                       oc_rep_get_encoded_payload_size());

#ifndef OC_OSCORE
    if (oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
      PRINT("  Sent POST request\n");
#else
    if (oc_do_multicast_update()) {
      PRINT("  Sent oc_do_multicast_update update\n");
#endif
    } else {
      PRINT("  Could not send POST request\n");
    }
  }
}

// -----------------------------------------------------------------------------

void
ets_listen_s_mode(int scope, int ga_max, int iid)
{

  for (int i = 1; i < ga_max; i++) {
    subscribe_group_to_multicast(i, iid, scope);
  }
}

// -----------------------------------------------------------------------------

char *
ets_error_to_string(int error_code)
{
  return stringFromResponse(error_code);
}

/**
 * function to retrieve the serial number of the discovered device
 *
 */
char *
ets_get_sn(int index)
{
  device_handle_t *device = NULL;
  device = (device_handle_t *)oc_list_head(discovered_devices);

  int i = 0;
  while (device != NULL) {
    if (index == i) {
      return device->device_serial_number;
    }
    i++;
    device = device->next;
  }
  return " empty ";
}

/**
 * function to retrieve the amount of discovered devices
 *
 */
int
ets_get_nr_devices(void)
{
  return (oc_list_length(discovered_devices));
}

/**
 * function to reset the owned device
 *
 */
void
ets_reset_device(char *sn)
{
  device_handle_t *device = ets_getdevice_from_sn(sn);

  if (device == NULL) {
    PRINT("[C]ERROR: Invalid sn\n");
    return;
  }

  // ets_mutex_lock(app_sync_lock);
  // int ret = oc_obt_device_hard_reset(&device->uuid, reset_device_cb, device);
  // if (ret >= 0) {
  //  PRINT("[C]\nSuccessfully issued request to perform hard RESET\n");
  //} else {
  //  PRINT("[C]\nERROR issuing request to perform hard RESET\n");
  // }
  // ets_mutex_unlock(app_sync_lock);
}

// -----------------------------------------------------------------------------
void
ets_reset_ets()
{
  PRINT("[C] ets_reset_ets: resetting device\n");
  oc_knx_device_storage_reset(0, 2);
}

int
ets_start(char *serial_number)
{

  strncpy(g_serial_number, serial_number, MAX_SERIAL_NUM_LENGTH);
  memset(&my_CBFunctions, 0, sizeof(my_CBFunctions));

  static const oc_handler_t handler = { .init = app_init,
                                        .signal_event_loop = signal_event_loop,
#ifdef OC_SERVER
                                        .register_resources = NULL,
#endif
#ifdef OC_CLIENT
                                        .requests_entry = NULL
#endif
  };

#ifdef OC_STORAGE
  oc_storage_config("./ets_creds");
#endif /* OC_STORAGE */
       // oc_set_factory_presets_cb(factory_presets_cb, NULL);
  oc_set_max_app_data_size(16384);

  oc_set_spake_response_cb(spake_callback);

  int init = oc_main_init(&handler);

#if defined(_WIN32)
  InitializeCriticalSection(&cs);
  InitializeConditionVariable(&cv);
  InitializeCriticalSection(&app_sync_lock);
#elif defined(__linux__)
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = ets_exit;
  sigaction(SIGINT, &sa, NULL);
#endif

  return init;
}

int
ets_stop(void)
{

  /* Free all device_handle_t objects allocated by this application */
  device_handle_t *device = (device_handle_t *)oc_list_pop(discovered_devices);
  while (device) {
    oc_memb_free(&device_handles, device);
    device = (device_handle_t *)oc_list_pop(discovered_devices);
  }
  return 0;
}

int
ets_poll(void)
{
  oc_clock_time_t next_event;
  next_event = oc_main_poll();
  signal_event_loop();

  // ets_mutex_lock(app_sync_lock);
  // next_event = oc_main_poll();
  // ets_mutex_unlock(app_sync_lock);

  // PRINT("blah");
  // PRINT("    ---> %d", next_event);

  return 0;
}

// -----------------------------------------------------------------------------

/**
 * event loop (window/linux) used for the python initiated thread.
 *
 */
static void
signal_event_loop(void)
{
#if defined(_WIN32)
  WakeConditionVariable(&cv);
#elif defined(__linux__)
  ets_mutex_lock(mutex);
  pthread_cond_signal(&cv);
  ets_mutex_unlock(mutex);
#endif
}

/**
 * function to quit the event loop
 *
 */
void
ets_exit(int signal)
{
  (void)signal;
  quit = 1;
  signal_event_loop();
}

/**
 * the event thread (windows or Linux)
 *
 */
#if defined(_WIN32)
DWORD WINAPI
func_event_thread(LPVOID lpParam)
{
  oc_clock_time_t next_event;
  while (quit != 1) {
    ets_mutex_lock(app_sync_lock);
    next_event = oc_main_poll();
    ets_mutex_unlock(app_sync_lock);

    if (next_event == 0) {
      SleepConditionVariableCS(&cv, &cs, INFINITE);
    } else {
      oc_clock_time_t now = oc_clock_time();
      if (now < next_event) {
        SleepConditionVariableCS(
          &cv, &cs, (DWORD)((next_event - now) * 1000 / OC_CLOCK_SECOND));
      }
    }
  }

  oc_main_shutdown();
  return TRUE;
}
#elif defined(__linux__)
static void *
func_event_thread(void *data)
{
  (void)data;
  oc_clock_time_t next_event;
  while (quit != 1) {
    ets_mutex_lock(app_sync_lock);
    next_event = oc_main_poll();
    ets_mutex_unlock(app_sync_lock);

    ets_mutex_lock(mutex);
    if (next_event == 0) {
      pthread_cond_wait(&cv, &mutex);
    } else {
      ts.tv_sec = (next_event / OC_CLOCK_SECOND);
      ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
      pthread_cond_timedwait(&cv, &mutex, &ts);
    }
    ets_mutex_unlock(mutex);
  }
  oc_main_shutdown();
  return NULL;
}
#endif

int
ets_main(void)
{
#if defined(_WIN32)
  InitializeCriticalSection(&cs);
  InitializeConditionVariable(&cv);
  InitializeCriticalSection(&app_sync_lock);
#elif defined(__linux__)
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = ets_exit;
  sigaction(SIGINT, &sa, NULL);
#endif

#ifdef OC_SERVER
  PRINT("[C]OC_SERVER\n");
#endif
#ifdef OC_CLIENT
  PRINT("[C]OC_CLIENT\n");
#endif
#ifdef OC_OSCORE
  PRINT("[C]OC_OSCORE\n");
#else
  PRINT("[C] NO OC_OSCORE ---\n");
#endif

  int init;

  strncpy(g_serial_number, "01234", MAX_SERIAL_NUM_LENGTH);

  static const oc_handler_t handler = { .init = app_init,
                                        .signal_event_loop = signal_event_loop,
#ifdef OC_SERVER
                                        .register_resources = NULL,
#endif
#ifdef OC_CLIENT
                                        .requests_entry = NULL
#endif
  };

#ifdef OC_STORAGE
  oc_storage_config("./ets_creds");
#endif /* OC_STORAGE */
       // oc_set_factory_presets_cb(factory_presets_cb, NULL);
  oc_set_max_app_data_size(16384);

  oc_set_spake_response_cb(spake_callback);

  init = oc_main_init(&handler);
  if (init < 0)
    return init;

#if defined(_WIN32)
  event_thread = CreateThread(
    NULL, 0, (LPTHREAD_START_ROUTINE)func_event_thread, NULL, 0, NULL);
  if (NULL == event_thread) {
    return -1;
  }
#elif defined(__linux__)
  if (pthread_create(&event_thread, NULL, &func_event_thread, NULL) != 0) {
    return -1;
  }
#endif

  while (quit != 1) {
#if defined(__linux__)
    sleep(5);
#endif

#if defined(_WIN32)
    Sleep(5);
#endif
  }

#if defined(_WIN32)
  WaitForSingleObject(event_thread, INFINITE);
#elif defined(__linux__)
  pthread_join(event_thread, NULL);
#endif

  /* Free all device_handle_t objects allocated by this application */
  device_handle_t *device = (device_handle_t *)oc_list_pop(discovered_devices);
  while (device) {
    oc_memb_free(&device_handles, device);
    device = (device_handle_t *)oc_list_pop(discovered_devices);
  }

  return 0;
}

// -----------------------------------------------------------------------------

long
ets_get_max_app_data_size(void)
{
  return oc_get_max_app_data_size();
}

void
ets_set_max_app_data_size(int data_size)
{
  oc_set_max_app_data_size((size_t)data_size);
}