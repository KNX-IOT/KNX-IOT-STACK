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
 * Group Object Table Resource (/fp/g)
 *
 */
typedef enum {
  OC_CFLAG_NONE = 0, /**< Communication */
  OC_CFLAG_COMMUNICATION =
    1 << 2, /**< false = Group Object value cannot read or written.*/
  OC_CFLAG_READ = 1 << 3, /**< 8 false = Group Object value cannot be read.*/
  OC_CFLAG_WRITE =
    1 << 4, /**< 16 false = Group Object value cannot be written.*/
  OC_CFLAG_INIT = 1 << 5, /**< 32 false = Disable read after initialization.*/
  OC_CFLAG_TRANSMISSION =
    1 << 6, /**< 64 false = Group Object value is not transmitted.*/
  OC_CFLAG_UPDATE =
    1 << 7, /**< 128 false = Group Object value is not updated.*/
} oc_cflag_mask_t;

void oc_print_cflags(oc_cflag_mask_t cflags);

/**
 * @brief Group Object Table Resource (/fp/g)
 *
 * the will be an array of objects (as json):
 * [
 *    {
 *        "id": "1",
 *        "href":"/LDSB1/SOO",
 *        "ga":[2305, 2401],
 *        "cflag":["r","w","t","u"]  // note this is a integer
 *    },
 *    {
 *        "id": "2",
 *        "href":"/LDSB1/RSC",
 *        "ga":[2306],
 *        "cflag":["t"]  // note this is an integer
 *     }
 * ]
 *
 * cflag translation
 * | string | bit     |  value |
 * | ------ | ------- |--------|
 * | c      | 2       |  4     |
 * | r      | 3       |  8     |
 * | w      | 4       |  16    |
 * | i      | 5       |  32    |
 * | t      | 6       |  64    |
 * | u      | 7       | 128    |

 * Key translation
 * | Json Key | Integer Value |
 * | -------- | ------------- |
 * | id       | 0             |
 * | href     | 11            |
 * | ga       | 7             |
 * | cflag    | 8             |
 *
 * The structure stores the information.
 * The structure will be used as an array.
 * There are function to find
 * - empty index in the array
 * - find the index with a specific id
 * - delete an index, e.g. free the array entry of data
 * - make the entry persistent
 */
typedef struct oc_group_object_table_t
{
  int id;                 /**< contents of id*/
  oc_string_t href;       /**<  contents of href*/
  oc_cflag_mask_t cflags; /**< contents of cflags as bitmap*/
  int ga_len;             /**< length of the array of ga identifiers*/
  int *ga;                /**< array of integers*/
} oc_group_object_table_t;

/**
 * @brief Function point Recipient - Publisher Table Resource (/fp/r) (/fp/p)
 *
 * the same table is used for recipient and publisher.
 * the only difference is the confirmable/not confirmable flag.
 * There will be 2 arrays of the structure to store the /fp/r or /fp/p data
 *
 * array of objects (as json)
 * [
 *    {
 *        "id": "1",
 *         ia": 5,
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
 * The structure stores the information.
 * The structure will be used as an array.
 * There are function to find:
 * - empty index in the array
 * - find the index with a specific id
 * - delete an index, e.g. free the array entry of data
 * - make the entry persistent
 */
typedef struct oc_group_rp_table_t
{
  int id;           /**< contents of id*/
  int ia;           /**< contents of ia (internal address)*/
  oc_string_t path; /**< contents of path, default path = ".knx"*/
  oc_string_t url;  /**< contents of url*/
  bool con;         /**< confirmed message, default = false*/
  int *ga;          /**< array of integers*/
  int ga_len;       /**< length of the array of ga identifiers*/
} oc_group_rp_table_t;

/**
 * @brief set an entry in teh group
 *
 * @param index the index where to add the entry
 * @param entry the group object entry
 * @return int 0 == success
 */
int oc_core_set_group_object_table(int index, oc_group_object_table_t entry);

/**
 * @brief register the group entries in the Group Object table
 * as multicast receive addresses
 *
 * function to be called when the device is (re)started in run-time mode
 */
void oc_register_group_multicasts();

/**
 * @brief find index belonging to the id
 *
 * @param the identifier of the entry
 * @return int the index in the table or -1
 */
int oc_core_find_index_in_group_object_table_from_id(int id);

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
int oc_core_find_group_object_table_url(char *url);

/**
 * @brief find next index in the group address table via url
 *
 * @param  url the url to find
 * @param cur_index  the current index to start from.
 * @return int the index in the table or -1
 */
int oc_core_find_next_group_object_table_url(char *url, int cur_index);

/**
 * @brief retrieve the cflags from the entry table
 *
 * @param index the index in the group object table
 * @return oc_cflag_mask_t the retrieved cflags
 */
oc_cflag_mask_t oc_core_group_object_table_cflag_entries(int index);

/**
 * @brief find the url (of the resource) that in the group object table entry.
 *
 * @param index the index in the table
 * @return oc_string_t the url
 */
oc_string_t oc_core_find_group_object_table_url_from_index(int index);

/**
 * @brief retrieve the number of group address entries for index
 *
 * @param index the index in the group address table
 * @return int the number of group addresses
 */
int oc_core_find_group_object_table_number_group_entries(int index);

/**
 * @brief get group address of index, and entry (e.g. list)
 *
 * @param index the entry in the group address table
 * @param entry the entry in the list of addresses at index
 * @return int the group address
 */
int oc_core_find_group_object_table_group_entry(int index, int entry);

/**
 * @brief print the entry in the Group Object Table
 *
 * @param entry the index of the entry in the Group Object Table
 */
void oc_print_group_object_table_entry(int entry);

/**
 * @brief dump the entry of the Group Object Table (to persistent) storage
 *
 * @param entry the index of the entry in the Group Object Table
 */
void oc_dump_group_object_table_entry(int entry);

/**
 * @brief load the entry of the Group Object Table (from persistent) storage
 *
 * @param entry the index of the entry in the Group Object Table
 */
void oc_load_group_object_table_entry(int entry);

/**
 * @brief load all entries of the Group Object Table (from persistent) storage
 *
 */
void oc_load_group_object_table();

/**
 * @brief delete entry of the Group Object Table
 * does not make the change persistent
 *
 * @param entry the index of the entry in the Group Object Table
 */
void oc_delete_group_object_table_entry(int entry);

/**
 * @brief delete all entries of the Group Object Table (from persistent) storage
 *
 */
void oc_delete_group_object_table();

/**
 * @brief delete all entries of the Recipient and Publisher Object Table (from
 * persistent) storage
 *
 */
void oc_delete_group_rp_table();

/**
 * @brief checks if the group address is part of the recipient table at index
 *
 * @param index the index in the recipient table
 * @param group_address the group address to check
 * @return true is part of the recipient entry
 * @return false is not part of the recipient entry
 */
bool oc_core_check_recipient_index_on_group_address(int index,
                                                    int group_address);

/**
 * @brief get the destination (path or url) of the recipient table at index
 *
 * @param index the index in the table
 * @return char* NULL or path or url of the destination
 */
char *oc_core_get_recipient_index_url_or_path(int index);

/**
 * @brief retrieve the internal address of the recipient in the table
 *
 * @param index the index number in the recipient table
 * @return int -1 does not exit otherwise the ia
 */
int oc_core_get_recipient_ia(int index);

/**
 * @brief return the size of the recipient table
 *
 * @return int the size of the table
 */
int oc_core_get_recipient_table_size();

/**
 * @brief add points to the well-known core discovery response
 *  when the request has query option
 * .well-known/core?d=urn:knx:g.s.[group-address].
 * @param request The request
 * @param device_index The device index
 * @param group_address the parsed group address from the query option
 * @param response_length the response lenght
 * @param matches if there are matches
 * @return true
 * @return false
 */
bool oc_add_points_in_group_object_table_to_response(oc_request_t *request,
                                                     size_t device_index,
                                                     int group_address,
                                                     size_t *response_length,
                                                     int matches);

/**
 * @brief Creation of the KNX feature point resources.
 *
 * @param device index of the device to which the resource are to be created
 */
void oc_create_knx_fp_resources(size_t device);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_FP_INTERNAL_H */
