/*
// Copyright (c) 2021-2022 Cascoda Ltd
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
  @brief knx /fp resource implementations
  @file
*/
#ifndef OC_KNX_FP_INTERNAL_H
#define OC_KNX_FP_INTERNAL_H

#include <stddef.h>
#include "oc_helpers.h"
#include "oc_ri.h"

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

/**
 * @brief print the communication flags to standard output
 * communication flags in ASCII e.g. "w" "r" "i" "t" "u" without quotes
 *
 * @param cflags the communication flags
 */
void oc_print_cflags(oc_cflag_mask_t cflags);

/**
 * @brief adds the communication flags a preallocated buffer

 * cflags in ASCII e.g. "w" "r" "i" "t" "u" without quotes
 * if the flag does not exist, then a "." will be added instead
 *
 * @param buffer the string buffer to add the cflags too
 * @param cflags The communication flags
 */
void oc_cflags_as_string(char *buffer, oc_cflag_mask_t cflags);

/**
 * @brief Group Object Table Resource (/fp/g)
 * The payload is an array of objects.
 * Example (JSON):
 * ```
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
 * ```
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
 * - delete an index, e.g. delete the array entry of data (persistent)
 * - make the entry persistent
 * - free the data
 */
typedef struct oc_group_object_table_t
{
  int id;                 /**< contents of id*/
  oc_string_t href;       /**< contents of href*/
  oc_cflag_mask_t cflags; /**< contents of cflags as bitmap*/
  int ga_len;             /**< length of the array of ga identifiers*/
  uint32_t *ga;           /**< array of group addresses (unsigned integers) */
} oc_group_object_table_t;

/**
 * @brief Function point Recipient - Publisher Table Resource (/fp/r) (/fp/p)
 *
 * the same table is used for recipient and publisher.
 * the only difference is the confirmable/not confirmable flag.
 * There will be 2 arrays of the structure to store the /fp/r or /fp/p data
 *
 * Example (JSON): array of objects
 * ```
 * [
 *    {
 *        "id": "1",
 *        "ia": 5,
 *        "ga":[2305, 2401],
 *        "path": "k",
 *    },
 *    {
 *        "id": "2",
 *        "url": "coap://<IP multicast, unicast address or fqdn>/<path>",
 *        "ga": [2305, 2306, 2307, 2308]
 *     }
 * ]
 * ```
 *
 * Key translation
 * | Json Key | Integer Value |
 * | -------- | ------------- |
 * | id       | 0             |
 * | ia       | 12            |
 * | iid      | 26            |
 * | fid      | 25            |
 * | grpid    | 13            |
 * | path     | 112           |
 * | url      | 10            |
 * | ga       | 7             |
 * | con      | -             |
 *
 * The structure stores the information.
 * The structure will be used as an array.
 * There are function to find:
 * - max amount of entries
 * - empty index in the array
 * - find the index with a specific id
 * - delete an index, e.g. delete the array entry of data
 * - make the entry persistent
 * - free up the allocated data
 * - return the structure at a specific index
 */
typedef struct oc_group_rp_table_t
{
  int id;           /**< contents of id*/
  int ia;           /**< contents of ia (internal address)*/
  int64_t iid;      /**< contents of installation id */
  int64_t fid;      /**< contents of fabric id */
  uint32_t grpid;   /**< the multicast group id */
  oc_string_t path; /**< contents of path, default path = ".knx"*/
  oc_string_t url;  /**< contents of url */
  oc_string_t at;   /**< Access token id. Reference to the security credentials
                       for unicast subscription encryption. */
  bool con;         /**< confirmed message, default = false*/
  uint32_t *ga;     /**< array of integers */
  int ga_len;       /**< length of the array of group addresses identifiers */
} oc_group_rp_table_t;

/**
 * @brief Prints a reduced version of the entries of the group publisher table:
 *        Only id, iid and grpid are printed for each entry.
 *
 * @return int 0 == success
 */
int oc_print_reduced_group_publisher_table(void);

/**
 * @brief Prints a reduced version of the entries of the group recipient table:
 *        Only id, iid and grpid are printed for each entry.
 *
 * @return int 0 == success
 */
int oc_print_reduced_group_recipient_table(void);

/**
 * @brief find id (cbor key 0) in the response
 *
 * @return int -1 : not found, > -1 : value found
 */
int oc_table_find_id_from_rep(oc_rep_t *object);

/**
 * @brief set an entry in the group object table
 *
 * @param index the index where to add the entry
 * @param entry the group object entry
 * @return int 0 == success
 */
int oc_core_set_group_object_table(int index, oc_group_object_table_t entry);

/**
 * @brief retrieve the group object table total size,
 * e.g. the number of entries that can be stored
 *
 * @return int the total number of entries
 */
int oc_core_get_group_object_table_total_size();

/**
 * @brief retrieve the group object table entry
 *
 * Note that always the group object table is returned.
 * regardless if the data is valid or not.
 *
 * To check if the data is valid, please check if
 * ga_len > 0, if ga_len <= 0 then the group object table does
 * not contain an entry.
 *
 * @param index the index in the group object table
 * @return oc_group_object_table_t* pointer to the entry
 */
oc_group_object_table_t *oc_core_get_group_object_table_entry(int index);

/**
 * @brief find empty slot in group object table
 *
 * @param id the index
 * @return the free index or -1 when no empty slots are available
 */
int find_empty_slot_in_group_object_table(int id);

/**
 * @brief register the multicast addresses to listen to
 *
 * The addresses are formed from:
 * - situation 1) grpid of the publisher entries
 * or if no grpid exist:
 * - situation 2) group address entries in the Group Object table
 *
 * for situation 1):
 * - loop over the group object table
 *   - for each group address entry
 *     - if cflags is "Write" "Update" "Read"
 *       - find the grpid entry in the publisher table
 *       - register the grpid as part of the address
 *
 * for situation 2):
 * - loop over the group object table
 *   - for each group address entry
 *     - if cflags is "Write" "Update" "Read"
 *       - register the  group address as part of the address
 *
 * function is called when the device is (re)started in run-time mode (e.g.
 * state = "loaded")
 */
void oc_register_group_multicasts();

/**
 * @brief find the grpid from the group_address in the publisher table
 *
 * @see oc_register_group_multicasts
 *
 * @param group_address The group_address from the group object table
 * @return the grpid matching the group_address the table publisher table
 *  or 0 if not found
 */
uint32_t oc_find_grpid_in_publisher_table(uint32_t group_address);

/**
 * @brief find the grpid from the group_address in the recipient table
 *
 * @see oc_register_group_multicasts
 *
 * @param group_address The group_address from the group object table
 * @return the grpid matching the group_address the table publisher table
 *  or 0 if not found
 */
uint32_t oc_find_grpid_in_recipient_table(uint32_t group_address);

/**
 * @brief initializes the data points at initialization
 * e.g. sends out an read s-mode message when the I flag is set.
 *
 */
void oc_init_datapoints_at_initialization();

/**
 * @brief find index belonging to the id
 *
 * @param id the identifier of the entry
 * @return int the index in the table or -1
 */
int oc_core_find_index_in_group_object_table_from_id(int id);

/**
 * @brief find (first) index in the group address table
 *
 * @param group_address the group address
 * @return int the index in the table or -1
 */
int oc_core_find_group_object_table_index(uint32_t group_address);

/**
 * @brief find next index in the group address table
 *
 * @param group_address the group address
 * @param cur_index  the current index to start from.
 * @return int the index in the table or -1
 */
int oc_core_find_next_group_object_table_index(uint32_t group_address,
                                               int cur_index);

/**
 * @brief find (first) index in the group address table via url
 *
 * @param url The url to find
 * @return int The index in the table or -1
 */
int oc_core_find_group_object_table_url(const char *url);

/**
 * @brief find next index in the group address table via url
 *
 * @param  url The url to find
 * @param cur_index  The current index to start from.
 * @return int The index in the table or -1
 */
int oc_core_find_next_group_object_table_url(const char *url, int cur_index);

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
                                                    uint32_t group_address);

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
 * @return uint32_t 0 does not exit otherwise the ia
 */
uint32_t oc_core_get_recipient_ia(int index);

/**
 * @brief return the size of the recipient table
 *
 * @return int the size of the table
 */
int oc_core_get_recipient_table_size();

/**
 * @brief retrieve the recipient table entry
 *
 * Note that always the group object table is returned.
 * regardless if the data is valid or not.
 *
 * To check if the data is valid, please check if
 * ga_len > 0, if ga_len <= 0 then the group object table does
 * not contain an entry.
 *
 * @param index the index in the recipient table
 * @return oc_group_rp_table_t* pointer to the entry
 */
oc_group_rp_table_t *oc_core_get_recipient_table_entry(int index);

/**
 * @brief find empty slot in recipient table
 *
 * @param id : supply 0
 * @return -1 : no empty slot, otherwise index of empty slot
 */
int oc_core_find_empty_slot_in_recipient_table(int id);

/**
 * @brief find index of id in recipient table
 *
 * @param id index to find
 * @return -1 not found, otherwise index in recipient table
 */
int oc_core_find_index_in_recipient_table_from_id(int id);

/**
 * @brief add recipient entry
 *
 * @param index The index in the table
 * @param entry The entry to be added
 * @return 0 : successful
 */
int oc_core_add_recipient_entry(int index, oc_group_rp_table_t entry);

/**
 * @brief return the size of the publisher table
 *
 * @return int the size of the table
 */
int oc_core_get_publisher_table_size();

/**
 * @brief retrieve the publisher table entry
 *
 * Note that always the group object table is returned.
 * regardless if the data is valid or not.
 *
 * To check if the data is valid, please check if
 * ga_len > 0, if ga_len <= 0 then the group object table does
 * not contain an entry.
 *
 * @param index the index in the publisher table
 * @return oc_group_rp_table_t* pointer to the entry
 */
oc_group_rp_table_t *oc_core_get_publisher_table_entry(int index);

/**
 * @brief add publisher entry
 *
 * @param index The index in the table
 * @param entry The entry to be added
 * @return 0 : successful
 */
int oc_core_add_publisher_entry(int index, oc_group_rp_table_t entry);
/**
 * @brief find empty slot in recipient table
 *
 * @param id : supply 0
 * @return -1 : no empty slot, otherwise index of empty slot
 */
int oc_core_find_empty_slot_in_publisher_table(int id);

/**
 * @brief find index of id in publisher table
 *
 * @param id index to find
 * @return -1 not found, otherwise index in recipient table
 */
int oc_core_find_index_in_publisher_table_from_id(int id);

/**
 * @brief add points to the well-known core discovery response
 *  when the request has query option
 * .well-known/core?d=urn:knx:g.s.[group-address]
 * @param request The request
 * @param device_index The device index
 * @param group_address the parsed group address from the query option
 * @param response_length the response length
 * @param matches if there are matches
 * @return true
 * @return false
 */
bool oc_add_points_in_group_object_table_to_response(oc_request_t *request,
                                                     size_t device_index,
                                                     uint32_t group_address,
                                                     size_t *response_length,
                                                     int matches);

/**
 * @brief checks if the href (url) belongs to the device
 *
 * @param href the url to be checked if it belongs to the device
 * @param discoverable checks only discoverable devices, e.g. belonging to a
 * function (/fp)
 * @param device_index The device index
 *
 * @return true
 * @return false
 */
bool oc_belongs_href_to_resource(oc_string_t href, bool discoverable,
                                 size_t device_index);

/**
 * @brief Creation of the KNX feature point resources.
 *
 * @param device_index index of the device to which the resource are to be
 * created
 */
void oc_create_knx_fp_resources(size_t device_index);

/**
 * @brief free the fp resources
 * e.g. frees up all allocated memory.
 *
 * @param device_index index of the device to which the resource are to be
 * freed.
 */
void oc_free_knx_fp_resources(size_t device_index);

/**
 * @brief create the group multi cast address
 * using the default port 5683
 *
 * @param in the endpoint to adapt
 * @param group_nr the group number
 * @param iid the installation id
 * @param scope the address scope
 * @return oc_endpoint_t the modified endpoint
 */
oc_endpoint_t oc_create_multicast_group_address(oc_endpoint_t in,
                                                uint32_t group_nr, int64_t iid,
                                                int scope);

/**
 * @brief create the group multi cast address with port
 *
 * create the multicast address from group and scope with a supplied port number
 *\code{.unparsed}
 * FF3_:FD__:____:____:(8-f)___:____
 * FF35:30:<ULA-routing-prefix>::<group id>
 *    | 5 == scope
 *    | 3 == scope
 * Multicast prefix: FF35:0030:  [4 bytes]
 * ULA routing prefix: FD11:2222:3333::  [6 bytes + 2 empty bytes]
 * Group Identifier: 8000 : 0068 [4 bytes ]
 *\endcode
 *
 * @param in the endpoint to adapt
 * @param group_nr the group number
 * @param iid the installation id
 * @param scope the address scope
 * @param port the port to be used
 * @return oc_endpoint_t the modified endpoint
 */
oc_endpoint_t oc_create_multicast_group_address_with_port(oc_endpoint_t in,
                                                          uint32_t group_nr,
                                                          int64_t iid,
                                                          int scope, int port);

/**
 * @brief subscribe to a multicast address, defined by group number and
 * installation id
 * using the default port 5683
 *
 * @see unsubscribe_group_to_multicast
 *
 * @param group_nr the group number (address)
 * @param iid the installation id
 * @param scope the address scope
 */
void subscribe_group_to_multicast(uint32_t group_nr, int64_t iid, int scope);

/**
 * @brief subscribe to a multicast address, defined by group number and
 * installation id
 *
 * @see unsubscribe_group_to_multicast_with_port
 *
 * @param group_nr the group number (address)
 * @param iid the installation id
 * @param scope the address scope
 * @param port the port
 */
void subscribe_group_to_multicast_with_port(uint32_t group_nr, int64_t iid,
                                            int scope, int port);

/**
 * @brief unsubscribe to a multicast address, defined by group number and
 * installation id
 *
 * @see subscribe_group_to_multicast
 *
 * @param group_nr the group number (address)
 * @param iid the installation id
 * @param scope the address scope
 */
void unsubscribe_group_to_multicast(uint32_t group_nr, int64_t iid, int scope);

/**
 * @brief unsubscribe to a multicast address, defined by group number and
 * installation id and port
 *
 * @see subscribe_group_to_multicast_with_port
 *
 * @param group_nr the group number (address)
 * @param iid the installation id
 * @param scope the address scope
 * @param port the port
 */
void unsubscribe_group_to_multicast_with_port(uint32_t group_nr, int64_t iid,
                                              int scope, int port);

#ifdef __cplusplus
}
#endif

#endif /* OC_KNX_FP_INTERNAL_H */
