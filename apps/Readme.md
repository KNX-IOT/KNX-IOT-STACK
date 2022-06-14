# Introduction

This folder contains code examples.

The intention of the examples is to explain certain aspects of the stack.
e.g. provide information in how to build an KNX IoT Point API device based on the stack.

naming convention:

- [filename]**_all** has code specific for Linux and Windows OS

## Example applications

### serial numbers

| Application       | serial number |
| ----------------- | ----------- |
| LSAB_minimal_all  | 000001 |
| LSSB_minimal_all  | 000003 |
| testserver_all    | 000005 |
| testclient_all    | 000006 |

### testserver_all.c

Test server on Windows & Linux.

- no KNX compliant application
- example to receive data from other device

### testclient_all.c

Test Client on Windows & Linux.

- no KNX compliant application
- KNX client application
   example to send data to other device
- can discover devices through well-known/core
- can send s-mode multicast message

### LSAB_minimal_all.c

KNX-IOT example on Windows & Linux.
capable of receiving commands for data point 417.61

- Implements Functional Block LSAB 417
  - only data point 61
  - e.g. dpa: 417.61
  - url : "p/o_1_1"

Note: can be configured to receive s-mode commands from LSSB_minimal_all.

### LSSB_minimal_all.c

KNX-IOT example on Windows & Linux.
capable of sending commands from data point 421.61

- Implements Functional Block LSSB 421
  - only data point 61
  - e.g. dpa: 421.61
  - url : "p/o_1_1"

Note: can be configured to send s-mode commands to LSAB_minimal_all.
Note: this application has no trigger mechanism to send the s-mode message
