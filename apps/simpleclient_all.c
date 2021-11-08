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
  int ret = oc_init_platform("Apple", NULL, NULL);
  ret |= oc_add_device("/oic/d", "oic.d.phone", "Kishen's IPhone", "ocf.1.0.0",
                       "ocf.res.1.0.0", NULL, NULL);
  return ret;
}

#define MAX_URI_LENGTH (30)
static char a_light[MAX_URI_LENGTH];
static oc_endpoint_t *light_server;

static bool state;
static int power;
static oc_string_t name;

oc_event_callback_retval_t
stop_observe(void *data)
{
  (void)data;
  PRINT("Stopping OBSERVE\n");
  oc_stop_observe(a_light, light_server);
  return OC_EVENT_DONE;
}

void
observe_light(oc_client_response_t *data)
{
  PRINT("OBSERVE_light:\n");
  oc_rep_t *rep = data->payload;
  while (rep != NULL) {
    PRINT("key %s, value ", oc_string(rep->name));
    switch (rep->type) {
    case OC_REP_BOOL:
      PRINT("%d\n", rep->value.boolean);
      state = rep->value.boolean;
      break;
    case OC_REP_INT:
      PRINT("%lld\n", rep->value.integer);
      power = (int)rep->value.integer;
      break;
    case OC_REP_STRING:
      PRINT("%s\n", oc_string(rep->value.string));
      oc_free_string(&name);
      oc_new_string(&name, oc_string(rep->value.string),
                    oc_string_len(rep->value.string));
      break;
    default:
      break;
    }
    rep = rep->next;
  }
}

void
post2_light(oc_client_response_t *data)
{
  PRINT("POST2_light:\n");
  if (data->code == OC_STATUS_CHANGED)
    PRINT("POST response: CHANGED\n");
  else if (data->code == OC_STATUS_CREATED)
    PRINT("POST response: CREATED\n");
  else
    PRINT("POST response code %d\n", data->code);

  oc_do_observe(a_light, light_server, NULL, &observe_light, LOW_QOS, NULL);
  oc_set_delayed_callback(NULL, &stop_observe, 30);
  PRINT("Sent OBSERVE request\n");
}

void
post_light(oc_client_response_t *data)
{
  PRINT("POST_light:\n");
  if (data->code == OC_STATUS_CHANGED)
    PRINT("POST response: CHANGED\n");
  else if (data->code == OC_STATUS_CREATED)
    PRINT("POST response: CREATED\n");
  else
    PRINT("POST response code %d\n", data->code);

  if (oc_init_post(a_light, light_server, NULL, &post2_light, LOW_QOS, NULL)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, true);
    oc_rep_set_int(root, power, 55);
    oc_rep_end_root_object();
    if (oc_do_post())
      PRINT("Sent POST request\n");
    else
      PRINT("Could not send POST request\n");
  } else
    PRINT("Could not init POST request\n");
}

void
put_light(oc_client_response_t *data)
{
  PRINT("PUT_light:\n");

  if (data->code == OC_STATUS_CHANGED)
    PRINT("PUT response: CHANGED\n");
  else
    PRINT("PUT response code %d\n", data->code);

  if (oc_init_post(a_light, light_server, NULL, &post_light, LOW_QOS, NULL)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, false);
    oc_rep_set_int(root, power, 105);
    oc_rep_end_root_object();
    if (oc_do_post())
      PRINT("Sent POST request\n");
    else
      PRINT("Could not send POST request\n");
  } else
    PRINT("Could not init POST request\n");
}

void
get_light(oc_client_response_t *data)
{
  PRINT("GET_light:\n");
  oc_rep_t *rep = data->payload;
  while (rep != NULL) {
    PRINT("key %s, value ", oc_string(rep->name));
    switch (rep->type) {
    case OC_REP_BOOL:
      PRINT("%d\n", rep->value.boolean);
      state = rep->value.boolean;
      break;
    case OC_REP_INT:
      PRINT("%lld\n", rep->value.integer);
      power = (int)rep->value.integer;
      break;
    case OC_REP_STRING:
      PRINT("%s\n", oc_string(rep->value.string));
      oc_free_string(&name);
      oc_new_string(&name, oc_string(rep->value.string),
                    oc_string_len(rep->value.string));
      break;
    default:
      break;
    }
    rep = rep->next;
  }

  if (oc_init_put(a_light, light_server, NULL, &put_light, LOW_QOS, NULL)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, true);
    oc_rep_set_int(root, power, 15);
    oc_rep_end_root_object();

    if (oc_do_put())
      PRINT("Sent PUT request\n");
    else
      PRINT("Could not send PUT request\n");
  } else
    PRINT("Could not init PUT request\n");
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

    oc_lf_get_entry_param(payload, len, i, "rt", &param, &param_len);
    PRINT(" DISCOVERY RT %.*s\n", param_len, param);

    oc_lf_get_entry_param(payload, len, i, "if", &param, &param_len);
    PRINT(" DISCOVERY IF %.*s\n", param_len, param);

    oc_lf_get_entry_param(payload, len, i, "ct", &param, &param_len);
    PRINT(" DISCOVERY CT %.*s\n", param_len, param);
  }

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

  oc_free_server_endpoints(light_server);
  oc_free_string(&name);
  oc_main_shutdown();
  return 0;
}
