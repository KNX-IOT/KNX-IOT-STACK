/*
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Copyright (c) 2021-2022 Cascoda Ltd
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/
#ifndef DOXYGEN
// Force doxygen to document static inline
#define STATIC static
#endif

/**
 * @brief Example device implementing Function Block LSAB
 * @file
 *  Example code for Function Block LSAB.
 *  Implements data point 61: switch on/off.
 *  This implementation is a actuator, e.g. receives data and acts on it.
 *
 * ## Application Design
 *
 * Support functions:
 *
 * - app_init:
 *   - initializes the stack values.
 * - register_resources:
 *   - function that registers all endpoints,
 *   - sets the GET/PUT/POST/DELETE handlers for each end point
 *
 * - main:
 *   - starts the stack, with the registered resources.
 *   - can be compiled out with NO_MAIN
 *
 *  Handlers for the implemented methods (get/post):
 *   - get_[path] :
 *     - function that is being called when a GET is called on [path]
 *     - set the global variables in the output
 *   - post_[path] :
 *     - function that is being called when a POST is called on [path]
 *     - checks the input data
 *     - if input data is correct
 *        - updates the global variables
 *
 * ## Defines
 * stack specific defines:
 * - __linux__
 *   build for Linux
 * - WIN32
 *   build for windows
 * - OC_OSCORE
 *   oscore is enabled as compile flag
 *
 * File specific defines:
 * - NO_MAIN
 *   compile out the function main()
 * - INCLUDE_EXTERNAL
 *   includes header file "external_header.h", so that other tools/dependencies
 *   can be included without changing this code
 */

#define _WIN32_WINNT 0x8000
#include "oc_api.h"
#include "oc_core_res.h"
#include "api/oc_knx_fp.h"
#include "port/oc_clock.h"
#include <signal.h>
// test purpose only
#include "api/oc_knx_dev.h"
#ifdef OC_SPAKE
#include "security/oc_spake2plus.h"
#endif
#ifdef INCLUDE_EXTERNAL
/* import external definitions from header file*/
/* this file should be externally supplied */
#include "external_header.h"
#endif

#ifdef __linux__
/** linux specific code */
#include <pthread.h>
#ifndef NO_MAIN
static pthread_mutex_t mutex;
static pthread_cond_t cv;
static struct timespec ts;
#endif /* NO_MAIN */
#endif

#include <stdio.h> /* defines FILENAME_MAX */

#define MY_NAME "Actuator (LSAB) 417" /**< The name of the application */

#ifdef WIN32
/** windows specific code */
#include <windows.h>
STATIC CONDITION_VARIABLE cv; /**< event loop variable */
STATIC CRITICAL_SECTION cs;   /**< event loop variable */
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#define btoa(x) ((x) ? "true" : "false")

bool g_mystate = false; /**< the state of the dpa 417.61 */
volatile int quit = 0;  /**< stop variable, used by handle_signal */
bool g_reset = false;   /**< reset the device (from startup) */

/**
 *  @brief function to set up the device.
 *
 * sets the:
 * - serial number
 * - base path
 * - knx spec version
 * - hardware version
 * - firmware version
 * - hardware type
 * - device model
 *
 */
int
app_init(void)
{
  /* set the manufacturer name */
  int ret = oc_init_platform("Cascoda", NULL, NULL);

  /* set the application name, version, base url, device serial number */
  ret |= oc_add_device(MY_NAME, "1.0.0", "//", "00FA10010701", NULL, NULL);

  oc_device_info_t *device = oc_core_get_device_info(0);
  PRINT("Serial Number: %s\n", oc_string_checked(device->serialnumber));

  /* set the hardware version 1.0.0 */
  oc_core_set_device_hwv(0, 1, 0, 0);

  /* set the firmware version 1.0.0 */
  oc_core_set_device_fwv(0, 1, 0, 0);

  /* set the hardware type*/
  oc_core_set_device_hwt(0, "Pi");

  /* set the application info*/
  oc_core_set_device_ap(0, 1, 0, 0);

  /* set the manufacturer info*/
  oc_core_set_device_mid(0, 12);

  /* set the model */
  oc_core_set_device_model(0, "Cascoda Actuator");

#ifdef OC_SPAKE
#define PASSWORD "LETTUCE"
  oc_spake_set_password(PASSWORD);
  PRINT(" SPAKE password %s\n", PASSWORD);
#endif

  return ret;
}

/**
 * get method for "p/o_1_1" resource.
 * function is called to initialize the return values of the GET method.
 * initialization of the returned values are done from the global property
 * values. Resource Description: This Resource describes a binary switch
 * (on/off). The Property "value" is a boolean. A value of 'true' means that the
 * switch is on. A value of 'false' means that the switch is off.
 *
 * @param request the request representation.
 * @param interfaces the interface used for this call
 * @param user_data the user data.
 */
STATIC void
get_o_1_1(oc_request_t *request, oc_interface_mask_t interfaces,
          void *user_data)
{
  (void)user_data; /* variable not used */

  /* TODO: SENSOR add here the code to talk to the HW if one implements a
     sensor. the call to the HW needs to fill in the global variable before it
     returns to this function here. alternative is to have a callback from the
     hardware that sets the global variables.
  */
  bool error_state = false; /* the error state, the generated code */
  int oc_status_code = OC_STATUS_OK;

  PRINT("-- Begin get_dpa_417_61: interface %d\n", interfaces);
  /* check if the accept header is CBOR */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
    return;
  }

  // check the query parameter m with the various values
  char *m;
  char *m_key;
  size_t m_key_len;
  size_t m_len = (int)oc_get_query_value(request, "m", &m);
  if (m_len != -1) {
    PRINT("  Query param: %.*s", (int)m_len, m);
    oc_init_query_iterator();
    size_t device_index = request->resource->device;
    oc_device_info_t *device = oc_core_get_device_info(device_index);
    if (device != NULL) {
      oc_rep_begin_root_object();
      while (oc_iterate_query(request, &m_key, &m_key_len, &m, &m_len) != -1) {
        // unique identifier
        if ((strncmp(m, "id", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          char mystring[100];
          snprintf(mystring, 99, "urn:knx:sn:%s%s",
                   oc_string(device->serialnumber),
                   oc_string(request->resource->uri));
          oc_rep_i_set_text_string(root, 9, mystring);
        }
        // resource types
        if ((strncmp(m, "rt", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          oc_rep_set_text_string(root, rt, "urn:knx:dpa.417.61");
        }
        // interfaces
        if ((strncmp(m, "if", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          oc_rep_set_text_string(root, if, "if.s");
        }
        if ((strncmp(m, "dpt", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          oc_rep_set_text_string(root, dpt, oc_string(request->resource->dpt));
        }
        // ga
        if ((strncmp(m, "ga", m_len) == 0) | (strncmp(m, "*", m_len) == 0)) {
          int index = oc_core_find_group_object_table_url(
            oc_string(request->resource->uri));
          if (index > -1) {
            oc_group_object_table_t *got_table_entry =
              oc_core_get_group_object_table_entry(index);
            if (got_table_entry) {
              oc_rep_set_int_array(root, ga, got_table_entry->ga,
                                   got_table_entry->ga_len);
            }
          }
        }
      } /* query iterator */
      oc_rep_end_root_object();
    } else {
      /* device is NULL */
      oc_send_cbor_response(request, OC_STATUS_BAD_OPTION);
    }
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  CborError error;
  oc_rep_begin_root_object();
  oc_rep_i_set_boolean(root, 1, g_mystate);
  oc_rep_end_root_object();
  error = g_err;

  if (error) {
    oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());

  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_417_61\n");
}

/**
 * Put method for "p/o_1_1" resource.
 * The function has as input the request body, which are the input values of the
 * PUT method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 * values. Resource Description:
 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
STATIC void
put_o_1_1(oc_request_t *request, oc_interface_mask_t interfaces,
          void *user_data)
{
  (void)interfaces;
  (void)user_data;
  bool error_state = false;
  PRINT("-- Begin put_dpa_417_61:\n");

  oc_rep_t *rep = NULL;
  // handle the different requests
  if (oc_is_redirected_request(request)) {
    PRINT(" S-MODE or /P\n");
  }
  rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_BOOL) {
      if (rep->iname == 1) {
        PRINT("  put_dpa_417_61 received : %d\n", rep->value.boolean);
        g_mystate = rep->value.boolean;
        oc_send_cbor_response(request, OC_STATUS_CHANGED);
        PRINT("-- End put_dpa_417_61\n");
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_response(request, OC_STATUS_BAD_REQUEST);
  PRINT("-- End put_dpa_417_61\n");
}

/**
 * register all the resources to the stack
 * this function registers all application level resources:
 * - each resource path is bind to a specific function for the supported methods
 * (GET, POST, PUT, DELETE)
 * - each resource is:
 *   - secure
 *   - observable
 *   - discoverable
 *   - used interfaces
 *
 * URL Table
 * | resource url |  functional block/dpa  | GET | PUT |
 * | ------------ | ---------------------- | ----| ---- |
 * | p/o_1_1      | urn:knx:dpa.417.61     | Yes | Yes  |
 */
void
register_resources(void)
{
  PRINT("Register Resource with local path \"/p/o_1_1\"\n");
  PRINT("Light Switching actuator 417 (LSAB) : SwitchOnOff \n");
  PRINT("Data point 417.61 (DPT_Switch) \n");
  oc_resource_t *res_light =
    oc_new_resource("light actuation", "/p/o_1_1", 2, 0);
  oc_resource_bind_resource_type(res_light, "urn:knx:dpa.417.61");
  oc_resource_bind_dpt(res_light, "urn:knx:dpt.Switch");
  oc_resource_bind_content_type(res_light, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_light, OC_IF_A); /* if.a */
  oc_resource_set_discoverable(res_light, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  oc_resource_set_periodic_observable(res_light, 1);
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  // oc_resource_set_observable(res_352, true);
  // set the GET handler
  oc_resource_set_request_handler(res_light, OC_GET, get_o_1_1, NULL);
  // set the PUT handler
  oc_resource_set_request_handler(res_light, OC_PUT, put_o_1_1, NULL);
  // register this resource,
  // this means that the resource will be listed in /.well-known/core
  oc_add_resource(res_light);
}

/**
 * @brief initiate preset for device
 * current implementation: device reset as command line argument
 * @param device the device identifier of the list of devices
 * @param data the supplied data.
 */
void
factory_presets_cb(size_t device, void *data)
{
  (void)device;
  (void)data;

  if (g_reset) {
    PRINT("factory_presets_cb: resetting device\n");
    oc_knx_device_storage_reset(0, 2);
  }
}

/**
 * @brief application reset
 *
 * @param device_index the device identifier of the list of devices
 * @param reset_value the knx reset value
 * @param data the supplied data.
 */
void
reset_cb(size_t device_index, int reset_value, void *data)
{
  (void)device_index;
  (void)data;

  PRINT("reset_cb %d\n", reset_value);
}

/**
 * @brief restart the device (application depended)
 *
 * @param device_index the device identifier of the list of devices
 * @param data the supplied data.
 */
void
restart_cb(size_t device_index, void *data)
{
  (void)device_index;
  (void)data;

  PRINT("-----restart_cb -------\n");
  // exit(0);
}

/**
 * @brief set the host name on the device (application depended)
 *
 * @param device_index the device identifier of the list of devices
 * @param host_name the host name to be set on the device
 * @param data the supplied data.
 */
void
hostname_cb(size_t device_index, oc_string_t host_name, void *data)
{
  (void)device_index;
  (void)data;

  PRINT("-----host name ------- %s\n", oc_string_checked(host_name));
}

static oc_event_callback_retval_t
send_delayed_response(void *context)
{
  oc_separate_response_t *response = (oc_separate_response_t *)context;

  if (response->active) {
    oc_set_separate_response_buffer(response);
    oc_send_separate_response(response, OC_STATUS_CHANGED);
    PRINT("Delayed response sent\n");
  } else {
    PRINT("Delayed response NOT active\n");
  }

  return OC_EVENT_DONE;
}

/**
 * @brief software update callback
 *
 * @param device the device index
 * @param response the instance of an internal struct that is used to track
 *      	         the state of the separate response
 * @param binary_size the full size of the binary
 * @param offset the offset of the image
 * @param payload the image data
 * @param len the length of the image data
 * @param data the user data
 */
void
swu_cb(size_t device, oc_separate_response_t *response, size_t binary_size,
       size_t offset, uint8_t *payload, size_t len, void *data)
{
  (void)device;
  (void)binary_size;
  char filename[] = "./downloaded.bin";
  PRINT(" swu_cb %s block=%d size=%d \n", filename, (int)offset, (int)len);

  FILE *write_ptr = fopen("downloaded_bin", "ab");
  size_t r = fwrite(payload, sizeof(*payload), len, write_ptr);
  fclose(write_ptr);

  oc_set_delayed_callback(response, &send_delayed_response, 0);
}

/**
 * @brief initializes the global variables
 * registers and starts the handler
 */
void
initialize_variables(void)
{
  /* initialize global variables for resources */
}

#ifndef NO_MAIN
#ifdef WIN32
/**
 * signal the event loop (windows version)
 * wakes up the main function to handle the next callback
 */
STATIC void
signal_event_loop(void)
{
  WakeConditionVariable(&cv);
}
#endif /* WIN32 */

#ifdef __linux__
/**
 * signal the event loop (Linux)
 * wakes up the main function to handle the next callback
 */
STATIC void
signal_event_loop(void)
{
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mutex);
}
#endif /* __linux__ */

/**
 * handle Ctrl-C
 * @param signal the captured signal
 */
void
handle_signal(int signal)
{
  (void)signal;
  signal_event_loop();
  quit = 1;
}

/**
 * @brief print usage and quits
 *
 */
void
print_usage()
{
  PRINT("Usage:\n");
  PRINT("no arguments : starts the server\n");
  PRINT("-help  : this message\n");
  PRINT("reset : does an full reset of the device\n");
  exit(0);
}

/**
 * @brief main application.
 * initializes the global variables
 * registers and starts the handler
 * handles (in a loop) the next event.
 * shuts down the stack
 */
int
main(int argc, char *argv[])
{
  int init;
  oc_clock_time_t next_event;
  char *fname = "my_software_image";

#ifdef WIN32
  /* windows specific */
  InitializeCriticalSection(&cs);
  InitializeConditionVariable(&cv);
  /* install Ctrl-C */
  signal(SIGINT, handle_signal);
#endif
#ifdef __linux__
  /* Linux specific */
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_signal;
  /* install Ctrl-C */
  sigaction(SIGINT, &sa, NULL);
#endif

  for (int i = 0; i < argc; i++) {
    PRINT("argv[%d] = %s\n", i, argv[i]);
  }
  if (argc > 1) {
    if (strcmp(argv[1], "reset") == 0) {
      PRINT(" internal reset\n");
      g_reset = true;
    }
    if (strcmp(argv[1], "-help") == 0) {
      print_usage();
    }
  }

  PRINT("KNX-IOT Server name : \"%s\"\n", MY_NAME);

  char buff[FILENAME_MAX];
  char *retbuf = NULL;
  retbuf = GetCurrentDir(buff, FILENAME_MAX);
  if (retbuf != NULL) {
    PRINT("Current working dir: %s\n", buff);
  }

  /*
   The storage folder depends on the build system
   the folder is created in the makefile, with $target as name with _cred as
   post fix.
  */
  PRINT("\tstorage at './LSAB_minimal_creds' \n");
  oc_storage_config("./LSAB_minimal_creds");

  /*initialize the variables */
  initialize_variables();

  /* initializes the handlers structure */
  STATIC const oc_handler_t handler = { .init = app_init,
                                        .signal_event_loop = signal_event_loop,
                                        .register_resources = register_resources
#ifdef OC_CLIENT
                                        ,
                                        .requests_entry = 0
#endif
  };

  /* set the application callbacks */
  oc_set_hostname_cb(hostname_cb, NULL);
  oc_set_reset_cb(reset_cb, NULL);
  oc_set_restart_cb(restart_cb, NULL);
  oc_set_factory_presets_cb(factory_presets_cb, NULL);
  oc_set_swu_cb(swu_cb, (void *)fname);

  /* start the stack */
  init = oc_main_init(&handler);

  if (init < 0) {
    PRINT("oc_main_init failed %d, exiting.\n", init);
    return init;
  }
#ifdef OC_OSCORE
  PRINT("OSCORE - Enabled\n");
#else
  PRINT("OSCORE - Disabled\n");
#endif /* OC_OSCORE */

  oc_device_info_t *device = oc_core_get_device_info(0);
  PRINT("serial number: %s\n", oc_string_checked(device->serialnumber));

  oc_endpoint_t *my_ep = oc_connectivity_get_endpoints(0);
  if (my_ep != NULL) {
    PRINTipaddr(*my_ep);
    PRINT("\n");
  }
  PRINT("Server \"%s\" running, waiting on incoming "
        "connections.\n",
        MY_NAME);

#ifdef WIN32
  /* windows specific loop */
  while (quit != 1) {
    next_event = oc_main_poll();
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
#endif

#ifdef __linux__
  /* Linux specific loop */
  while (quit != 1) {
    next_event = oc_main_poll();
    pthread_mutex_lock(&mutex);
    if (next_event == 0) {
      pthread_cond_wait(&cv, &mutex);
    } else {
      ts.tv_sec = (next_event / OC_CLOCK_SECOND);
      ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
      pthread_cond_timedwait(&cv, &mutex, &ts);
    }
    pthread_mutex_unlock(&mutex);
  }
#endif

  /* shut down the stack */

  oc_main_shutdown();
  return 0;
}
#endif /* NO_MAIN */
