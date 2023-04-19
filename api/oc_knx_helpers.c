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
  int response_length = 0;
  int length;

  length = oc_rep_add_line_to_buffer("<");
  response_length += length;
  length = oc_rep_add_line_to_buffer(url);
  response_length += length;
  if (ps_exists && total_exists) {
    length = oc_rep_add_line_to_buffer("?l=ps;l=total>");
    response_length += length;
  } else if (ps_exists) {
    length = oc_rep_add_line_to_buffer("?l=ps>");
    response_length += length;
  } else if (total_exists) {
    length = oc_rep_add_line_to_buffer("?l=total>");
    response_length += length;
  }

  return response_length;
}

int
oc_frame_integer(int value)
{
  char string[10];
  snprintf((char *)&string, 9, "%d", value);
  return oc_rep_add_line_to_buffer(string);
}


// makes a number from two ascii hexa characters
int
ahex2int(char a, char b)
{

  a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
  b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

  return (a << 4) + b;
}

int oc_knx_serial_number_to_array(char *sn_string, ser_num my_serialNumber)
{
  int i = 0;
  int sn_len = strlen(sn_string);
  // make sure everything is reset to zero..
  memset(my_serialNumber, 0, 6);

  if (sn_len >= 12) {
      my_serialNumber[5] = ahex2int(sn_string[10], sn_string[11]);
      my_serialNumber[4] = ahex2int(sn_string[8], sn_string[9]);
      my_serialNumber[3] = ahex2int(sn_string[6], sn_string[7]);
      my_serialNumber[2] = ahex2int(sn_string[4], sn_string[5]);
      my_serialNumber[1] = ahex2int(sn_string[2], sn_string[3]);
      my_serialNumber[0] = ahex2int(sn_string[0], sn_string[1]);
    } else {
    for (i = 0; i < (sn_len / 2); i++) {
        my_serialNumber[i] = ahex2int(sn_string[i*2], sn_string[(i*2) +1]);
      }
  }
  return 0;
}