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

#ifndef PAGE_SIZE
#define PAGE_SIZE 20
#endif

/**
 * @brief helper function to check if query parameter l exists
 *
 * example: /fp/r?l=total&l=ps
 * @param request the request
 * @param ps_exists return value if l=ps exists
 * @param total_exists return value if l=total exists
 * @return 1 == l exists (with either ps or total or both)
 * @return -1 == invalid l parameters
 * @return 0 == l doesn't exist
 */
int check_if_query_l_exist(oc_request_t *request, bool *ps_exists,
                           bool *total_exists);

/**
 * @brief helper function to frame url part of query response:
 * spec 1.0:
 * - <url?l=ps>
 * - <url?l=total>
 * - <url?l=ps;ps=total>
 * spec 1.1:
 * - url
 *
 * @param url the url to be framed
 * @param ps_exists frame ps
 * @param ps page size
 * @param total_exists frame total
 * @param total total items
 * @return total bytes framed
 */
int oc_frame_query_l(char *url, bool ps_exists, int ps, bool total_exists,
                     int total);

/**
 * @brief helper function to check if query parameter pn exists
 *
 * example: /dev/ipv6?pn=0&ps=3
 * @param request the request
 * @param pn_value return -1 if not exist otherwise value
 * @param ps_value return -1 if not exist otherwise value
 * @return true == pn exists
 */
bool check_if_query_pn_exist(oc_request_t *request, int *pn_value,
                             int *ps_value);

/**
 * @brief helper function to frame next page indicator, if more requests (pages)
 * are needed to get the full list
 *
 * @param url the url to be framed
 * @param next_page_num the next page number to be framed
 * @return total bytes framed
 */
int add_next_page_indicator(char *url, int next_page_num);

/**
 * @brief helper function to frame an integer in the response:
 * @param value the value to be framed, max 9 chars
 * @return total bytes framed
 */
int oc_frame_integer(int value);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_HELPERS_H */