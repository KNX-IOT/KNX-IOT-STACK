# Introduction

This folder contains code examples.

The intention of the examples is to explain certain aspects of the stack.
e.g. provide information in how to build an KNX-IOT device based on the stack.

naming convention:

- [filename]**_all** has code specific for linux and Windows OS

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
- example to receive data to other device

### testclient_all.c

Test Client on Windows & Linux.

- KNX client application
   example to send data to other device
- can discover devices through well-known/core
- can send s-mode multicast message (on all coap nodes)

### LSAB_minimal_all.c

KNX-IOT example on Windows & Linux.
capable of receiving commands for datapoint 417.61
over multicast "all coap nodes"

- Implements FB LSAB 417
  - only datapoint 61
  - e.g. dpa: 417.61

Note: can be configured to receive commands from LSSB_minimal_all.

uses pimoroni displayotron hat :

https://pinout.xyz/pinout/display_o_tron_hat?msclkid=02fa4484c6d511ecaaaf64d47a2d5e81

https://github.com/pimoroni/displayotron

### LSSB_minimal_all.c

KNX-IOT example on Windows & Linux.
capable of sending commands from datapoint 421.61
over multicast "all coap nodes"

- Implements FB LSSB 421
  - only datapoint 61
  - e.g. dpa: 421.61

Note: can be configured to send commands to LSAB_minimal_all.

## support applications

### simpleclient_displayotron.c

example code for the displayotron pi hat.
Works on Linux (Pi) with the pi-hat mounted.
example functionallity:

- reads out the buttons on the pi-hat
