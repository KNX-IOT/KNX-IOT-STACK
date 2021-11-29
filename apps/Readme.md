# Introduction

This folder contains code examples.
The apps are intended to use in pairs, e.g. they are not full fledged applications.

The intention of the examples is to explain certain aspects of the stack.

naming convention:

- [filename]**_all* has code specific for linux and Windows OS
- [filename]**_openthread** has code specific for usage on an openthread based platform

## Example applications


### simpleserver_all.c

Server example on Windows & Linux.

- no KNX application
- example to receive data to other device

### simpleclient_all.c

Client example on Windows & Linux.

- KNX client application
   example to send data to other device
- can discover devices through well-known/core
- can send s-mode multicast message (on all coap nodes)


### LSAB_minimal_all.c

KNX-IOT example on Windows & Linux.
capable of receiving commands for datapoint 417.61

- Implements FB LSAB 417
  - only datapoint 61
  - e.g. dpa: 417.61

### LSSB_minimal_all.c

KNX-IOT example on Windows & Linux.
capable of sending commands from datapoint 421.61

- Implements FB LSSB 421
  - only datapoint 61
  - e.g. dpa: 421.61
