#ifndef OC_CONFIG_H
#define OC_CONFIG_H

/* Time resolution */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// FOR TESTING
#define OC_INOUT_BUFFER_POOL 2 
#define OC_INOUT_BUFFER_SIZE (1232)

typedef uint64_t oc_clock_time_t;
#define strncasecmp _strnicmp
/* Sets one clock tick to 1 ms */
#define OC_CLOCK_CONF_TICKS_PER_SECOND (1000)

/* Security Layer */
/* Max inactivity timeout before tearing down DTLS connection */
#define OC_DTLS_INACTIVITY_TIMEOUT (300)

/* Maximum number of concurrent requests */
#define OC_MAX_NUM_CONCURRENT_REQUESTS (20)

/* Add support for passing network up/down events to the app */
#define OC_NETWORK_MONITOR
/* Add support for passing TCP/TLS/DTLS session connection events to the app */
#define OC_SESSION_EVENTS

/* Add support for dns lookup to the endpoint */
#define OC_DNS_LOOKUP
//#define OC_DNS_LOOKUP_IPV6

/* Add request history for deduplicate UDP/DTLS messages */
#define OC_REQUEST_HISTORY

// The maximum size of a response to an OBSERVE request, in bytes.
#define OC_MAX_OBSERVE_SIZE 512

#if !defined(OC_DYNAMIC_ALLOCATION)
#define OC_DYNAMIC_ALLOCATION
#endif /* OC_DYNAMIC_ALLOCATION */
#if !defined(OC_BLOCK_WISE)
#define OC_BLOCK_WISE
#endif /* OC_BLOCK_WISE */

/* Maximum number of callbacks for Network interface event monitoring */
#define OC_MAX_NETWORK_INTERFACE_CBS (2)

/* Maximum number of callbacks for connection of session */
#define OC_MAX_SESSION_EVENT_CBS (2)

/* library features that require persistent storage */
#ifdef OC_SECURITY
#define OC_STORAGE
#endif

#define OC_STORAGE

#ifdef __cplusplus
}
#endif

#endif /* OC_CONFIG_H */
