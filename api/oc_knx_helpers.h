/*
// Copyright (c) 2023 Cascoda Ltd.
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
/**
  @brief generic knx helpers
  @file
*/
#ifndef OC_KNX_HELPERS_H
#define OC_KNX_HELPERS_H

#include "oc_api.h"
#include "oc_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef char ser_num[6];


/**
 * @brief helper function to check if query parameter l exists
 *
 * @param request the request
 * @param ps_exists return value if l=ps exists
 * @param total_exists return value if l=total exists
 * @return int 1 == l exists (with either ps or total or both)
 */
int check_if_query_l_exist(oc_request_t *request, bool *ps_exists,
                           bool *total_exists);

/**
 * @brief helper function to frame url part of query response:
 * <url?l=ps>
 * <url?l=total>
 * <url?l=ps;ps=total>
 *
 * @param url the url to be framed
 * @param ps_exists frame ps
 * @param total_exists frame total
 * @return total bytes framed
 */
int oc_frame_query_l(char *url, bool ps_exists, bool total_exists);

/**
 * @brief helper function to frame an integer in the response:
 * @param value the value to be framed, max 9 chars
 * @return total bytes framed
 */
int oc_frame_integer(int value);

/**
 * @brief helper function to convert the string serial number to 6 bytes
 * see: 3.30 Datapoint Type DPT_SerNum
 * @param value the input string (e.g. serial number in hex (string)
 * @return array of 6 bytes
 */
int oc_knx_serial_number_to_array(char *sn_string, ser_num my_serialNumber);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_HELPERS_H */