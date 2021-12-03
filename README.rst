.. image:: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/build.yml/badge.svg
   :target: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/build.yml

.. image:: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/cmake-linux.yml/badge.svg
   :target: https://github.com/iKNX-IOT/KNX-IOT-STACK/actions/workflows/cmake-linux.yml

.. image:: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/cmake-windows.yml/badge.svg
   :target: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/cmake-windows.yml

.. image:: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/unittest.yml/badge.svg
   :target: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/unittest.yml

.. image:: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/doxygen.yml/badge.svg
   :target: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/doxygen.yml

.. image:: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/check-format.yml/badge.svg
   :target: https://github.com/KNX-IOT/KNX-IOT-STACK/actions/workflows/check-format.yml


Introduction
------------

KNX-IOT is an open-source, reference implementation of the KNX standards for the Internet of Things (IoT). 
Specifically, the stack realizes all the functionalities of the KNX-IOT specification.


.. image:: ./images/knxstack-v1.png
   :scale: 100%
   :alt: Architecture
   :align: center


Please review the following resources for more details:

The project was created to bring together the open-source community to accelerate the development of the framework and services required to connect the growing number of IoT devices. 
The  project offers device vendors and application developers royalty-free access  under the `Apache 2.0 license <https://github.com/KNX-IOT/KNX-IOT-STACK/blob/main/LICENSE.md>`_.

Stack features
-----------------------

- **OS agnostic:** The  device stack and modules work cross-platform (pure C code) and execute in an event-driven style. The stack interacts with lower level OS/hardware platform-specific functionality through a set of abstract interfaces. This decoupling of standards related functionality from platform adaptation code promotes ease of long-term maintenance and evolution of the stack through successive releases.

.. image:: ./images/porting.png
   :scale: 100%
   :alt: PortingLayer
   :align: center

- **Porting layer:** The platform abstraction is a set of generically defined interfaces which elicit a specific contract from implementations. The stack utilizes these interfaces to interact with the underlying OS/platform. The simplicity and boundedness of these interface definitions allow them to be rapidly implemented on any chosen OS/target. Such an implementation constitutes a "port".
- **Optional support for static memory:** On minimal environments lacking heap allocation functions, the stack may be configured to statically allocate all internal structures by setting a number of build-time parameters, which by consequence constrain the allowable workload for an application.



Project directory structure
---------------------------

api/*
  contains the implementations of client/server APIs, the resource model,
  utility and helper functions to encode/decode
  to/from data model, module for encoding and interpreting type 4
  UUIDs, base64 strings, endpoints, and handlers for the discovery, platform
  and device resources.

messaging/coap/*
  contains a tailored CoAP implementation.

security/*
  contains resource handlers that implement the security model.

utils/*
  contains a few primitive building blocks used internally by the core
  framework.


deps/*
  contains external project dependencies.

deps/tinycbor/*
  contains the tinyCBOR sources.

deps/mbedtls/*
  contains the mbedTLS sources.

patches/*
  contains patches for deps/mbedTLS and need to be applied once.

include/*
  contains all common headers.

include/oc_api.h
  contains client/server APIs.

include/oc_rep.h
  contains helper functions to encode/decode to/from cbor

include/oc_helpers.h
  contains utility functions for allocating strings and
  arrays either dynamically from the heap or from pre-allocated
  memory pools.


port/\*.h
  collectively represents the platform abstraction.

port/<OS>/*
  contains adaptations for each OS.

apps/*
  contains sample  applications.


Build instructions
------------------

Grab source and dependencies using:

``git clone --recursive https://github.com/KNX-IOT/KNX-IOT_STACK.git``

Please check here for build instructions:




Send Feedback
-------------------------------------------------
Questions
`Wiki <https://github.com/KNX-IOT/KNX-IOT-STACK/wiki>`_

Bugs
`Github issues <https://github.com/KNX-IOT/KNX-IOT-STACK/issues>`_
