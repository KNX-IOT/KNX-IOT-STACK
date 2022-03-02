# Introduction

This folder contains code examples.

The intention of the examples is to explain certain aspects of the stack.
e.g. provide information in how to build an KNX-IOT device based on the stack.

naming convention:

- [filename]**_all** has code specific for linux and Windows OS
- [filename]**_openthread** has code specific for usage on an openthread based platform
- [filename]**_pi** has code specific for usage on an Raspberry Pi platform (e.g. using a pi-hat).

## Example applications

### serial numbers

| Application       | serial number |
| ----------------- | ----------- |
| LSAB_minimal_all  | 000001 |
| LSAB_minimal_pi   | 000002 |
| LSSB_minimal_all  | 000003 |
| LSSB_minimal_pi   | 000004 |
| simpleserver_all  | 000005 |
| simpleclient_all  | 000006 |

### simpleserver_all.c

Server example on Windows & Linux.

- no KNX compliant application
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
over multicast "all coap nodes"

- Implements FB LSAB 417
  - only datapoint 61
  - e.g. dpa: 417.61

Note: can be configured to receive commands from LSSB_minimal_all.

### LSSB_minimal_all.c

KNX-IOT example on Windows & Linux.
capable of sending commands from datapoint 421.61
over multicast "all coap nodes"

- Implements FB LSSB 421
  - only datapoint 61
  - e.g. dpa: 421.61

Note: can be configured to send commands to LSAB_minimal_all.

### LSAB_minimal_pi.c

KNX-IOT example on Linux.
capable of receiving commands for datapoint 417.61
over multicast "all coap nodes"

- Implements FB LSAB 417
  - only datapoint 61
  - e.g. dpa: 417.61
  - the data point is connected to the LED, the led turns on/off
  
Note: can be configured to recieve commands from LSSB_minimal_pi.

### LSSB_minimal_pi.c

KNX-IOT example on Windows & Linux.
capable of sending commands from datapoint 421.61
e.g. uses the buttons to send the multicast messages
over multicast "all coap nodes"

- Implements FB LSSB 421
  - only datapoint 61
  - e.g. dpa: 421.61
  - the button is attached to the datapoint

Note: can be configured to send commands to LSAB_minimal_pi.

## support applications

### simpleclient_displayotron.c

example code for the displayotron pi hat.
Works on Linux (Pi) with the pi-hat mounted.
example functionallity:

- reads out the buttons on the pi-hat
