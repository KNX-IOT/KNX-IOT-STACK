/*
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Copyright (c) 2021 Cascoda Ltd
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
 *  Example code
 */
/**
 * ## Application Design
 *
 * support functions:
 *
 * - app_init
 *   initializes the oic/p and oic/d values.
 * - register_resources
 *   function that registers all endpoints, e.g. sets the RETRIEVE/UPDATE/DELETE
 handlers for each end point
 *
 * - main
 *   starts the stack, with the registered resources.
 *   can be compiled out with NO_MAIN
 *
 *  handlers for the implemented methods (get/post):
 *   - get_[path]
 *     function that is being called when a RETRIEVE is called on [path]
 *     set the global variables in the output
 *   - post_[path]
 *     function that is being called when a UPDATE is called on [path]
 *     checks the input data
 *     if input data is correct
 *       updates the global variables
 *
 * ## stack specific defines
 *
 *  - OC_SECURITY
      enable security
 *    - OC_PKI
 *      enable use of PKI, note onboarding is enabled by means of run time code
 *  - OC_IDD_API
 *    IDD via API, otherwise use header file to define the IDD
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
 * - OPTIMIZE_PSTAT
 *   disable PSTAT observe
 */
/*
 tool_version          : 20200103
 input_file            : ../device_output/out_codegeneration_merged.swagger.json
 version of input_file :
 title of input_file   : server_lite_1599
*/

#include "oc_api.h"
#include "oc_core_res.h"
#include "port/oc_clock.h"
#include <signal.h>

#if defined(OC_IDD_API)
#include "oc_introspection.h"
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

#define MAX_STRING 30         /**< max size of the strings. */
#define MAX_PAYLOAD_STRING 65 /**< max size strings in the payload */
#define MAX_ARRAY 10          /**< max size of the array */
/* Note: Magic numbers are derived from the resource definition, either from the
 * example or the definition.*/

volatile int quit = 0;          /**< stop variable, used by handle_signal */
STATIC const size_t DEVICE = 0; /**< default device index */

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
  int ret = oc_init_platform("Cascoda", NULL, NULL);

  ret |= ock_add_device("blah", "1.0", "//", "012346", NULL, NULL);

  oc_device_info_t *device = oc_core_get_device_info(0);
  PRINT("Serial Number: %s\n", oc_string(device->serialnumber));

  /* set the hardware version*/
  oc_core_set_device_hwv(0, 5, 6, 7);

  /* set the firmware version*/
  oc_core_set_device_fwv(0, 1, 2, 3);

  /* set the internal address (ia) */
  oc_core_set_device_ia(0, 5);

  /* set the hardware type*/
  oc_core_set_device_hwt(0, "hwt-mytype");

  /* set the programming mode */
  oc_core_set_device_pm(0, true);

  /* set the model */
  oc_core_set_device_model(0, "my model");

  /* set the host name */
  oc_core_set_device_hostname(0, "my.hostname");

  /* set the installation id (iid) */
  oc_core_set_device_iid(0, "my installation");

  /* set the internal address */
  oc_core_set_device_ia(0, 5);

  oc_device_mode_display(0);

  return ret;
}

/**
 * helper function to check if the POST input document contains
 * the common readOnly properties or the resource readOnly properties
 * @param name the name of the property
 * @param error_state the current (input) error state
 * @return the error_status, e.g. if error_status is true, then the input
 * document contains something illegal
 */
STATIC bool
check_on_readonly_common_resource_properties(oc_string_t name, bool error_state)
{
  if (strcmp(oc_string(name), "n") == 0) {
    error_state = true;
    PRINT("   property \"n\" is ReadOnly \n");
  } else if (strcmp(oc_string(name), "if") == 0) {
    error_state = true;
    PRINT("   property \"if\" is ReadOnly \n");
  } else if (strcmp(oc_string(name), "rt") == 0) {
    error_state = true;
    PRINT("   property \"rt\" is ReadOnly \n");
  } else if (strcmp(oc_string(name), "id") == 0) {
    error_state = true;
    PRINT("   property \"id\" is ReadOnly \n");
  } else if (strcmp(oc_string(name), "id") == 0) {
    error_state = true;
    PRINT("   property \"id\" is ReadOnly \n");
  }
  return error_state;
}

/**
 * get method for "/p/a" resource.
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
get_dpa_352(oc_request_t *request, oc_interface_mask_t interfaces,
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

  PRINT("-- Begin get_dpa_352: interface %d\n", interfaces);
  /* check if the accept header is CBOR */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
    return;
  }

  CborError error;
  error = cbor_encode_boolean(&g_encoder, true);
  if (error) {
    PRINT("CBOR error %s\n", cbor_error_string(error));
    // oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());
  error = cbor_encode_boolean(&g_encoder, false);
  if (error) {
    PRINT("CBOR error %s\n", cbor_error_string(error));
    // oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());

  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_352\n");
}

/**
 * get method for "/b" resource.
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
get_dpa_352b(oc_request_t *request, oc_interface_mask_t interfaces,
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

  PRINT("-- Begin get_dpa_352b: interface %d\n", interfaces);
  /* check if the accept header is CBOR */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
    return;
  }

  // set a string value
  CborError error;
  error = cbor_encode_text_stringz(&g_encoder, "blahblah");
  if (error) {
    PRINT("CBOR error %s\n", cbor_error_string(error));
    // oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());
  error = cbor_encode_text_string(&g_encoder, "xyzxyz", 3);
  if (error) {
    PRINT("CBOR error %s\n", cbor_error_string(error));
    // oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());

  // oc_rep_start_root_object();
  // oc_rep_end_root_object();
  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_352b\n");
}

/**
 * get method for "/p/c" resource.
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
get_dpa_353(oc_request_t *request, oc_interface_mask_t interfaces,
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

  PRINT("-- Begin get_dpa_353: interface %d\n", interfaces);

  /* check if the accept header is CBOR */
  if (request->accept != APPLICATION_CBOR) {
    PRINT(" accept %d", request->accept);
    oc_send_response(request, OC_STATUS_BAD_OPTION);
    return;
  }

  // oc_rep_start_root_object();
  // oc_rep_end_root_object();
  CborError error;
  error = cbor_encode_int(&g_encoder, (int64_t)555);
  if (error) {
    PRINT("CBOR error %s\n", cbor_error_string(error));
    // oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());
  error = cbor_encode_int(&g_encoder, (int64_t)666);
  if (error) {
    PRINT("CBOR error %s\n", cbor_error_string(error));
    // oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());
  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_353\n");
}

/**
 * post method for "/p/a" resource.
 * The function has as input the request body, which are the input values of the
 POST method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 values.
 * Resource Description:

 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
STATIC void
post_dpa_352(oc_request_t *request, oc_interface_mask_t interfaces,
             void *user_data)
{
  (void)interfaces;
  (void)user_data;
  bool error_state = false;
  PRINT("-- Begin post_dpa_352:\n");
  oc_rep_t *rep = request->request_payload;

  /* loop over the request document for each required input field to check if
   * all required input fields are present */
  bool var_in_request = false;
  rep = request->request_payload;
  //  while (rep != NULL) {
  //    if (strcmp(oc_string(rep->name),
  //               g_binaryswitch_RESOURCE_PROPERTY_NAME_value) == 0) {
  //      var_in_request = true;
  //    }
  //    rep = rep->next;
  //  }
  if (var_in_request == false) {
    error_state = true;
    PRINT(" required property: 'value' not in request\n");
  }
  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;

  /* if the input is ok, then process the input document and assign the global
   * variables */
  if (error_state == false) {
    switch (interfaces) {
    default: {
      /* loop over all the properties in the input document */
      oc_rep_t *rep = request->request_payload;

      /* set the response */
      PRINT("Set response \n");
      oc_rep_start_root_object();
      /*oc_process_baseline_interface(request->resource); */
      // PRINT("   %s : %s", g_binaryswitch_RESOURCE_PROPERTY_NAME_value,
      //        (char *)btoa(g_binaryswitch_value));
      //  oc_rep_set_boolean(root, value, g_binaryswitch_value);

      oc_rep_end_root_object();
      /* TODO: ACTUATOR add here the code to talk to the HW if one implements an
       actuator. one can use the global variables as input to those calls the
       global values have been updated already with the data from the request */

      oc_send_cbor_response(request, OC_STATUS_OK);
    }
    }
  } else {
    PRINT("  Returning Error \n");
    /* TODO: add error response, if any */
    // oc_send_response(request, OC_STATUS_NOT_MODIFIED);
    oc_send_response(request, OC_STATUS_BAD_REQUEST);
  }
  PRINT("-- End post_dpa_352\n");
}

/**
 * post method for "/p/b" resource.
 * The function has as input the request body, which are the input values of the
 POST method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 values.
 * Resource Description:

 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
STATIC void
post_dpa_352b(oc_request_t *request, oc_interface_mask_t interfaces,
              void *user_data)
{
  (void)interfaces;
  (void)user_data;
  bool error_state = false;
  PRINT("-- Begin post_dpa_352b:\n");
  oc_rep_t *rep = request->request_payload;

  /* loop over the request document for each required input field to check if
   * all required input fields are present */
  bool var_in_request = false;
  rep = request->request_payload;
  //  while (rep != NULL) {
  //    if (strcmp(oc_string(rep->name),
  //               g_binaryswitch_RESOURCE_PROPERTY_NAME_value) == 0) {
  //      var_in_request = true;
  //    }
  //    rep = rep->next;
  //  }
  if (var_in_request == false) {
    error_state = true;
    PRINT(" required property: 'value' not in request\n");
  }
  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;

  /* if the input is ok, then process the input document and assign the global
   * variables */
  if (error_state == false) {
    switch (interfaces) {
    default: {
      /* loop over all the properties in the input document */
      oc_rep_t *rep = request->request_payload;

      /* set the response */
      PRINT("Set response \n");
      oc_rep_start_root_object();
      /*oc_process_baseline_interface(request->resource); */
      // PRINT("   %s : %s", g_binaryswitch_RESOURCE_PROPERTY_NAME_value,
      //        (char *)btoa(g_binaryswitch_value));
      //  oc_rep_set_boolean(root, value, g_binaryswitch_value);

      oc_rep_end_root_object();
      /* TODO: ACTUATOR add here the code to talk to the HW if one implements an
       actuator. one can use the global variables as input to those calls the
       global values have been updated already with the data from the request */

      oc_send_cbor_response(request, OC_STATUS_CHANGED);
    }
    }
  } else {
    PRINT("  Returning Error \n");
    /* TODO: add error response, if any */
    // oc_send_response(request, OC_STATUS_NOT_MODIFIED);
    oc_send_response(request, OC_STATUS_BAD_REQUEST);
  }
  PRINT("-- End post_dpa_352b\n");
}

/**
 * post method for "/p/b" resource.
 * The function has as input the request body, which are the input values of the
 POST method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 values.
 * Resource Description:

 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
STATIC void
post_dpa_353(oc_request_t *request, oc_interface_mask_t interfaces,
             void *user_data)
{
  (void)interfaces;
  (void)user_data;
  bool error_state = false;
  PRINT("-- Begin post_dpa_353:\n");
  oc_rep_t *rep = request->request_payload;

  /* loop over the request document for each required input field to check if
   * all required input fields are present */
  bool var_in_request = false;
  rep = request->request_payload;
  //  while (rep != NULL) {
  //    if (strcmp(oc_string(rep->name),
  //               g_binaryswitch_RESOURCE_PROPERTY_NAME_value) == 0) {
  //      var_in_request = true;
  //    }
  //    rep = rep->next;
  //  }
  if (var_in_request == false) {
    error_state = true;
    PRINT(" required property: 'value' not in request\n");
  }
  /* loop over the request document to check if all inputs are ok */
  rep = request->request_payload;

  /* if the input is ok, then process the input document and assign the global
   * variables */
  if (error_state == false) {
    switch (interfaces) {
    default: {
      /* loop over all the properties in the input document */
      oc_rep_t *rep = request->request_payload;

      /* set the response */
      PRINT("Set response \n");
      oc_rep_start_root_object();
      /*oc_process_baseline_interface(request->resource); */
      // PRINT("   %s : %s", g_binaryswitch_RESOURCE_PROPERTY_NAME_value,
      //        (char *)btoa(g_binaryswitch_value));
      //  oc_rep_set_boolean(root, value, g_binaryswitch_value);

      oc_rep_end_root_object();
      /* TODO: ACTUATOR add here the code to talk to the HW if one implements an
       actuator. one can use the global variables as input to those calls the
       global values have been updated already with the data from the request */
      oc_send_response(request, OC_STATUS_CHANGED);
    }
    }
  } else {
    PRINT("  Returning Error \n");
    /* TODO: add error response, if any */
    // oc_send_response(request, OC_STATUS_NOT_MODIFIED);
    oc_send_cbor_response(request, OC_STATUS_NOT_MODIFIED);
  }
  PRINT("-- End post_dpa_353b\n");
}

/**
 * register all the resources to the stack
 * this function registers all application level resources:
 * - each resource path is bind to a specific function for the supported methods
 * (GET, POST, PUT)
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

  PRINT("Register Resource with local path \"/p/a\"\n");
  oc_resource_t *res_352 = oc_new_resource("myname", "p/a", 1, 0);
  oc_resource_bind_resource_type(res_352, "urn:knx:dpa.352.51");
  oc_resource_bind_content_type(res_352, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_352, OC_IF_AC); /* if.a */
  oc_resource_set_discoverable(res_352, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  oc_resource_set_periodic_observable(res_352, 1);
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  /*oc_resource_set_observable(res_352, true); */
  oc_resource_set_request_handler(res_352, OC_GET, get_dpa_352, NULL);
  oc_resource_set_request_handler(res_352, OC_POST, post_dpa_352, NULL);
  oc_add_resource(res_352);

  PRINT("Register Resource with local path \"/p/b\"\n");
  oc_resource_t *res_352b = oc_new_resource("myname_b", "p/b", 1, 0);
  oc_resource_bind_resource_type(res_352b, "urn:knx:dpa.352.52");
  oc_resource_bind_content_type(res_352b, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_352b, OC_IF_SE); /* if.s */
  oc_resource_set_discoverable(res_352b, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  oc_resource_set_periodic_observable(res_352b, 1);
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  /*oc_resource_set_observable(res_352, true); */
  oc_resource_set_request_handler(res_352b, OC_GET, get_dpa_352b, NULL);
  oc_resource_set_request_handler(res_352b, OC_POST, post_dpa_352b, NULL);
  oc_add_resource(res_352b);

  PRINT("Register Resource with local path \"/p/c\"\n");
  oc_resource_t *res_353 = oc_new_resource("myname_c", "p/c", 1, 0);
  oc_resource_bind_resource_type(res_353, "urn:knx:dpa.353.52");
  oc_resource_bind_content_type(res_353, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_353, OC_IF_SE); /* if.s */
  oc_resource_set_discoverable(res_353, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  oc_resource_set_periodic_observable(res_353, 1);
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  /*oc_resource_set_observable(res_352, true); */
  oc_resource_set_request_handler(res_353, OC_GET, get_dpa_353, NULL);
  oc_resource_set_request_handler(res_353, OC_POST, post_dpa_353, NULL);
  oc_add_resource(res_353);
}

/**
 * initiate preset for device
 *
 * @param device the device identifier of the list of devices
 * @param data the supplied data.
 */
void
factory_presets_cb(size_t device, void *data)
{
  (void)device;
  (void)data;
}

/**
 * initializes the global variables
 * registers and starts the handler
 */
void
initialize_variables(void)
{
  /* initialize global variables for resource "/binaryswitch" */
  // g_binaryswitch_value =
  //    false; /* current value of property "value" The status of the switch. */

  /* set the flag for NO oic/con resource. */
  oc_set_con_res_announced(false);
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

#ifdef OC_SECURITY
/**
 * oc_ownership_status_cb callback implementation
 * handler to print out the DI after onboarding
 *
 * @param device_uuid the device ID
 * @param device_index the index in the list of device IDs
 * @param owned owned or unowned indication
 * @param user_data the supplied user data.
 */
void
oc_ownership_status_cb(const oc_uuid_t *device_uuid, size_t device_index,
                       bool owned, void *user_data)
{
  (void)user_data;
  (void)device_index;
  (void)owned;

  char uuid[37] = { 0 };
  oc_uuid_to_str(device_uuid, uuid, OC_UUID_LEN);
  PRINT(" oc_ownership_status_cb: DI: '%s'\n", uuid);
}
#endif /* OC_SECURITY * /                                                        \
                                                                               \ \
/**                                                                              \
 * main application.                                                             \
 * initializes the global variables                                              \
 * registers and starts the handler                                              \
 * handles (in a loop) the next event.                                           \
 * shuts down the stack                                                          \
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

  PRINT("KNX-IOT Server name : \"server_1599\"\n");

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
  PRINT("\tstorage at './simpleserver_all_creds' \n");
  oc_storage_config("./simpleserver_all_creds");

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

  /* start the stack */
  init = oc_main_init(&handler);

  if (init < 0) {
    PRINT("oc_main_init failed %d, exiting.\n", init);
    return init;
  }

#ifdef OC_SECURITY
  /* print out the current DI of the device */
  char uuid[37] = { 0 };
  oc_uuid_to_str(oc_core_get_device_id(0), uuid, OC_UUID_LEN);
  PRINT(" DI: '%s'\n", uuid);
  oc_add_ownership_status_cb(oc_ownership_status_cb, NULL);
#endif /* OC_SECURITY */

#ifdef OC_SECURITY
  PRINT("Security - Enabled\n");
#else
  PRINT("Security - Disabled\n");
#endif /* OC_SECURITY */

  PRINT("Server \"server_1599\" running, waiting on incoming "
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

  /* shut down the stack */

  oc_main_shutdown();
  return 0;
}
#endif /* NO_MAIN */
