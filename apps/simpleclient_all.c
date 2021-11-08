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

#include "oc_api.h"
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
  ret |= oc_add_device("/oic/d", "oic.d.phone", "Control Application",
                       "ocf.1.0.0", "ocf.res.1.0.0", NULL, NULL);
  return ret;
}

static oc_endpoint_t *the_server;

void
put_dev_pm(oc_client_response_t *data)
{
  PRINT("put_dev_pm:\n");
  PRINT("  content format %d\n", data->content_format);

  oc_rep_t *rep = data->payload;

  if ((rep != NULL) && (rep->type == OC_REP_BOOL)) {
    PRINT("  put_dev_pm received : %d\n", rep->value.boolean);
  }
}

void
get_dev_pm(oc_client_response_t *data)
{
  PRINT("get_dev_pm:\n");

  PRINT("  content format %d\n", data->content_format);

  oc_rep_t *rep = data->payload;
  
  if ((rep != NULL) && (rep->type == OC_REP_BOOL)) {
    PRINT("  get_dev_pm received : %d\n", rep->value.boolean);
  }


  if (oc_init_put("/dev/pm", data->endpoint, NULL, &put_dev_pm, HIGH_QOS,
                  NULL)) {

    cbor_encode_boolean(&g_encoder, true);

    if (oc_do_put_ex(APPLICATION_CBOR, APPLICATION_CBOR))
      PRINT("  Sent PUT request\n");
    else
      PRINT("  Could not send PUT request\n");
  }
}

void
get_dev(oc_client_response_t *data)
{
  PRINT("\nGET_DEV:\n");

  PRINT("  content format %d\n", data->content_format);

  PRINT("%.*s\n", (int)data->_payload_len, data->_payload);

  oc_do_get_ex("/dev/pm", data->endpoint, NULL, &get_dev_pm, HIGH_QOS,
               APPLICATION_CBOR, APPLICATION_CBOR, NULL);
}

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

  oc_do_get_ex("/dev", endpoint, NULL, &get_dev, HIGH_QOS,
               APPLICATION_LINK_FORMAT, APPLICATION_LINK_FORMAT, NULL);

  PRINT(" DISCOVERY- END\n");
  return OC_STOP_DISCOVERY;
}

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

int
main(void)
{
  int init;

  PRINT("Simple Client:\n");

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

  static const oc_handler_t handler = { .init = app_init,
                                        .signal_event_loop = signal_event_loop,
                                        .requests_entry = issue_requests };

  oc_clock_time_t next_event;

#ifdef OC_STORAGE
  oc_storage_config("./simpleclient_creds");
#endif /* OC_STORAGE */

  init = oc_main_init(&handler);
  if (init < 0)
    return init;

#ifdef OC_SECURITY
  PRINT("Security - Enabled\n");
#else
  PRINT("Security - Disabled\n");
#endif /* OC_SECURITY */

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
