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
#ifndef DOXYGEN
// Force doxygen to document static inline
#define STATIC static
#endif

/**
 * @file
 *  Demo application; examples for client code
 */
/**
 * ## Application Design
 *
 * support functions:
 *
 * - app_init
 *   initializes the stack values.
 * - register_resources
 *   function that registers all endpoints,
 *   e.g. sets the GET/PUT/POST/DELETE
 *      handlers for each end point
 *  - register client sequence
 *
 * - main
 *   starts the stack, with the registered resources.
 *   can be compiled out with NO_MAIN
 *
 *  handlers for the implemented methods (get/post):
 *   - get_[path]
 *     function that is being called when a GET is called on [path]
 *     set the global variables in the output
 *   - post_[path]
 *     function that is being called when a POST is called on [path]
 *     checks the input data
 *     if input data is correct
 *       updates the global variables
 *
 * ## stack specific defines
 *
 *  - OC_SECURITY
      enable security
 * - __linux__
 *   build for linux
 * - WIN32
 *   build for windows
 *
 * ## File specific defines
 *
 * - NO_MAIN
 *   compile out the function main()
 *
 * # Usage
 * Application can be used in 2 ways:
 * - discovery of resources through well-known/core
 *   this kicks off a sequence of commands (next one triggered on the previous
 response):
 *   - issues a GET on /dev of the discovered device
 *   - issues a PUT on /dev/pm
 *     Note that performing a POST is identical as PUT.
 * - issuing a multicast s-mode commands
 *   issued through all coap nodes/.knx
 *   typical command:
 *
 */

#define _WIN32_WINNT 0x8000
#include "oc_api.h"
#include "oc_knx.h"
#include "api/oc_knx_client.h"
#include "port/oc_clock.h"
#include <signal.h>
#include <stdio.h>

#ifdef __linux__
/** linux specific code */
#include <pthread.h>
#ifndef NO_MAIN
static pthread_mutex_t mutex;
static pthread_cond_t cv;
static struct timespec ts;
#endif /* NO_MAIN */
#endif

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

volatile int quit = 0; /**< stop variable, used by handle_signal */

static int
app_init(void)
{
  int ret = oc_init_platform("Cascoda", NULL, NULL);

  ret |= oc_add_device("my-client", "1.0.0", "//", "000006", NULL, NULL);

  return ret;
}

static oc_endpoint_t *the_server;

void
put_dev_pm(oc_client_response_t *data)
{
  oc_rep_t *rep = data->payload;

  if ((rep != NULL) && (rep->type == OC_REP_BOOL)) {
    PRINT("  put_dev_pm received : %d\n", rep->value.boolean);
  }
}

void
get_dev_pm(oc_client_response_t *data)
{
  PRINT("get_dev_pm:\n");

  oc_rep_t *rep = data->payload;

  if ((rep != NULL) && (rep->type == OC_REP_BOOL)) {
    PRINT("  get_dev_pm received : %d\n", rep->value.boolean);
  }

  if (oc_init_put("/dev/pm", data->endpoint, NULL, &put_dev_pm, HIGH_QOS,
                  NULL)) {

    cbor_encode_boolean(&g_encoder, true);

    if (oc_do_put_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
      PRINT("  Sent PUT request\n");
    } else {
      PRINT("  Could not send PUT request\n");
    }
  }
}
void
callback(oc_client_response_t *rsp)
{
  return;
}

oc_event_callback_retval_t
do_pm(void *ep)
{
  oc_endpoint_t *endpoint = ep;
  endpoint->flags |= SECURED | OSCORE;
  oc_do_get("/dev/pm", endpoint, NULL, callback, HIGH_QOS, NULL);
  return OC_EVENT_CONTINUE;
}

oc_endpoint_t the_endpoint;

static oc_discovery_flags_t
discovery(const char *payload, int len, oc_endpoint_t *endpoint,
          void *user_data)
{
  //(void)anchor;
  (void)user_data;
  (void)endpoint;
  const char *uri;
  int uri_len;
  const char *param;
  int param_len;

  PRINT(" DISCOVERY:\n");
  PRINT("%.*s\n", len, payload);

  int nr_entries = oc_lf_number_of_entries(payload, len);
  PRINT(" entries %d\n", nr_entries);

  for (int i = 0; i < nr_entries; i++) {

    oc_lf_get_entry_uri(payload, len, i, &uri, &uri_len);

    PRINT(" DISCOVERY URL %.*s\n", uri_len, uri);

    // oc_string_to_endpoint()

    oc_lf_get_entry_param(payload, len, i, "rt", &param, &param_len);
    PRINT(" DISCOVERY RT %.*s\n", param_len, param);

    oc_lf_get_entry_param(payload, len, i, "if", &param, &param_len);
    PRINT(" DISCOVERY IF %.*s\n", param_len, param);

    oc_lf_get_entry_param(payload, len, i, "ct", &param, &param_len);
    PRINT(" DISCOVERY CT %.*s\n", param_len, param);
  }

  memcpy(&the_endpoint, endpoint, sizeof(the_endpoint));

  // char sernum[6] = { 0x00, 0xfa, 0x10, 0x01, 0x07, 0x01 };
  // memcpy(&the_endpoint.oscore_id, sernum, 6);
  // the_endpoint.oscore_id_len = 6;

  if (oc_endpoint_set_oscore_id_from_str(&the_endpoint, "00fa10010701") != 0) {
    // PRINT(
    //   "  \n");
    // return;
  }
  // do parameter exchange
  oc_initiate_spake_parameter_request(endpoint, "00FA10010701", "LETTUCE",
                                      "rcpids", strlen("rcpids"));

  oc_set_delayed_callback(&the_endpoint, do_pm, 10);

  PRINT(" DISCOVERY- END\n");
  return OC_STOP_DISCOVERY;
}

/* do normal discovery */
static void
issue_requests(void)
{
  PRINT("Discovering devices:\n");

  oc_do_wk_discovery_all("rt=urn:knx:dpa.*", 0x2, &discovery, NULL);
}

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

void
my_spake_cb(int error, char *sn, char *oscore_id, int oscore_id_size,
            uint8_t *secret, int secret_size)
{
  PRINT("my_spake_cb: SPAKE2+ Handshake Finished! %s\n", sn);
  PRINT("my_spake_cb: code: %d\n", error);
  PRINT("my_spake_cb: Shared Secret: ");
  for (int i = 0; i < secret_size; i++) {
    PRINT("%02x", secret[i]);
  }
  PRINT("\n");
}

void
print_usage()
{
  PRINT("Usage:\n");
  PRINT("none : issue discovery request and perform a GET on /dev/pm and do an "
        "PUT /dev/pm\n");
  PRINT("-help : this message\n");
  exit(0);
}

int
main(int argc, char *argv[])
{
  int init;
  bool do_send_s_mode = false;

  for (int i = 0; i < argc; i++) {
    PRINT("argv[%d] = %s\n", i, argv[i]);
  }
  if (argc > 1) {
    if (strcmp(argv[1], "-help") == 0) {
      print_usage();
    }
  }

  PRINT("Simple Client SPAKE:\n");

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

  // static const oc_handler_t handler = { .init = app_init,
  static oc_handler_t handler = { .init = app_init,
                                  .signal_event_loop = signal_event_loop,
                                  .requests_entry = issue_requests };

  oc_clock_time_t next_event;

#ifdef OC_STORAGE
  oc_storage_config("./simpleclient_all_creds");
#endif /* OC_STORAGE */

  init = oc_main_init(&handler);
  if (init < 0)
    return init;

#ifdef OC_SECURITY
  PRINT("Security - Enabled\n");
#else
  PRINT("Security - Disabled\n");
#endif /* OC_SECURITY */

  oc_set_spake_response_cb(my_spake_cb);

  PRINT("SimpleClient_spake_all running, waiting on incoming "
        "connections.\n");

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

  oc_free_server_endpoints(the_server);
  // oc_free_string(&name);
  oc_main_shutdown();
  return 0;
}
