/*
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

#ifndef OC_KNX_FP_INTERNAL_H
#define OC_KNX_FP_INTERNAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief cflag masks
 *
 *
 *
 */
typedef enum {
  OC_CFLAG_NONE = 0,       ///< Communication
  OC_CFLAG_READ = 1 << 1,  ///< false = Group Object value cannot be read.
  OC_CFLAG_WRITE = 1 << 2, ///< false = Group Object value cannot be written
  OC_CFLAG_TRANSMISSION =
    1 << 3,                 ///< false = Group Object value is not transmitted.
  OC_CFLAG_UPDATE = 1 << 4, ///< false = Group Object value is not updated.
  OC_CFLAG_INIT = 1 << 5    ///< false = Disable read after initialization.
} oc_cflag_mask_t;

/**
 * @brief Group Object Table Resource (/fp/g)
 *
 * array of objects (as json)
 * [
 *    {
 *        "id": "1",
 *        "href":"/LDSB1/SOO",
 *        "ga":[2305, 2401],
 *        "cflag":["r","w","t","u"]
 *    },
 *    {
 *        "id": "2",
 *        "href":"/LDSB1/RSC",
 *        "ga":[2306],
 *        "cflag":["t"]
 *     }
 * ]
 *
 * cflag translation
 * | string | Integer Value |
 * | ------ | ------------- |
 * | r      | 1             |
 * | w      | 2             |
 * | t      | 3             |
 * | u      | 4             |
 * | i      | 5             |

 * Key translation
 * | Json Key | Integer Value |
 * | -------- | ------------- |
 * | id       | 0             |
 * | href     | 11            |
 * | ga       | 7             |
 * | cflag    | 8             |
 *
 */
typedef struct oc_group_object_table_t
{
  int id;                 ///< contents of id
  oc_string_t href;       ///<  contents of href
  int *ga;                ///< array of integers
  int ga_len;             //< length of the array of ga identifiers
  oc_cflag_mask_t cflags; ///< contents of cflags as bitmap
} oc_group_object_table_t;

/**
 * @brief Function point Recipient - Publisher Table Resource (/fp/r) (/fp/p)
 *
 * the same table is used for recipient and publisher.
 * the only difference is the confirmable/not confirmable sending.
 *
 * array of objects (as json)
 * [
 *    {
 *        "id": "1",
 *         ia": "<knx-installation-id>.<recipient�s IA>",
 *        "ga":[2305, 2401],
 *        "path": ".knx",
 *    },
 *    {
 *        "id": "2",
 *        "url": "coap://<IP multicast, unicast address or fqdn>/<path>",
 *        "ga": [2305, 2306, 2307, 2308]
 *     }
 * ]
 *
 * Key translation
 * | Json Key | Integer Value |
 * | -------- | ------------- |
 * | id       | 0             |
 * | ia       | 12            |
 * | path     | 112           |
 * | url      | 10            |
 * | ga       | 7             |
 * | con      | -             |
 *
 */
typedef struct oc_group_rp_table_t
{
  int id;           ///< contents of id
  oc_string_t ia;   ///< contents of ia
  oc_string_t path; ///< contents of path
  oc_string_t url;  ///< contents of url
  bool con;         ///< confirmed message, default = false
  int *ga;          ///< array of integers
  int ga_len;       //< length of the array of ga identifiers
} oc_group_rp_table_t;

/**
 * @brief find (first) index in the group address table
 *
 * @param group_address the group address
 * @return int the index in the table or -1
 */
int oc_core_find_group_object_table_index(int group_address);

/**
 * @brief find next index in the group address table
 *
 * @param group_address the group address
 * @param cur_index  the current index to start from.
 * @return int the index in the table or -1
 */
int oc_core_find_next_group_object_table_index(int group_address,
                                               int cur_index);


/**
 * @brief find (first) index in the group address table via url
 *
 * @param url the url to find
 * @return int the index in the table or -1
 */
int oc_core_find_group_object_table_url(char* url);

/**
 * @brief find next index in the group address table via url
 *
 * @param  url the url to find
 * @param cur_index  the current index to start from.
 * @return int the index in the table or -1
 */
int oc_core_find_next_group_object_table_url(char* url,
                                               int cur_index);

/**
 * @brief find the url (of the resource) that in the group object table entry.
 *
 * @param index the index in the table
 * @return oc_string_t the url
 */
oc_string_t oc_core_find_group_object_table_url_from_index(int index);

int oc_core_find_group_object_table_number_group_entries(int index);


int oc_core_find_group_object_table_group_entry(int index, int entry);



// these are needed for the system with the broker
int oc_core_find_reciepient_table_index(int group_address);
oc_string_t oc_core_find_reciepient_table_url_from_index(int index);

/**
@brief Creation of the KNX feature point resources.

@param device index of the device to which the resource are to be created
*/
void oc_create_knx_fp_resources(size_t device);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_FP_INTERNAL_H */
