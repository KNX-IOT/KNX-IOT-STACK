/*
// Copyright (c) 2016 Intel Corporation
// Copyright (c) 2022 Cascoda Ltd.
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
  @brief platform abstraction of logging
  @file

  generic logging functions:
  - OC_LOGipaddr
    prints the endpoint information to stdout
  - OC_LOGbytes
    prints the bytes to stdout
  - OC_DBG
    prints information as Debug level
  - OC_WRN
    prints information as Warning level
  - OC_ERR
    prints information as Error level

  compile flags:
  - OC_DEBUG
    enables output of logging functions
  - OC_NO_LOG_BYTES
    disables output of OC_LOGbytes logging function
    if OC_DEBUG is enabled.
  - OC_LOG_TO_FILE
    logs the PRINT statements to file
*/
#ifndef OC_LOG_H
#define OC_LOG_H

#include <stdio.h>
#include <string.h>
#ifdef WIN32
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifdef __ANDROID__
#include "android/oc_log_android.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef OC_PRINT
#ifdef __ANDROID__
#define TAG "OC-JNI"
#define PRINT(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#else
#ifdef OC_LOG_TO_FILE
// logging to file
#define PRINT(...) oc_file_print(__VA_ARGS__)
#else
#define PRINT(...) printf(__VA_ARGS__)
#endif
#endif
#else
#define PRINT(...)
#endif

#ifdef OC_PRINT_APP
#define PRINT_APP(...) printf(__VA_ARGS__)
#else
#define PRINT_APP(...)
#endif

#define SPRINTF(...) sprintf(__VA_ARGS__)
#define SNPRINTF(...) snprintf(__VA_ARGS__)

#define PRINTipaddr(endpoint)                                                  \
  do {                                                                         \
    const char *scheme = "coap";                                               \
    if ((endpoint).flags & SECURED)                                            \
      scheme = "coaps";                                                        \
    if ((endpoint).flags & TCP)                                                \
      scheme = "coap+tcp";                                                     \
    if ((endpoint).flags & TCP && (endpoint).flags & SECURED)                  \
      scheme = "coaps+tcp";                                                    \
    if ((endpoint).flags & IPV4) {                                             \
      PRINT("%s://%d.%d.%d.%d:%d", scheme, ((endpoint).addr.ipv4.address)[0],  \
            ((endpoint).addr.ipv4.address)[1],                                 \
            ((endpoint).addr.ipv4.address)[2],                                 \
            ((endpoint).addr.ipv4.address)[3], (endpoint).addr.ipv4.port);     \
    } else {                                                                   \
      PRINT(                                                                   \
        "%s://[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%"    \
        "02x:%"                                                                \
        "02x%"                                                                 \
        "02x]:%d",                                                             \
        scheme, ((endpoint).addr.ipv6.address)[0],                             \
        ((endpoint).addr.ipv6.address)[1], ((endpoint).addr.ipv6.address)[2],  \
        ((endpoint).addr.ipv6.address)[3], ((endpoint).addr.ipv6.address)[4],  \
        ((endpoint).addr.ipv6.address)[5], ((endpoint).addr.ipv6.address)[6],  \
        ((endpoint).addr.ipv6.address)[7], ((endpoint).addr.ipv6.address)[8],  \
        ((endpoint).addr.ipv6.address)[9], ((endpoint).addr.ipv6.address)[10], \
        ((endpoint).addr.ipv6.address)[11],                                    \
        ((endpoint).addr.ipv6.address)[12],                                    \
        ((endpoint).addr.ipv6.address)[13],                                    \
        ((endpoint).addr.ipv6.address)[14],                                    \
        ((endpoint).addr.ipv6.address)[15], (endpoint).addr.ipv6.port);        \
    }                                                                          \
  } while (0)

#define PRINTipaddr_flags(endpoint)                                            \
  do {                                                                         \
    if ((endpoint).flags & SECURED) {                                          \
      PRINT(" Secured ");                                                      \
    };                                                                         \
    if ((endpoint).flags & MULTICAST) {                                        \
      PRINT(" MULTICAST ");                                                    \
    };                                                                         \
    if ((endpoint).flags & TCP) {                                              \
      PRINT(" TCP ");                                                          \
    };                                                                         \
    if ((endpoint).flags & IPV4) {                                             \
      PRINT(" IPV4 ");                                                         \
    };                                                                         \
    if ((endpoint).flags & IPV6) {                                             \
      PRINT(" IPV6 ");                                                         \
    };                                                                         \
    if ((endpoint).flags & OSCORE) {                                           \
      PRINT(" OSCORE ");                                                       \
    };                                                                         \
    if ((endpoint).flags & ACCEPTED) {                                         \
      PRINT(" ACCEPTED ");                                                     \
    };                                                                         \
    if ((endpoint).flags & OSCORE_DECRYPTED) {                                 \
      PRINT(" OSCORE_DECRYPTED ");                                             \
    };                                                                         \
    PRINT(" \n");                                                              \
  } while (0)

#define PRINTipaddr_local(endpoint)                                            \
  do {                                                                         \
    const char *scheme = "coap";                                               \
    if ((endpoint).flags & SECURED)                                            \
      scheme = "coaps";                                                        \
    if ((endpoint).flags & TCP)                                                \
      scheme = "coap+tcp";                                                     \
    if ((endpoint).flags & TCP && (endpoint).flags & SECURED)                  \
      scheme = "coaps+tcp";                                                    \
    if ((endpoint).flags & IPV4) {                                             \
      PRINT("%s://%d.%d.%d.%d:%d", scheme,                                     \
            ((endpoint).addr_local.ipv4.address)[0],                           \
            ((endpoint).addr_local.ipv4.address)[1],                           \
            ((endpoint).addr_local.ipv4.address)[2],                           \
            ((endpoint).addr_local.ipv4.address)[3],                           \
            (endpoint).addr_local.ipv4.port);                                  \
    } else {                                                                   \
      PRINT(                                                                   \
        "%s://[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%"    \
        "02x:%"                                                                \
        "02x%"                                                                 \
        "02x]:%d",                                                             \
        scheme, ((endpoint).addr_local.ipv6.address)[0],                       \
        ((endpoint).addr_local.ipv6.address)[1],                               \
        ((endpoint).addr_local.ipv6.address)[2],                               \
        ((endpoint).addr_local.ipv6.address)[3],                               \
        ((endpoint).addr_local.ipv6.address)[4],                               \
        ((endpoint).addr_local.ipv6.address)[5],                               \
        ((endpoint).addr_local.ipv6.address)[6],                               \
        ((endpoint).addr_local.ipv6.address)[7],                               \
        ((endpoint).addr_local.ipv6.address)[8],                               \
        ((endpoint).addr_local.ipv6.address)[9],                               \
        ((endpoint).addr_local.ipv6.address)[10],                              \
        ((endpoint).addr_local.ipv6.address)[11],                              \
        ((endpoint).addr_local.ipv6.address)[12],                              \
        ((endpoint).addr_local.ipv6.address)[13],                              \
        ((endpoint).addr_local.ipv6.address)[14],                              \
        ((endpoint).addr_local.ipv6.address)[15],                              \
        (endpoint).addr_local.ipv6.port);                                      \
    }                                                                          \
  } while (0)

#define IPADDR_BUFF_SIZE 64 // max size : scheme://[ipv6]:port = 59 bytes

#define SNPRINTFipaddr(str, size, endpoint)                                    \
  do {                                                                         \
    const char *scheme = "coap";                                               \
    if ((endpoint).flags & SECURED)                                            \
      scheme = "coaps";                                                        \
    if ((endpoint).flags & TCP)                                                \
      scheme = "coap+tcp";                                                     \
    if ((endpoint).flags & TCP && (endpoint).flags & SECURED)                  \
      scheme = "coaps+tcp";                                                    \
    memset(str, 0, size);                                                      \
    if ((endpoint).flags & IPV4) {                                             \
      SNPRINTF(str, size, "%s://%d.%d.%d.%d:%d", scheme,                       \
               ((endpoint).addr.ipv4.address)[0],                              \
               ((endpoint).addr.ipv4.address)[1],                              \
               ((endpoint).addr.ipv4.address)[2],                              \
               ((endpoint).addr.ipv4.address)[3], (endpoint).addr.ipv4.port);  \
    } else {                                                                   \
      SNPRINTF(                                                                \
        str, size,                                                             \
        "%s://"                                                                \
        "[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:"     \
        "%02x%02x]:%d",                                                        \
        scheme, ((endpoint).addr.ipv6.address)[0],                             \
        ((endpoint).addr.ipv6.address)[1], ((endpoint).addr.ipv6.address)[2],  \
        ((endpoint).addr.ipv6.address)[3], ((endpoint).addr.ipv6.address)[4],  \
        ((endpoint).addr.ipv6.address)[5], ((endpoint).addr.ipv6.address)[6],  \
        ((endpoint).addr.ipv6.address)[7], ((endpoint).addr.ipv6.address)[8],  \
        ((endpoint).addr.ipv6.address)[9], ((endpoint).addr.ipv6.address)[10], \
        ((endpoint).addr.ipv6.address)[11],                                    \
        ((endpoint).addr.ipv6.address)[12],                                    \
        ((endpoint).addr.ipv6.address)[13],                                    \
        ((endpoint).addr.ipv6.address)[14],                                    \
        ((endpoint).addr.ipv6.address)[15], (endpoint).addr.ipv6.port);        \
    }                                                                          \
  } while (0)

#define SNPRINTFbytes(buff, size, data, len)                                   \
  do {                                                                         \
    char *beg = (buff);                                                        \
    char *end = (buff) + (size);                                               \
    for (size_t i = 0; beg <= (end - 3) && i < (len); i++) {                   \
      beg += (i == 0) ? SPRINTF(beg, "%02x", (data)[i])                        \
                      : SPRINTF(beg, ":%02x", (data)[i]);                      \
    }                                                                          \
  } while (0)

#define OC_LOG(level, ...)                                                     \
  do {                                                                         \
    PRINT("%s: %s <%s:%d>: ", level, __FILENAME__, __func__, __LINE__);        \
    PRINT(__VA_ARGS__);                                                        \
    PRINT("\n");                                                               \
  } while (0)

#define OC_LOGbytes_internalxx(prefix, bytes, length)                          \
  do {                                                                         \
    PRINT("%s: %s <%s:%d>:\n", prefix, __FILENAME__, __func__, __LINE__);      \
    uint16_t i;                                                                \
    for (i = 0; i < (length); i++)                                             \
      PRINT(" %02X", (bytes)[i]);                                              \
    PRINT("\n");                                                               \
  } while (0)

#define OC_LOGbytes_internal(prefix, bytes, length)                            \
  do {                                                                         \
    uint16_t i;                                                                \
    for (i = 0; i < (length); i++)                                             \
      PRINT(" %02X", (bytes)[i]);                                              \
    PRINT("\n");                                                               \
  } while (0)

#ifdef OC_DEBUG
#ifdef __ANDROID__
#define OC_LOG(level, ...)                                                     \
  android_log(level, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define OC_LOGipaddr(endpoint)                                                 \
  android_log_ipaddr("DEBUG", __FILE__, __func__, __LINE__, endpoint)
#define OC_LOGbytes(bytes, length)                                             \
  android_log_bytes("DEBUG", __FILE__, __func__, __LINE__, bytes, length)
#else /* ! __ANDROID */
/*
#define OC_LOG(level, ...)                                                     \
  do {                                                                         \
    PRINT("%s: %s <%s:%d>: ", level, __FILENAME__, __func__, __LINE__);        \
    PRINT(__VA_ARGS__);                                                        \
    PRINT("\n");                                                               \
  } while (0)
*/

#define OC_LOGipaddr(endpoint)                                                 \
  do {                                                                         \
    PRINT("DEBUG: %s <%s:%d>: ", __FILENAME__, __func__, __LINE__);            \
    PRINTipaddr(endpoint);                                                     \
    PRINT("\n");                                                               \
  } while (0)

#ifndef OC_NO_LOG_BYTES
#define OC_LOGbytes(bytes, length)                                             \
  do {                                                                         \
    PRINT("D: %s <%s:%d>: ", __FILENAME__, __func__, __LINE__);                \
    uint16_t i;                                                                \
    for (i = 0; i < (length); i++)                                             \
      PRINT(" %02X", (bytes)[i]);                                              \
    PRINT("\n");                                                               \
  } while (0)
#else
#endif /* NO_LOG_BYTES */
#endif /* __ANDROID__ */

#define OC_DBG(...) OC_LOG("D", __VA_ARGS__)
#define OC_WRN(...) OC_LOG("W", __VA_ARGS__)
#define OC_ERR(...) OC_LOG("E", __VA_ARGS__)

#else
// #define OC_LOG(...)
#define OC_DBG(...)
//#define OC_WRN(...)
//#define OC_ERR(...)
#define OC_LOGipaddr(endpoint)
#define OC_LOGbytes(bytes, length)
#endif

// always do OC_ERR and OC_WRN logs
#define OC_ERR(...) OC_LOG("E", __VA_ARGS__)
#define OC_WRN(...) OC_LOG("W", __VA_ARGS__)

#ifdef OC_DEBUG_OSCORE
#define OC_DBG_OSCORE(...) OC_LOG("OSCORE", __VA_ARGS__)
#define OC_DBG_SPAKE(...) OC_LOG("SPAKE", __VA_ARGS__)
#define OC_LOGbytes_OSCORE(bytes, length)                                      \
  OC_LOGbytes_internal("OSCORE", bytes, length)
#define OC_LOGbytes_SPAKE(bytes, length)                                       \
  OC_LOGbytes_internal("SPAKE", bytes, length)
#else
#define OC_DBG_OSCORE(...)
#define OC_DBG_SPAKE(...)
#define OC_LOGbytes_OSCORE(bytes, length)
#define OC_LOGbytes_SPAKE(bytes, length)
#endif

#ifdef __cplusplus
}
#endif

#endif /* OC_LOG_H */
