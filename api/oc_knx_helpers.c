/*
 // Copyright (c) 2023 Cascoda Ltd
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

#include "oc_api.h"
#include "oc_knx_helpers.h"

int
check_if_query_l_exist(oc_request_t *request, bool *ps_exists,
                       bool *total_exists)
{
  if (ps_exists == NULL) {
    return 0;
  }
  if (total_exists == NULL) {
    return 0;
  }

  *ps_exists = false;
  *total_exists = false;

  // handle query parameters
  if (oc_query_values_available(request)) {
    // check if l exist
    if (oc_query_value_exists(request, "l")) {
      // find out if l=ps and or l=total
      bool more_query_params;
      char *value = NULL;
      int value_len = -1;
      oc_init_query_iterator();
      do {
        more_query_params =
          oc_iterate_query_get_values(request, "l", &value, &value_len);
        if (value_len == 2) {
          if (strncmp("ps", value, value_len) == 0) {
            *ps_exists = true;
          }
        }
        if (value_len == 5) {
          if (strncmp("total", value, value_len) == 0) {
            *total_exists = true;
          }
        }
      } while (more_query_params);
    } /* query l exists */
  }   /* query available */

  if (*ps_exists == true) {
    return 1;
  }
  if (*total_exists == true) {
    return 1;
  }
  return 0;
}

int
oc_frame_query_l(char *url, bool ps_exists, bool total_exists)
{
  // example : < / fp / r / ? l = total>; total = 22; ps = 5
  // spec 1.1. : no query arguments anymore in the url
  // of the link format response line
  int response_length = 0;
  int length;

  length = oc_rep_add_line_to_buffer("<");
  response_length += length;
  length = oc_rep_add_line_to_buffer(url);
  response_length += length;
  length = oc_rep_add_line_to_buffer(">");
  response_length += length;

  /*
  if (ps_exists && total_exists) {
    length = oc_rep_add_line_to_buffer("?l=ps;l=total>");
    response_length += length;
  } else if (ps_exists) {
    length = oc_rep_add_line_to_buffer("?l=ps>");
    response_length += length;
  } else if (total_exists) {
    length = oc_rep_add_line_to_buffer("?l=total>");
    response_length += length;
  } else {
    length = oc_rep_add_line_to_buffer(">");
    response_length += length;
  }
  */

  return response_length;
}

int
check_if_query_pn_exist(oc_request_t *request, int *pn_value, int *ps_value)
{
  char *value = NULL;
  int value_len = -1;

  if (pn_value == NULL) {
    return 0;
  }
  if (ps_value == NULL) {
    return 0;
  }

  *ps_value = -1;
  *pn_value = -1;

  // handle query parameters
  if (oc_query_values_available(request)) {
    // check if pn exist
    if (oc_query_value_exists(request, "pn")) {
      oc_iterate_query_get_values(request, "pn", &value, &value_len);
      *pn_value = atoi(value);
    }
    oc_init_query_iterator();
    if (oc_query_value_exists(request, "ps")) {
      oc_iterate_query_get_values(request, "ps", &value, &value_len);
      *ps_value = atoi(value);
    }
  } /* query available */

  if (*ps_value > -1) {
    return 1;
  }
  if (*pn_value > -1) {
    return 1;
  }
  return 0;
}

int
oc_frame_integer(int value)
{
  char string[10];
  snprintf((char *)&string, 9, "%d", value);
  return oc_rep_add_line_to_buffer(string);
}
