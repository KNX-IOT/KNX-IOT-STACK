/*
// Copyright (c) 2017, 2020 Intel Corporation
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
  @brief end point implementation, e.g. IP(v6) addressing for sending &
  receiving data
  @file
*/
#ifndef OC_ENDPOINT_H
#define OC_ENDPOINT_H

#include "oc_helpers.h"
#include "oc_uuid.h"
#ifdef OC_OSCORE
#include "messaging/coap/oscore_constants.h"
#endif /* OC_OSCORE */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ipv6 data structure
 *
 */
typedef struct
{
  uint16_t port;       /**< port number */
  uint8_t address[16]; /**< address */
  uint8_t scope;       /**< scope of the address (multicast) */
} oc_ipv6_addr_t;

/**
 * @brief ipv4 data structure
 *
 */
typedef struct
{
  uint16_t port;      /**< port */
  uint8_t address[4]; /**< address */
} oc_ipv4_addr_t;

/**
 * @brief transport flags (bit map)
 * these flags are used to determine what to do on communication level
 */
enum transport_flags {
  DISCOVERY = 1 << 0,        /**< used for discovery */
  SECURED = 1 << 1,          /**< secure communication */
  IPV4 = 1 << 2,             /**< ipv4 communication */
  IPV6 = 1 << 3,             /**< ipv6 communication */
  TCP = 1 << 4,              /**< tcp communication */
  OSCORE = 1 << 5,           /**< OSCORE communication */
  MULTICAST = 1 << 6,        /**< multicast enabled */
  ACCEPTED = 1 << 7,         /**< accepted */
  OSCORE_DECRYPTED = 1 << 8, /**< OSCORE decrypted message */
  OSCORE_ENCRYPTED = 1 << 9, /**< OSCORE encrypted message */
};

#define SERIAL_NUM_SIZE (20)
/**
 * @brief the endpoint information
 *
 */
typedef struct oc_endpoint_t
{
  struct oc_endpoint_t *next;              /**< pointer to the next structure */
  size_t device;                           /**< device index */
  enum transport_flags flags;              /**< the transport flags */
  char serial_number[SERIAL_NUM_SIZE + 1]; /**< serial number of the device to
                                            talk to */
  union dev_addr {
    oc_ipv6_addr_t ipv6; /**< ipv6 address */
    oc_ipv4_addr_t ipv4; /**< ipv4 address */
  } addr, addr_local;
  int interface_index; /**< interface index */
  uint8_t priority;    /**< priority */
  uint32_t group_address; /**< group address,
                          being used to find back the OSCORE
                     credential to be used for encryption for s-mode messages
                     e.g. looping over the list of group addresses of the key */
#ifdef OC_OSCORE
  uint8_t piv[OSCORE_PIV_LEN]; /**< OSCORE partial iv */
  uint8_t piv_len;             /**< OSCORE partial iv length */
#endif                         /* OC_OSCORE */
} oc_endpoint_t;

#define oc_make_ipv4_endpoint(__name__, __flags__, __port__, ...)              \
  oc_endpoint_t __name__ = { .flags = __flags__,                               \
                             .addr.ipv4 = { .port = __port__,                  \
                                            .address = { __VA_ARGS__ } } }
#define oc_make_ipv6_endpoint(__name__, __flags__, __port__, ...)              \
  oc_endpoint_t __name__ = { .flags = __flags__,                               \
                             .device = 0,                                      \
                             .group_address = 0,                                    \
                             .addr.ipv6 = { .port = __port__,                  \
                                            .address = { __VA_ARGS__ } } }

/**
 * @brief create new endpoint
 *
 * @return oc_endpoint_t* created new endpoint
 */
oc_endpoint_t *oc_new_endpoint(void);

/**
 * @brief free endpoint
 *
 * @param endpoint the endpoint to be freed
 */
void oc_free_endpoint(oc_endpoint_t *endpoint);

/**
 * @brief set device serial number for the endpoint, e.g. the one to talk too
 *
 * @param endpoint the end point
 * @param serial_number the device serial number
 */
void oc_endpoint_set_serial_number(oc_endpoint_t *endpoint,
                                   char *serial_number);

/**
 * @brief convert the endpoint to a human readable string (e.g.
 * "coaps://[fe::22]:/")
 *
 * @param endpoint the endpoint
 * @param endpoint_str endpoint as human readable string
 * @return int 0 success
 */
int oc_endpoint_to_string(oc_endpoint_t *endpoint, oc_string_t *endpoint_str);

/**
 * @brief string to endpoint
 *
 * @param endpoint_str the endpoint as string (e.g. "coaps://[fe::22]:/blah")
 * @param endpoint the address part of the string
 * @param uri the uri part of the endpoint
 * @return int 0 success
 */
int oc_string_to_endpoint(oc_string_t *endpoint_str, oc_endpoint_t *endpoint,
                          oc_string_t *uri);

/**
 * @brief parse endpoint
 *
 * @param endpoint_str
 * @param path
 * @return int
 */
int oc_endpoint_string_parse_path(oc_string_t *endpoint_str, oc_string_t *path);

/**
 * @brief is endpoint (ipv6) link local
 *
 * @param endpoint the endpoint to check
 * @return int 0 = endpoint is link local
 */
int oc_ipv6_endpoint_is_link_local(oc_endpoint_t *endpoint);

/**
 * @brief compare endpoint
 *
 * @param ep1 endpoint 1 to compare
 * @param ep2 endpoint 2 to compare
 * @return int 0 = equal
 */
int oc_endpoint_compare(const oc_endpoint_t *ep1, const oc_endpoint_t *ep2);

/**
 * @brief compare address of the endpoint
 *
 * @param ep1 endpoint 1 to compare
 * @param ep2 endpoint 2 to compare
 * @return int 0 = equal
 */
int oc_endpoint_compare_address(const oc_endpoint_t *ep1,
                                const oc_endpoint_t *ep2);

/**
 * @brief set interface index on the endpoint
 *
 * @param ep the endpoint
 * @param interface_index the interface index
 */
void oc_endpoint_set_local_address(oc_endpoint_t *ep, int interface_index);

/**
 * @brief copy endpoint
 *
 * @param dst the destination endpoint
 * @param src the source endpoint
 */
void oc_endpoint_copy(oc_endpoint_t *dst, oc_endpoint_t *src);

/**
 * @brief copy list of endpoint
 *
 * @param dst the destination list of endpoints
 * @param src the source list of endpoints
 */
void oc_endpoint_list_copy(oc_endpoint_t **dst, oc_endpoint_t *src);

/**
 * @brief print the (first) endpoint to std out
 *
 * @param ep
 */
void oc_endpoint_print(oc_endpoint_t *ep);

#ifdef __cplusplus
}
#endif

#endif /* OC_ENDPOINT_H */
