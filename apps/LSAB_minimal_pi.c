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
 * @file
 *  Example code for Function Block LSAB
 *  Implements only data point 61: switch on/off
 *  This implementation is a actuator, e.g. receives data
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
 *    - OC_PKI
 *      enable use of PKI
 * - __linux__
 *   build for linux
 * - WIN32
 *   build for windows
 *
 * ## File specific defines
 *
 * - NO_MAIN
 *   compile out the function main()
 * - INCLUDE_EXTERNAL
 *   includes header file "external_header.h", so that other tools/dependencies
 can be included without changing this code
 */

#include "oc_api.h"
#include "oc_core_res.h"
#include "port/oc_clock.h"

#include <signal.h>

#include <Python.h>

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
#ifdef OC_SPAKE
#include "security/oc_spake2plus.h"
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

volatile int quit = 0; /**< stop variable, used by handle_signal */

// Python objects used for initialization
PyObject *pModule;

// Python function for turning the backlight of the LCD on or off.
// Takes one boolean argument matching the desired state of the backlight
PyObject *pSetBacklightFunc;
void
python_binding_init(void)
{
  Py_Initialize();

  // Import the Python module that talks to the Displayotron HAT
  PyObject *pName = PyUnicode_DecodeFSDefault("lsab_minimal");

  // Add current directory to Python path, so that we may import the module
  // located in the same folder as the executable
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("import os");
  PyRun_SimpleString("sys.path.append(os.getcwd())");

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if (pModule) {
    // Import the test function, to check the import is successful
    // & that we are importing the right script
    PyObject *pPrintInPythonFunc =
      PyObject_GetAttrString(pModule, "print_in_python");

    if (pPrintInPythonFunc && PyCallable_Check(pPrintInPythonFunc)) {
      // Ensure that the Python embedding is successful
      PyObject *pValue = PyObject_CallObject(pPrintInPythonFunc, NULL);
    }
    // Do not need the test function anymore, so we decrement the reference
    // counter so that the memory for this object can be garbage-collected
    Py_DECREF(pPrintInPythonFunc);

    // Import the rest of the Python API
    pSetBacklightFunc = PyObject_GetAttrString(pModule, "set_backlight");

    // Initialize the state of the LCD
    PyRun_SimpleString("import lsab_minimal");
    PyRun_SimpleString("lsab_minimal.init()");

  } else {
    PyErr_Print();
    fprintf(stderr, "Failed to load lsab_minimal\n");
    fprintf(stderr, "Please ensure that lsab_minimal.py is in the directory "
                    "you are running this executable from!\n");
    exit(1);
  }
}

void
set_backlight(bool value)
{
  if (pSetBacklightFunc && PyCallable_Check(pSetBacklightFunc)) {
    // When called from C, Python functions expect a single tuple argument
    // containing the arguments defined in the API
    PyObject *pArgs = PyTuple_New(1);
    PyObject *pArgTheZeroth = PyBool_FromLong(value);
    PyTuple_SetItem(pArgs, 0, pArgTheZeroth);

    PyObject *pReturn = PyObject_Call(pSetBacklightFunc, pArgs, NULL);
    PyErr_Print();

    // cleanup - must free every temporary Python object
    Py_DECREF(pArgs);
    Py_DECREF(pArgTheZeroth);
    Py_DECREF(pReturn);
  } else {
    PyErr_Print();
    fprintf(stderr, "set_backlight was not called successfully!");
  }
}

/**
 * function to set up the device.
 *
 * sets the:
 * - serial number
 * - friendly device name
 * - spec version
 *
 */
int
app_init(void)
{
  /* set the manufacturer name */
  int ret = oc_init_platform("Cascoda", NULL, NULL);

  /* set the application name, version, base url, device serial number */
  ret |= oc_add_device(MY_NAME, "1.0.0", "//", "000002", NULL, NULL);

  oc_device_info_t *device = oc_core_get_device_info(0);
  PRINT("Serial Number: %s\n", oc_string(device->serialnumber));

  /* set the hardware version 1.0.0 */
  oc_core_set_device_hwv(0, 1, 0, 0);

  /* set the firmware version*/
  oc_core_set_device_fwv(0, 1, 0, 0);

  /* set the hardware type*/
  oc_core_set_device_hwt(0, "Pi");

  /* set the model */
  oc_core_set_device_model(0, "Cascoda Actuator");

#ifdef OC_SPAKE
#define PASSWORD "LETTUCE"
  oc_spake_set_password(PASSWORD);
  PRINT(" SPAKE password %s\n", PASSWORD);
#endif
  return ret;
}

/** the state of the dpa 417.61 */
bool g_mystate = false;

/**
 * get method for "p/1" resource.
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
get_dpa_417_61(oc_request_t *request, oc_interface_mask_t interfaces,
               void *user_data)
{
  (void)user_data; /* variable not used */

  /* TODO: SENSOR add here the code to talk to the HW if one implements a
     sensor. the call to the HW needs to fill in the global variable before it
     returns to this function here. alternative is to have a callback from the
     hardware that sets the global variables.
  */
  bool error_state = false; /**< the error state, the generated code */
  int oc_status_code = OC_STATUS_OK;

  PRINT("-- Begin get_dpa_417_61: interface %d\n", interfaces);
  /* check if the accept header is CBOR */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
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
 * post method for "p/1" resource.
 * The function has as input the request body, which are the input values of the
 * POST method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 * values. Resource Description:
 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
STATIC void
post_dpa_417_61(oc_request_t *request, oc_interface_mask_t interfaces,
                void *user_data)
{
  (void)interfaces;
  (void)user_data;
  bool error_state = false;
  PRINT("-- Begin post_dpa_417_61:\n");

  oc_rep_t *rep = NULL;
  // handle the different requests
  if (oc_is_redirected_request(request)) {
    PRINT(" S-MODE or /P\n");
  }

  rep = request->request_payload;
  char buffer[200];
  memset(buffer, 200, 1);
  oc_rep_to_json(rep, (char *)&buffer, 200, true);
  PRINT("%s", buffer);

  while (rep != NULL) {
    if (rep->type == OC_REP_BOOL) {
      if (rep->iname == 1) {
        PRINT("  post_dpa_417_61 received : %d\n", rep->value.boolean);
        g_mystate = rep->value.boolean;
        set_backlight(g_mystate);
        oc_send_cbor_response(request, OC_STATUS_CHANGED);
        PRINT("-- End post_dpa_417_61\n");
        return;
      }
    }
    rep = rep->next;
  }

  oc_send_response(request, OC_STATUS_BAD_REQUEST);
  PRINT("-- End post_dpa_417_61\n");
}

/**
 * register all the resources to the stack
 * this function registers all application level resources:
 * - each resource path is bind to a specific function for the supported methods
 * (GET, POST, PUT, DELETE)
 * - each resource is
 *   - secure
 *   - observable
 *   - discoverable
 *   - used interfaces, including the default interface.
 *     default interface is the first of the list of interfaces as specified in
 * the input file
 */
void
register_resources(void)
{
  PRINT("Register Resource with local path \"/p/1\"\n");
  PRINT("Light Switching actuator 417 (LSAB) : SwitchOnOff \n");
  PRINT("Data point 417.61 (DPT_Switch) \n");
  oc_resource_t *res_light = oc_new_resource("light actuation", "p/1", 2, 0);
  oc_resource_bind_resource_type(res_light, "urn:knx:dpa.417.61");
  oc_resource_bind_resource_type(res_light, "DPT_Switch");
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
  oc_resource_set_request_handler(res_light, OC_GET, get_dpa_417_61, NULL);
  // set the POST handler
  oc_resource_set_request_handler(res_light, OC_POST, post_dpa_417_61, NULL);
  // register this resource,
  // this means that the resource will be listed in /.well-known/core
  oc_add_resource(res_light);
}

/**
 * initiate preset for device
 *
 * @param device the device identifier of the list of devices
 * @param data the supplied data.
 */
void
factory_presets_cb(size_t device_index, void *data)
{
  (void)device_index;
  (void)data;
}

/**
 * initializes the global variables
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
 * main application.
 * initializes the global variables
 * registers and starts the handler
 * handles (in a loop) the next event.
 * shuts down the stack
 */
int
main(void)
{
  int init;
  oc_clock_time_t next_event;
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
  PRINT("\tstorage at './LSAB_minimal_pi' \n");
  oc_storage_config("./LSAB_minimal_pi_creds");

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

  oc_set_factory_presets_cb(factory_presets_cb, NULL);

  // Start the embedded Python interpreter and initialize the Python API
  // Necessary for controlling the backlight
  python_binding_init();
  // Set the backlight to the initial value
  // set_backlight(g_mystate);

  set_backlight(false);

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
  PRINT("serial number: %s", oc_string(device->serialnumber));

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
