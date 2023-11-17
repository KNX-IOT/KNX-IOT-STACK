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
 *  Example code
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
 *  handlers for the implemented methods (get/put):
 *   - get_[path]
 *     function that is being called when a GET is called on [path]
 *     set the global variables in the output
 *   - put_[path]
 *     function that is being called when a PUT is called on [path]
 *     updates the global variables
 *
 * ## stack specific defines
 * - OC_OSCORE
 * - OC_SPAKE
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
#define _WIN32_WINNT 0x8000
#include "oc_api.h"
#include "oc_core_res.h"
#include "api/oc_knx_fp.h"
#include "api/oc_knx_sec.h"
#include "api/oc_knx_gm.h"
#ifdef OC_SPAKE
#include "security/oc_spake2plus.h"
#endif
#include "port/oc_clock.h"
#include <signal.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// test purpose only
#include "api/oc_knx_dev.h"

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

bool g_352_51_state = false;   /**< state variable for dp 352.51 instance 0 */
bool g_352_51_1_state = false; /**< state variable for dp 352.51 instance 1 */
bool g_352_52_state = false;   /**< state variable for dp 352.52 instance 0 */
bool g_353_52_state = false;   /**< state variable for dp 353.52 instance 0 */
volatile int quit = 0;         /**< stop variable, used by handle_signal */
bool g_reset = false;
int call_counter = 0;

#define MY_SERIAL_NUMBER "123456789012"

/**
 * @brief callback for the smode response
 * testing purpose
 *
 * @param url url called
 * @param rep the response
 * @param rep_value the response value
 */
void
oc_add_s_mode_response_cb(char *url, oc_rep_t *rep, oc_rep_t *rep_value)
{
  (void)rep;
  (void)rep_value;
  PRINT("oc_add_s_mode_response_cb %s\n", url);
}

void
oc_gateway_s_mode_cb(size_t device_index, char *sender_ip_address,
                     oc_group_object_notification_t *s_mode_message, void *data)
{
  (void)data;

  PRINT("testserver_all: oc_gateway_s_mode_cb %s\n", sender_ip_address);
  PRINT("   ga  = %d\n", s_mode_message->ga);
  PRINT("   sia = %d\n", s_mode_message->sia);
  PRINT("   st  = %s\n", oc_string_checked(s_mode_message->st));
  PRINT("   val = %s\n", oc_string_checked(s_mode_message->value));
}

/**
 * function to set up the device.
 *
 * sets the:
 * - manufacturer name
 * - serial number
 * - friendly device name (not needed for knx)
 * - spec version
 * - base path
 * - serial number
 * - hardware version
 * - firmware version
 * - hardware type
 * - model name
 * - spake password
 */
int
app_init(void)
{
  /* create platform and set the manufacturer name */
  int ret = oc_init_platform("Cascoda", NULL, NULL);

  /* create the device and set
    - specification number (1.0.0)
    - base path (/)
    - the serial number
  */
  ret |= oc_add_device("my_name", "1.0.0", "//", "123456789012", NULL, NULL);
  oc_device_info_t *device = oc_core_get_device_info(0);
  /* set the hardware version*/
  oc_core_set_device_hwv(0, 5, 6, 7);
  /* set the firmware version*/
  oc_core_set_device_fwv(0, 1, 2, 3);
  /* set the hardware type*/
  oc_core_set_device_hwt(0, "hwt-mytype");
  /* set the model */
  oc_core_set_device_model(0, "my model");

#ifdef OC_SPAKE
#define PASSWORD "LETTUCE"
  oc_spake_set_password(PASSWORD);
  PRINT(" SPAKE password %s\n", PASSWORD);
#endif

  /* set the client callback, for testing purposes only */
  oc_set_s_mode_response_cb(oc_add_s_mode_response_cb);

  /* set the gateway call back for receiving all s-mode messages */
  oc_set_gateway_cb(oc_gateway_s_mode_cb, NULL);

  return ret;
}

/**
 * get method for "/p/a_1" resource.
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
get_dpa_352_51(oc_request_t *request, oc_interface_mask_t interfaces,
               void *user_data)
{
  (void)user_data;          /**< variable not used */
  (void)interfaces;         /**< variable not used */
  bool error_state = false; /**< the error state, the generated code */
  int oc_status_code = OC_STATUS_OK;

  PRINT("-- Begin get_dpa_352_51\n");
  /* check if the accept header is CBOR */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
    return;
  }
  // handle the query parameter m
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
          oc_rep_set_text_string(root, if, "if.a");
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
      oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
    }
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  CborError error;
  // error = cbor_encode_boolean(&g_encoder, g_352_51_state);
  oc_rep_begin_root_object();
  oc_rep_i_set_boolean(root, 1, g_352_51_state);
  oc_rep_end_root_object();
  error = g_err;
  if (error) {
    oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());

  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_352_51\n");
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
get_dpa_352_51_1(oc_request_t *request, oc_interface_mask_t interfaces,
                 void *user_data)
{
  (void)user_data;  /* variable not used */
  (void)interfaces; /**< variable not used */

  bool error_state = false; /**< the error state, the generated code */
  int oc_status_code = OC_STATUS_OK;

  PRINT("-- Begin get_dpa_352_51_1\n");
  /* check if the accept header is CBOR */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
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
          oc_rep_set_text_string(root, if, "if.a");
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
      oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
    }
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  CborError error;
  error = cbor_encode_boolean(&g_encoder, g_352_51_1_state);
  oc_rep_begin_root_object();
  oc_rep_i_set_boolean(root, 1, g_352_51_1_state);
  oc_rep_end_root_object();
  error = g_err;

  if (error) {
    oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());

  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_352_51_1\n");
}

/**
 * get method for "/p/b" resource.
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
get_dpa_352_52(oc_request_t *request, oc_interface_mask_t interfaces,
               void *user_data)
{
  (void)user_data;          /**< variable not used */
  (void)interfaces;         /**< variable not used */
  bool error_state = false; /**< the error state, the generated code */
  int oc_status_code = OC_STATUS_OK;

  PRINT("-- Begin get_dpa_352_52\n");
  /* check if the accept header is CBOR */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
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
          oc_rep_set_text_string(root, if, "if.a");
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
      oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
    }
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  CborError error;
  // error = cbor_encode_boolean(&g_encoder, g_352_52_state);
  oc_rep_begin_root_object();
  oc_rep_i_set_boolean(root, 1, g_352_52_state);
  oc_rep_end_root_object();
  error = g_err;
  if (error) {
    oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());

  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_352_52\n");
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
get_dpa_353_52(oc_request_t *request, oc_interface_mask_t interfaces,
               void *user_data)
{
  (void)user_data;          /* variable not used */
  bool error_state = false; /**< the error state, the generated code */
  int oc_status_code = OC_STATUS_OK;
  CborError error;

  PRINT("-- Begin get_dpa_353_52: interface %d\n", interfaces);
  /* check if the accept header is CBOR */
  if (oc_check_accept_header(request, APPLICATION_CBOR) == false) {
    PRINT(" accept %d", request->accept);
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
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
          oc_rep_set_text_string(root, if, "if.a");
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
      oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
    }
    oc_send_cbor_response(request, OC_STATUS_OK);
    return;
  }

  // error = cbor_encode_boolean(&g_encoder, g_352_52_state);
  oc_rep_begin_root_object();
  oc_rep_i_set_boolean(root, 1, g_352_52_state);
  oc_rep_end_root_object();
  error = g_err;
  if (error) {
    oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());
  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response_no_format(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_353_52\n");
}

/**
 * put method for "/p/a" resource.
 * The function has as input the request body, which are the input values of the
 * PUT method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 * values.
 * Resource Description:
 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
STATIC void
put_dpa_352_51(oc_request_t *request, oc_interface_mask_t interfaces,
               void *user_data)
{
  (void)interfaces;
  (void)user_data;
  bool error_state = false;
  PRINT("-- Begin put_dpa_352_51:\n");
  oc_rep_t *rep = NULL;
  // handle the different requests
  if (oc_is_redirected_request(request)) {
    PRINT("  S-MODE or /P\n");
  }
  rep = request->request_payload;
  // handle the type of payload correctly.
  while (rep != NULL) {
    if (rep->type == OC_REP_BOOL) {
      if (rep->iname == 1) {
        PRINT("  put_dpa_352_51 received : %d\n", rep->value.boolean);
        g_352_51_state = rep->value.boolean;
        oc_send_cbor_response(request, OC_STATUS_CHANGED);
        PRINT("-- End put_dpa_352_51\n");
        return;
      }
    }
    rep = rep->next;
  }

  PRINT("  Returning Error \n");
  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
  PRINT("-- End put_dpa_352_51\n");
}

/**
 * put method for "/p/a_1" resource.
 * The function has as input the request body, which are the input values of the
 * PUT method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 * values.
 * Resource Description:
 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
STATIC void
put_dpa_352_51_1(oc_request_t *request, oc_interface_mask_t interfaces,
                 void *user_data)
{
  (void)interfaces;
  (void)user_data;
  bool error_state = false;
  PRINT("-- Begin put_dpa_352_51_1:\n");

  oc_rep_t *rep = NULL;
  // handle the different requests
  if (oc_is_redirected_request(request)) {
    PRINT("  S-MODE or /P\n");
  }
  rep = request->request_payload;

  // handle the type of payload correctly.
  while (rep != NULL) {
    if (rep->type == OC_REP_BOOL) {
      if (rep->iname == 1) {
        PRINT("  put_dpa_352_51_1 received : %d\n", rep->value.boolean);
        g_352_51_1_state = rep->value.boolean;
        oc_send_cbor_response(request, OC_STATUS_CHANGED);
        PRINT("-- End put_dpa_352_51_1\n");
        return;
      }
    }
    rep = rep->next;
  }

  PRINT("  Returning Error \n");
  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
  PRINT("-- End put_dpa_352_51_1\n");
}

/**
 * put method for "/p/b" resource.
 * The function has as input the request body, which are the input values of the
 * PUT method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 * values.
 * Resource Description:
 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
STATIC void
put_dpa_352_52(oc_request_t *request, oc_interface_mask_t interfaces,
               void *user_data)
{
  (void)interfaces;
  (void)user_data;
  PRINT("-- Begin put_dpa_352_52:\n");

  oc_rep_t *rep = NULL;
  // handle the different requests
  if (oc_is_redirected_request(request)) {
    PRINT("  S-MODE or /P\n");
  }
  rep = request->request_payload;

  /* loop over the request document to check if all inputs are ok */
  while (rep != NULL) {
    if (rep->type == OC_REP_BOOL) {
      if (rep->iname == 1) {
        PRINT("  put_dpa_352_52 received : %d\n", rep->value.boolean);
        g_352_52_state = rep->value.boolean;
        oc_send_cbor_response(request, OC_STATUS_CHANGED);
        PRINT("-- End put_dpa_352_52\n");
        return;
      }
    }
    rep = rep->next;
  }

  /* if the input is ok, then process the input document and assign the global
   * variables */
  PRINT("  Returning Error \n");
  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
  PRINT("-- End put_dpa_352_52\n");
}

/**
 * put method for "/p/b" resource.
 * The function has as input the request body, which are the input values of the
 * PUT method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 * values.
 * Resource Description:
 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
STATIC void
put_dpa_353_52(oc_request_t *request, oc_interface_mask_t interfaces,
               void *user_data)
{
  (void)interfaces;
  (void)user_data;
  oc_rep_t *rep = NULL;
  PRINT("-- Begin put_dpa_353_52:\n");

  if (oc_is_redirected_request(request)) {
    PRINT("  S-MODE or /P\n");
  }
  rep = request->request_payload;
  while (rep != NULL) {
    if (rep->type == OC_REP_BOOL) {
      if (rep->iname == 1) {
        PRINT("  put_dpa_353_52 received : %d\n", rep->value.boolean);
        g_353_52_state = rep->value.boolean;
        oc_send_cbor_response(request, OC_STATUS_CHANGED);
        PRINT("-- End put_dpa_353_52\n");
        return;
      }
    }
    rep = rep->next;
  }
  oc_send_response_no_format(request, OC_STATUS_BAD_REQUEST);
  PRINT("-- End put_dpa_353_52\n");
}

/**
 * register all the resources to the stack
 * this function registers all application level resources:
 * - each resource path is bind to a specific function for the supported methods
 * (GET, POST, PUT, DELETE)
 * - each resource will can be set up with
 *   - resource types
 *   - discoverable (e.g. listed in /.well-known/core)
 *   - used interfaces
 *   - content type (CBOR/JSON)
 *   - function block instance (default = instance 0)
 *   - observable
 * Note that the resource type(s) determines the functional block.
 */
void
register_resources(void)
{

  PRINT("Register Resource with local path \"/p/a\"\n");
  oc_resource_t *res_352 = oc_new_resource("myname", "/p/a", 1, 0);
  oc_resource_bind_resource_type(res_352, "urn:knx:dpa.352.51");
  oc_resource_bind_dpt(res_352, "urn:knx:dpt.switch");
  oc_resource_bind_content_type(res_352, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_352, OC_IF_A); /* if.a */
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
  oc_resource_set_request_handler(res_352, OC_GET, get_dpa_352_51, NULL);
  oc_resource_set_request_handler(res_352, OC_PUT, put_dpa_352_51, NULL);
  oc_add_resource(res_352);

  oc_resource_t *res_352_1 = oc_new_resource("myname", "/p/a_1", 1, 0);
  oc_resource_bind_resource_type(res_352_1, "urn:knx:dpa.352.51");
  oc_resource_bind_dpt(res_352_1, "urn:knx:dpt.switch");
  oc_resource_bind_content_type(res_352_1, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_352_1, OC_IF_A); /* if.a */
  oc_resource_set_discoverable(res_352_1, true);
  oc_resource_set_function_block_instance(res_352_1, 1);

  oc_resource_set_request_handler(res_352_1, OC_GET, get_dpa_352_51_1, NULL);
  oc_resource_set_request_handler(res_352_1, OC_PUT, put_dpa_352_51_1, NULL);
  oc_add_resource(res_352_1);

  PRINT("Register Resource with local path \"/p/b\"\n");
  oc_resource_t *res_352b = oc_new_resource("myname_b", "/p/b", 1, 0);
  oc_resource_bind_resource_type(res_352b, "urn:knx:dpa.352.52");
  oc_resource_bind_dpt(res_352b, "urn:knx:dpt.switch");
  oc_resource_bind_content_type(res_352b, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_352b, OC_IF_S); /* if.s */
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
  oc_resource_set_request_handler(res_352b, OC_GET, get_dpa_352_52, NULL);
  oc_resource_set_request_handler(res_352b, OC_PUT, put_dpa_352_52, NULL);
  oc_add_resource(res_352b);

  PRINT("Register Resource with local path \"/p/c\"\n");
  oc_resource_t *res_353 = oc_new_resource("myname_c", "/p/c", 1, 0);
  oc_resource_bind_resource_type(res_353, "urn:knx:dpa.353.52");
  oc_resource_bind_dpt(res_353, "urn:knx:dpt.switch");
  oc_resource_bind_dpt(res_353, "urn:knx:dpt.switch2");
  oc_resource_bind_content_type(res_353, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_353, OC_IF_S); /* if.s  */
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
  oc_resource_set_request_handler(res_353, OC_GET, get_dpa_353_52, NULL);
  oc_resource_set_request_handler(res_353, OC_PUT, put_dpa_353_52, NULL);
  oc_add_resource(res_353);
}

/**
 * initiate preset for device
 *
 * @param device_index the device identifier of the list of devices
 * @param data the supplied data.
 */
void
factory_presets_cb(size_t device_index, void *data)
{
  (void)device_index;
  (void)data;

  if (g_reset) {
    PRINT("resetting device\n");
    oc_knx_device_storage_reset(0, 2);
  }

  oc_core_set_and_store_device_ia(device_index, 5);
  oc_core_set_and_store_device_iid(device_index, 7);
}

/**
 * application reset
 *
 * @param device_index the device identifier of the list of devices
 * @param data the supplied data.
 */
void
reset_cb(size_t device_index, int reset_value, void *data)
{
  (void)data;

  PRINT("reset_cb %d\n", reset_value);
}

/**
 * restart the device (application depended)
 *
 * @param device_index the device identifier of the list of devices
 * @param data the supplied data.
 */
void
restart_cb(size_t device_index, void *data)
{
  (void)data;

  PRINT("-----restart_cb -------\n");
  // exit(0);
}

/**
 * set the host name on the device (application depended)
 *
 * @param device_index the device identifier of the list of devices
 * @param host_name the host name to be set on the device
 * @param data the supplied data.
 */
void
hostname_cb(size_t device_index, oc_string_t host_name, void *data)
{
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

/* separate files for each call to transport a block of data*/
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
 * initializes the global variables
 * registers and starts the handler
 */
void
initialize_variables(void)
{
  /* initialize global (state) variables for resources */
  g_352_51_state = false;
  g_352_51_1_state = false;
  g_352_52_state = false;
  g_353_52_state = false;
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

int g_counter = 0;

oc_event_callback_retval_t
issue_requests(void *data)
{
  g_counter++;

  PRINT("  issue_requests_s_mode: issue\n");

  oc_do_s_mode_with_scope(2, "/p/a", "w");
  oc_do_s_mode_with_scope(5, "/p/a", "w");

  oc_do_s_mode_with_scope(2, "/p/b", "w");
  oc_do_s_mode_with_scope(5, "/p/b", "w");

  oc_do_s_mode_with_scope(2, "/p/c", "w");
  oc_do_s_mode_with_scope(5, "/p/c", "w");

  PRINT("---------------> s_mode loop %d\n", g_counter);
  if (g_counter == 10) {
    PRINT("---------------> QUIT  %d\n", g_counter);
    exit(0);
  }

  oc_set_delayed_callback(NULL, issue_requests, 0);
  return OC_EVENT_DONE;
}

/**
 * send a multicast s-mode message
 * fires only once
 * @param data the callback data
 */
oc_event_callback_retval_t
issue_requests_s_mode_delayed(void *data)
{
  (void)data;

  // setting the test data
  oc_device_info_t *device = oc_core_get_device_info(0);
  device->ia = 5;
  device->iid = 16;

  PRINT(" issue_requests_s_mode_delayed : ia = %d\n", device->ia);
  PRINT(" issue_requests_s_mode_delayed : iid = %" PRIu64 "\n", device->iid);

  PRINT(" issue_requests_s_mode_delayed : config data\n");
  int ga_values[5] = { 1, 255, 256, 1024, 1024 * 256 };
  oc_string_t href;
  oc_new_string(&href, "/p/c", strlen("/p/c"));

  oc_group_object_table_t entry;
  entry.cflags = OC_CFLAG_WRITE | OC_CFLAG_READ;
  entry.id = 55;
  entry.href = href;
  entry.ga_len = 1;
  entry.ga = (int *)&ga_values;

  oc_core_set_group_object_table(0, entry);

  PRINT("INDEX 0\n");
  oc_print_group_object_table_entry(0);
  PRINT("\n");

  oc_group_object_table_t entry2;
  entry2.cflags = OC_CFLAG_TRANSMISSION;
  entry2.id = 5;
  entry2.href = href;
  entry2.ga_len = 1;
  entry2.ga = (int *)&ga_values;

  oc_core_set_group_object_table(1, entry2);
  PRINT("INDEX 1\n");
  oc_print_group_object_table_entry(1);
  PRINT("\n");

  oc_group_object_table_t entry3;
  entry3.cflags = OC_CFLAG_WRITE | OC_CFLAG_INIT;
  entry3.id = 6;
  entry3.href = href;
  entry3.ga_len = 1;
  entry3.ga = (int *)&ga_values;

  oc_core_set_group_object_table(2, entry3);
  PRINT("INDEX 2\n");
  oc_print_group_object_table_entry(2);
  PRINT("\n");

  oc_group_object_table_t entry4;
  entry4.cflags = OC_CFLAG_WRITE;
  entry4.id = 7;
  entry4.href = href;
  entry4.ga_len = 1;
  entry4.ga = (int *)&ga_values;
  oc_core_set_group_object_table(3, entry4);
  PRINT("INDEX 3\n");
  oc_print_group_object_table_entry(3);
  PRINT("\n");

  int index = 0;

  // set loaded
  device->lsm_s = LSM_S_LOADED;

  // listen to the registered multicast addresses e.g. group address 1
  oc_register_group_multicasts();

  // test invoking read on initialization.
  oc_init_datapoints_at_initialization();

  oc_set_delayed_callback(NULL, issue_requests, 1);
  // issue_requests();

  return OC_EVENT_DONE;
}

oc_endpoint_t g_endpoint;

void
response_get_pm(oc_client_response_t *data)
{
  PRINT("=============> response_get_pm (%d): content format :%d  code:%d\n",
        call_counter, data->content_format, data->code);
  oc_print_rep_as_json(data->payload, true);

  call_counter++;

  // oc_do_get_ex("/dev/pm", &g_endpoint, NULL, response_get_pm, HIGH_QOS,
  //              APPLICATION_CBOR, APPLICATION_CBOR, NULL);
}

void
spake_cb(int error, char *sn, char *oscore_id, int oscore_id_size,
         uint8_t *secret, int secret_size)
{
  PRINT("spake CB: invoke PM with encryption!!!!!\n");
#ifdef OC_OSCORE
  g_endpoint.flags = IPV6;
  g_endpoint.flags += OSCORE;
  PRINT("  spake_cb: enable OSCORE encryption\n");

  PRINT("  spake_cb SN %s\n", sn);
  PRINT("  spake_cb id size %d\n", oscore_id_size);
  PRINT("  spake_cb ms size %d\n", secret_size);

  // oc_endpoint_copy(&g_endpoint, endpoint);
  oc_endpoint_set_oscore_id_from_str(&g_endpoint, sn);

  // PRINT("  spake_cb: ep serial %s\n", g_endpoint.serial_number);
#endif
  PRINT("spake CB\n");
  oc_do_get_ex("/dev/pm", &g_endpoint, NULL, response_get_pm, HIGH_QOS,
               APPLICATION_CBOR, APPLICATION_CBOR, NULL);
}

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
  char *my_serialnum = "123456789012";

  PRINT("[C]DISCOVERY: %.*s\n", len, payload);
  int nr_entries = oc_lf_number_of_entries(payload, len);
  PRINT("[C] entries %d\n", nr_entries);
  int found = 0;

  // PRINT("[C] issue get on /dev/pm\n");
  oc_endpoint_print(endpoint);

  /* remove OSCORE flag */
  endpoint->flags = IPV6;
  PRINT("  [C] disable OSCORE encryption\n");
  PRINTipaddr_flags(*endpoint);
  PRINTipaddr(*endpoint);
  oc_endpoint_set_oscore_id_from_str(endpoint, my_serialnum);
  oc_char_println_hex(endpoint->oscore_id, endpoint->oscore_id_len);

  // copy the endpoint so that we know it in the spake2plus callback
  oc_endpoint_copy(&g_endpoint, endpoint);

  oc_set_spake_response_cb(spake_cb);
  // oc_initiate_spake(endpoint, "LETTUCE", "abcdef");

  // for testing the receive key must be the same, since we are talking to the
  // same device. so it depends on who wil store the oscore context first.. with
  // SID and RID.
  uint8_t array[30];
  size_t array_size = 30;

  memset(array, 9, 30);

  oc_conv_hex_string_to_byte_array(MY_SERIAL_NUMBER, strlen(MY_SERIAL_NUMBER),
                                   array, &array_size);

  PRINT("-------<RID  %s  %d     ", MY_SERIAL_NUMBER, (int)array_size);
  oc_char_println_hex(array, array_size);
  oc_initiate_spake_parameter_request(endpoint, my_serialnum, "LETTUCE", array,
                                      array_size);

  PRINT("[C] DISCOVERY- END\n");
  return OC_STOP_DISCOVERY;
}

/**
 * discovers myself
 */
void
issue_requests_oscore(void)
{

  PRINT("issue_requests_oscore\n");
  int index = 0;
  oc_auth_at_t access_token;
  memset(&access_token, 0, sizeof(access_token));

  access_token.profile = OC_PROFILE_COAP_OSCORE;
  oc_core_set_at_table(0, index, access_token, false);
  oc_print_auth_at_entry(0, index);

  oc_new_string(&access_token.osc_id, "123", strlen("123"));
  oc_new_string(&access_token.id, "1234", strlen("1234"));
  oc_new_string(&access_token.osc_contextid, "id1", strlen("id1"));
  oc_new_string(&access_token.osc_ms, (char *)"ABCDE", 5);
  oc_new_string(&access_token.kid, "", 0);
  oc_new_string(&access_token.sub, "", 0);
  // oc_new_string(&access_token.osc_alg, "", 0);
  access_token.profile = OC_PROFILE_COAP_OSCORE;
  int64_t ga_values[5] = { 1, 2, 3, 4, 5 };
  access_token.ga = ga_values;
  access_token.ga_len = 3;
  oc_core_set_at_table(0, index, access_token, false);
  oc_print_auth_at_entry(0, index);
  oc_init_oscore(0);

  access_token.ga_len = 5;
  oc_new_string(&access_token.id, "1", strlen("1"));
  oc_new_string(&access_token.osc_id, "2", strlen("2"));
  oc_new_string(&access_token.osc_contextid, "3", strlen("3"));
  oc_core_set_at_table(0, index, access_token, false);
  oc_print_auth_at_entry(0, index);

  // first step is discover myself..
  oc_do_wk_discovery_all("ep=urn:knx:sn.123456789012", 2, discovery_cb, NULL);
}

void oc_issue_s_mode(int scope, int sia_value, uint32_t grpid,
                     uint32_t group_address, uint64_t iid, char *rp,
                     uint8_t *value_data, int value_size);

static bool oscore_init = false;

/**
 * test of decoding a message to myself
 */
oc_event_callback_retval_t
issue_s_mode_secure(void *data)
{

  PRINT("issue_s_mode_secure\n");

  int index = 0;

  if (oscore_init == false) {
    oc_auth_at_t access_token;
    memset(&access_token, 0, sizeof(access_token));

    access_token.profile = OC_PROFILE_COAP_OSCORE;
    oc_core_set_at_table(0, index, access_token, true);
    oc_print_auth_at_entry(0, index);

    oc_new_string(&access_token.osc_id, "y1234567890AB",
                  strlen("01234567890AB"));
    char str_ms[14] = {
      '1', '2', '3', '4', '5', '\0', '6', '7', '8', '9', '1'
    };

    oc_new_string(&access_token.id, "1234", strlen("1234"));
    oc_new_string(&access_token.osc_contextid, "1234567890AB",
                  strlen("01234567890AB"));
    oc_new_string(&access_token.osc_ms, (char *)str_ms, 11);
    oc_new_string(&access_token.kid, "", 0);
    oc_new_string(&access_token.sub, "", 0);
    // oc_new_string(&access_token.osc_alg, "", 0);
    access_token.profile = OC_PROFILE_COAP_OSCORE;
    int64_t ga_values[5] = { 1, 2, 3, 4, 5 };
    access_token.ga = ga_values;
    access_token.ga_len = 3;
    oc_core_set_at_table(0, index, access_token, true);
    oc_print_auth_at_entry(0, index);
    oc_init_oscore(0);

    subscribe_group_to_multicast(1, 16, 2);

    oscore_init = true;
  }

  oc_issue_s_mode(2, 6, 1, 1, 16, "w", 0, 0);

  return OC_EVENT_CONTINUE;
}

/**
 * test of decoding a message to myself
 */
static oc_event_callback_retval_t
issue_spake(void *data)
{

  PRINT("issue_spake\n");
  // oc_endpoint_t *my_ep = oc_connectivity_get_endpoints(0);

  oc_do_wk_discovery_all("ep=urn:knx:sn.123456789012", 2, discovery_cb, NULL);

  // int index = 0;
  // oc_initiate_spake(my_ep, "LETTUCE", "ABCD");

  return OC_EVENT_DONE;
}
/**
 * set a multicast s-mode message as delayed callback
 */
void
issue_requests_s_mode(void)
{
  PRINT(" issue_requests_s_mode\n");
  // oc_set_delayed_callback(NULL, issue_requests_s_mode_delayed, 2);
  oc_set_delayed_callback(NULL, issue_s_mode_secure, 2);
}

/**
 * prints the usage of the application
 */
void
print_usage()
{
  PRINT("Usage:\n");
  PRINT("none : starts the application as server (e.g. no client interaction) "
        "functionality)\n ");
  PRINT("-help : this message\n");
  PRINT("s-mode : does an event (to itself)\n");
  PRINT("oscore : spake2hand shake (to itself) & issue secure request to "
        "/dev/pm \n");
  PRINT("reset  : does an full reset of the device\n");
  exit(0);
}

/**
 * main application.
 * - initializes the global variables
 * - registers and starts the handler
 * - handles (in a loop) the next event.
 * - shuts down the stack
 *
 * @param argc the number of arguments.
 * @param argv the argument list
 */
int
main(int argc, char *argv[])
{
  int init;

  bool do_send_s_mode = false;
  bool do_send_oscore = false;
  bool do_test_myself = false;
  // false;
  true; //  false;
  g_reset = true;

  oc_clock_time_t next_event;

  for (int i = 0; i < argc; i++) {
    PRINT("argv[%d] = %s\n", i, argv[i]);
  }
  if (argc > 1) {
    PRINT("arg[1]: %s\n", argv[1]);
    if (strcmp(argv[1], "s-mode") == 0) {
      do_send_s_mode = true;
      PRINT(" smode: %d\n", do_send_s_mode);
    }
    if (strcmp(argv[1], "oscore") == 0) {
      do_send_oscore = true;
      PRINT(" oscore: %d\n", do_send_oscore);
    }
    if (strcmp(argv[1], "reset") == 0) {
      PRINT(" internal reset\n");
      g_reset = true;
    }
    if (strcmp(argv[1], "-help") == 0) {
      print_usage();
    }
  }

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

  PRINT("KNX-IOT Server name : \"testserver_all\"\n");

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
  PRINT("\tstorage at './testserver_all_creds' \n");
  oc_storage_config("./testserver_all_creds");

#ifdef OC_SECURITY
  PRINT("Security - Enabled\n");
#else
  PRINT("Security - Disabled\n");
#endif /* OC_SECURITY */

#ifdef OC_OSCORE
  PRINT("OC_OSCORE - Enabled\n");
#else
  PRINT("OC_OSCORE - Disabled\n");
#endif /* OC_OSCORE */

  /*initialize the variables */
  initialize_variables();

  /* initializes the handlers structure */
  oc_handler_t handler = { .init = app_init,
                           .signal_event_loop = signal_event_loop,
                           .register_resources = register_resources
#ifdef OC_CLIENT
                           ,
                           .requests_entry = 0
#endif
  };
#ifdef OC_CLIENT
  if (do_send_s_mode) {
    handler.requests_entry = issue_requests_s_mode;
  }
#endif

#ifdef OC_CLIENT
  // oc_set_delayed_callback(NULL, issue_requests_s_mode_delayed, 2);
  if (do_send_oscore) {
    handler.requests_entry = issue_requests_oscore;
  }
  if (do_send_oscore) {
    handler.requests_entry = issue_requests_oscore;
  }

#endif

#ifdef OC_CLIENT
  // if (do_test_myself) {
  //  handler.requests_entry = issue_s_mode_secure;
  //}
  // oc_set_delayed_callback(NULL, schedule_spake, 2);
#endif

  char *fname = "myswu_app";

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

  oc_device_info_t *device = oc_core_get_device_info(0);
  PRINT("serial number: %s\n", oc_string(device->serialnumber));
  device->pm = 1;

  oc_endpoint_t *my_ep = oc_connectivity_get_endpoints(0);
  if (my_ep != NULL) {
    PRINTipaddr(*my_ep);
    PRINT("\n");
    // my_ep = my_ep->next;
  }

  PRINT("Server \"testserver_all\" running (polling), waiting on incoming "
        "connections.\n\n\n");

  oc_set_delayed_callback(NULL, issue_spake, 2);

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
