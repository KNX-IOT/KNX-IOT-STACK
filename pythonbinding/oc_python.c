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

/* Structure in app to track currently discovered owned/unowned devices */

typedef struct device_handle_t
{
  struct device_handle_t *next;
  char device_serial_number[10];
  char device_name[64];
  char ip_address[100];
  oc_endpoint_t ep;
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
#define py_mutex_lock(m) EnterCriticalSection(&m)
#define py_mutex_unlock(m) LeaveCriticalSection(&m)

#elif defined(__linux__)
static pthread_t event_thread;
static pthread_mutex_t app_sync_lock;
static pthread_mutex_t mutex;
static pthread_cond_t cv;

/* OS specific definition for lock/unlock */
#define py_mutex_lock(m) pthread_mutex_lock(&m)
#define py_mutex_unlock(m) pthread_mutex_unlock(&m)

static struct timespec ts;
#endif
static int quit = 0;

/**
 * structure with the callbacks
 *
 */
typedef struct py_cb_struct
{
  changedCB changedFCB;
  resourceCB resourceFCB;
  clientCB clientFCB;
  discoveryCB discoveryFCB;
  spakeCB spakeFCB;
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
  return strings[code];
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
 * function to install callbacks, called from python
 *
 */
void
py_install_changedCB(changedCB changedCB)
{
  PRINT("[C]install_changedCB\n");
  my_CBFunctions.changedFCB = changedCB;
}

/**
 * function to install resource callbacks, called from python
 *
 */
void
py_install_resourceCB(resourceCB resourceCB)
{
  PRINT("[C]install_resourceCB\n");
  my_CBFunctions.resourceFCB = resourceCB;
}
/**
 * function to install client callbacks, called from python
 *
 */
void
py_install_clientCB(clientCB clientCB)
{
  PRINT("[C]install_clientCB\n");
  my_CBFunctions.clientFCB = clientCB;
}

/**
 * function to install discovery callbacks, called from python
 *
 */
void
py_install_discoveryCB(discoveryCB discoveryCB)
{
  PRINT("[C]installdiscoveryCB\n");
  my_CBFunctions.discoveryFCB = discoveryCB;
}

/**
 * function to install spake callbacks, called from python
 *
 */
void
py_install_spakeCB(spakeCB spakeCB)
{
  PRINT("[C]installspakeCB\n");
  my_CBFunctions.spakeFCB = spakeCB;
}

/**
 * function to call the callback to python.
 *
 */
void
inform_python(const char *uuid, const char *state, const char *event)
{
  // PRINT("[C]inform_python %p\n",my_CBFunctions.changedFCB);
  if (my_CBFunctions.changedFCB != NULL) {
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
 * CFUNCTYPE(None, c_int, c_char_p, c_int)
 */
void
inform_spake_python(char *sn, int state, char *key, int key_size)
{
  PRINT("[C]inform_spake_python %p %s %d %d key=[", my_CBFunctions.spakeFCB, sn,
        state, key_size);
  for (int i = 0; i < key_size; i++) {
    PRINT("%02x", (unsigned char)key[i]);
  }
  PRINT("]\n");

  if (my_CBFunctions.spakeFCB != NULL) {
    my_CBFunctions.spakeFCB(sn, state, key, key_size);
  }
}

// -----------------------------------------------------------------------------

/**
 * function to convert the sn to the device handle
 *
 */
device_handle_t *
py_getdevice_from_sn(char *sn)
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

  ret |= oc_add_device("py-client", "1.0.0", "//", "012349", NULL, NULL);

  return ret;
}

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
  py_mutex_lock(mutex);
  pthread_cond_signal(&cv);
  py_mutex_unlock(mutex);
#endif
}

/**
 * function to quit the event loop
 *
 */
void
py_exit(int signal)
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
    py_mutex_lock(app_sync_lock);
    next_event = oc_main_poll();
    py_mutex_unlock(app_sync_lock);

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
    py_mutex_lock(app_sync_lock);
    next_event = oc_main_poll();
    py_mutex_unlock(app_sync_lock);

    py_mutex_lock(mutex);
    if (next_event == 0) {
      pthread_cond_wait(&cv, &mutex);
    } else {
      ts.tv_sec = (next_event / OC_CLOCK_SECOND);
      ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
      pthread_cond_timedwait(&cv, &mutex, &ts);
    }
    py_mutex_unlock(mutex);
  }
  oc_main_shutdown();
  return NULL;
}
#endif

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
      py_mutex_lock(app_sync_lock);
      memset(buffer, 0, buffer_size);
      memcpy(&buffer, (char *)data->_payload, (int)data->_payload_len);

      inform_client_python((char *)my_data->sn, status, "link_format",
                           (char *)my_data->r_id, (char *)my_data->url,
                           (int)data->_payload_len, buffer);
      py_mutex_unlock(app_sync_lock);
    } else if (data->content_format == APPLICATION_CBOR) {
      py_mutex_lock(app_sync_lock);
      memset(buffer, 0, buffer_size);
      int json_size =
        py_oc_rep_to_json(data->payload, (char *)&buffer, buffer_size, false);
      inform_client_python((char *)my_data->sn, status, "json",
                           (char *)my_data->r_id, (char *)my_data->url,
                           (int)json_size, (char *)buffer);
      py_mutex_unlock(app_sync_lock);
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
py_cbor_get(char *sn, char *uri, char *query, char *cbdata)
{
  int ret = -1;
  device_handle_t *device = py_getdevice_from_sn(sn);

  user_struct_t *new_cbdata;
  new_cbdata = (user_struct_t *)malloc(sizeof(user_struct_t));
  if (new_cbdata != NULL) {
    strcpy(new_cbdata->r_id, cbdata);
    strcpy(new_cbdata->url, uri);
    strcpy(new_cbdata->sn, sn);
  }

  PRINT("  [C]py_cbor_get: [%s], [%s] [%s] [%s]\n", sn, uri, query, cbdata);
#ifdef OC_OSCORE
  device->ep.flags += OSCORE;
  PRINT("  [C] enable OSCORE encryption\n");
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
py_cbor_get_unsecured(char *sn, char *uri, char *query, char *cbdata)
{
  int ret = -1;
  device_handle_t *device = py_getdevice_from_sn(sn);

  user_struct_t *new_cbdata;
  new_cbdata = (user_struct_t *)malloc(sizeof(user_struct_t));
  if (new_cbdata != NULL) {
    strcpy(new_cbdata->r_id, cbdata);
    strcpy(new_cbdata->url, uri);
    strcpy(new_cbdata->sn, sn);
  }

  PRINT("  [C]py_cbor_get_unsecured: [%s], [%s] [%s] [%s]\n", sn, uri, query,
        cbdata);

  ret = oc_do_get_ex(uri, &device->ep, query, general_get_cb, HIGH_QOS,
                     APPLICATION_CBOR, APPLICATION_CBOR, new_cbdata);
  if (ret >= 0) {
    PRINT("  [C]Successfully issued GET request\n");
  } else {
    PRINT("  [C]ERROR issuing GET request\n");
  }
}

void
py_linkformat_get(char *sn, char *uri, char *query, char *cbdata)
{
  int ret = -1;
  oc_endpoint_t ep;
  device_handle_t *device = py_getdevice_from_sn(sn);

  PRINT("  [C]py_linkformat_get: [%s], [%s] [%s] [%s]\n", sn, uri, query,
        cbdata);
#ifdef OC_OSCORE
  device->ep.flags += OSCORE;
  PRINT("  [C] enable OSCORE encryption\n");
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
py_linkformat_get_unsecured(char *sn, char *uri, char *query, char *cbdata)
{
  int ret = -1;
  oc_endpoint_t ep;
  device_handle_t *device = py_getdevice_from_sn(sn);

  PRINT("  [C]py_linkformat_get_unsecured: [%s], [%s] [%s] [%s]\n", sn, uri,
        query, cbdata);

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

// -----------------------------------------------------------------------------

void
py_cbor_post(char *sn, char *uri, char *query, char *id, int size, char *data)
{
  int ret = -1;
  device_handle_t *device = py_getdevice_from_sn(sn);

  PRINT("  [C]py_cbor_post: [%s], [%s] [%s] [%s] %d\n", sn, uri, id, query,
        size);
#ifdef OC_OSCORE
  device->ep.flags += OSCORE;
  PRINT("  [C] enable OSCORE encryption\n");
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
py_cbor_put(char *sn, char *uri, char *query, char *id, int size, char *data)
{
  int ret = -1;
  device_handle_t *device = py_getdevice_from_sn(sn);

  PRINT("  [C]py_cbor_put: [%s], [%s] [%s] [%s] %d\n", sn, uri, id, query,
        size);
#ifdef OC_OSCORE
  device->ep.flags += OSCORE;
  PRINT("  [C] enable OSCORE encryption\n");
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
py_cbor_delete(char *sn, char *uri, char *query, char *id)
{
  int ret = -1;
  device_handle_t *device = py_getdevice_from_sn(sn);

  PRINT("  [C]py_cbor_delete: [%s], [%s] [%s] [%s]\n", sn, uri, id, query);
#ifdef OC_OSCORE
  device->ep.flags += OSCORE;
  PRINT("  [C] enable OSCORE encryption\n");
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

  oc_endpoint_to_string(data->endpoint, &my_address);

  if ((rep != NULL) && (rep->type == OC_REP_STRING)) {
    char *my_sn = oc_string(rep->value.string);
    PRINT("[C]  get_sn received %s (address) :%s\n", my_sn,
          oc_string(my_address));

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

  for (int i = 0; i < nr_entries; i++) {

    oc_lf_get_entry_uri(payload, len, i, &uri, &uri_len);
    PRINT("[C] DISCOVERY URL %.*s\n", uri_len, uri);
    // oc_string_to_endpoint()
    oc_lf_get_entry_param(payload, len, i, "rt", &param, &param_len);
    PRINT("[C] DISCOVERY RT %.*s\n", param_len, param);
    oc_lf_get_entry_param(payload, len, i, "if", &param, &param_len);
    PRINT("[C] DISCOVERY IF %.*s\n", param_len, param);
    oc_lf_get_entry_param(payload, len, i, "ct", &param, &param_len);
    PRINT("[C] DISCOVERY CT %.*s\n", param_len, param);
  }

  inform_discovery_python(len, payload);

  PRINT("[C] issue get on /dev/sn\n");
  oc_endpoint_print(endpoint);

  oc_do_get_ex("/dev/sn", endpoint, NULL, response_get_sn, HIGH_QOS,
               APPLICATION_CBOR, APPLICATION_CBOR, endpoint);

  PRINT("[C] DISCOVERY- END\n");
  return OC_CONTINUE_DISCOVERY;
}

void
py_discover_devices(int scope)
{
  py_mutex_lock(app_sync_lock);
  oc_do_wk_discovery_all("rt=urn:knx:dpa.*", scope, discovery_cb, NULL);

  py_mutex_unlock(app_sync_lock);
  signal_event_loop();
}

void
py_discover_devices_with_query(int scope, const char *query)
{
  py_mutex_lock(app_sync_lock);
  oc_do_wk_discovery_all(query, scope, discovery_cb, NULL);

  py_mutex_unlock(app_sync_lock);
  signal_event_loop();
}

// -----------------------------------------------------------------------------

void
py_initate_spake(char *sn, char *password)
{
  int ret = -1;
  device_handle_t *device = py_getdevice_from_sn(sn);

  PRINT("  [C]py_initate_spake: [%s] [%s]\n", sn, password);
  if (oc_string_len(device->ep.serial_number) == 0) {
    oc_new_string(&device->ep.serial_number, sn, strlen(sn));
  }
  ret = oc_initiate_spake(&device->ep, password);
  PRINT("  [C]py_initate_spake: [%d]-- done\n", ret);
  if (ret == -1) {
    // failure, so unblock python
    inform_spake_python(sn, ret, "", 0);
  }
}

void
spake_callback(int error, uint8_t *secret, int secret_size)
{
  inform_spake_python("", error, secret, secret_size);
}

// -----------------------------------------------------------------------------

/* send a multicast s-mode message */
void
py_issue_requests_s_mode(int scope, int sia, int ga, char *st, int value_type,
                         char *value)
{
  PRINT(" [C] py_issue_requests_s_mode\n");

  oc_make_ipv6_endpoint(mcast, IPV6 | DISCOVERY | MULTICAST, 5683, 0xff, scope,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0xfd);

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
      int float_value = atof(value);
      oc_rep_i_set_double(value, 1, (double)float_value);
    }
    cbor_encoder_close_container_checked(&root_map, &value_map);
    oc_rep_end_root_object();

    if (oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
      PRINT("  Sent POST request\n");
    } else {
      PRINT("  Could not send POST request\n");
    }
  }
}

// -----------------------------------------------------------------------------

/**
 * function to retrieve the device name of the owned/unowned device
 *
 */
char *
get_device_name(int index)
{
  device_handle_t *device = NULL;

  device = (device_handle_t *)oc_list_head(discovered_devices);
  int i = 0;
  while (device != NULL) {
    if (index == i) {
      return device->device_name;
    }
    i++;
    device = device->next;
  }
  return " empty ";
}

/**
 * function to retrieve the device name belonging to the sn
 *
 */
char *
get_device_name_from_sn(char *uuid)
{
  device_handle_t *device = NULL;
  device = (device_handle_t *)oc_list_head(discovered_devices);
  int i = 0;
  while (device != NULL) {
    if (strcmp(device->device_serial_number, uuid) == 0) {
      return device->device_name;
    }
    i++;
    device = device->next;
  }
  return " empty ";
}

/**
 * function to retrieve the sn of the discovered device
 *
 */
char *
py_get_sn(int index)
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
 * function to retrieve the number of discovered device
 *
 */
int
py_get_nr_devices(void)
{
  return (oc_list_length(discovered_devices));
}

/**
 * function to reset the owned device
 *
 */
void
py_reset_device(char *sn)
{
  device_handle_t *device = py_getdevice_from_sn(sn);

  if (device == NULL) {
    PRINT("[C]ERROR: Invalid uuid\n");
    return;
  }

  // py_mutex_lock(app_sync_lock);
  // int ret = oc_obt_device_hard_reset(&device->uuid, reset_device_cb, device);
  // if (ret >= 0) {
  //  PRINT("[C]\nSuccessfully issued request to perform hard RESET\n");
  //} else {
  //  PRINT("[C]\nERROR issuing request to perform hard RESET\n");
  // }
  // py_mutex_unlock(app_sync_lock);
}

// void
// display_device_uuid()
//{
//  char buffer[OC_UUID_LEN];
//  oc_uuid_to_str(oc_core_get_device_id(0), buffer, sizeof(buffer));

//  PRINT("[C] Started device with ID: %s\n", buffer);
//}

// char *
// py_get_obt_uuid()
//{
//  char buffer[OC_UUID_LEN];
//  oc_uuid_to_str(oc_core_get_device_id(0), buffer, sizeof(buffer));

//  char *uuid = malloc(sizeof(char) * OC_UUID_LEN);
//  strncpy(uuid, buffer, OC_UUID_LEN);
//  return uuid;
//}

// -----------------------------------------------------------------------------

int
py_main(void)
{
#if defined(_WIN32)
  InitializeCriticalSection(&cs);
  InitializeConditionVariable(&cv);
  InitializeCriticalSection(&app_sync_lock);
#elif defined(__linux__)
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = py_exit;
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
  oc_storage_config("./onboarding_tool_creds");
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
py_get_max_app_data_size(void)
{
  return oc_get_max_app_data_size();
}

void
py_set_max_app_data_size(int data_size)
{
  oc_set_max_app_data_size((size_t)data_size);
}