cmake_minimum_required (VERSION 3.10)
project(knx-iot-stack)

# Patch mbedtls
set(OC_REAPPLY_MBEDTLS_PATCHES ON CACHE BOOL "")
if(OC_REAPPLY_MBEDTLS_PATCHES)
    include(mbedtls-patch.cmake)
    set(OC_REAPPLY_MBEDTLS_PATCHES OFF CACHE BOOL 
        "By default, mbedTLS patches are applied upon the first CMake Configure. Set this to ON to reapply the patches on the next configure."
         FORCE
    )
endif()

# Do not build anything except for the library
option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
option(ENABLE_TESTING "Build mbed TLS tests." OFF)

set(OC_DYNAMIC_ALLOCATION_ENABLED ON CACHE BOOL "Enable dynamic memory allocation within the OCF stack and MBedtls.")
#set(OC_SECURITY_ENABLED ON CACHE BOOL "Enable security")
set(OC_PKI_ENABLED ON CACHE BOOL "Enable PKI security")
set(OC_DEBUG_ENABLED OFF CACHE BOOL "Enable debug messages")

# If an mbedtls platform layer is defined, add it to the mbedtls list of libs
if(TARGET mbedtls-plat)
    set(libs ${libs} mbedtls-plat)
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/deps/mbedtls)

target_include_directories(mbedcrypto PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/deps/mbedtls/include
)

if(TARGET mbedcrypto-plat)
	target_link_libraries(mbedcrypto mbedcrypto-plat)
endif()


# API
add_library(kis-api INTERFACE)

target_sources(kis-api INTERFACE
    ${PROJECT_SOURCE_DIR}/api/oc_base64.c
    ${PROJECT_SOURCE_DIR}/api/oc_blockwise.c
    ${PROJECT_SOURCE_DIR}/api/oc_buffer.c
    ${PROJECT_SOURCE_DIR}/api/oc_client_api.c
    ${PROJECT_SOURCE_DIR}/api/oc_clock.c
    ${PROJECT_SOURCE_DIR}/api/oc_core_res.c
    ${PROJECT_SOURCE_DIR}/api/oc_device_mode.c
    ${PROJECT_SOURCE_DIR}/api/oc_discovery.c
    ${PROJECT_SOURCE_DIR}/api/oc_endpoint.c
    ${PROJECT_SOURCE_DIR}/api/oc_helpers.c
    ${PROJECT_SOURCE_DIR}/api/oc_main.c
    ${PROJECT_SOURCE_DIR}/api/oc_network_events.c
    ${PROJECT_SOURCE_DIR}/api/oc_rep.c
    ${PROJECT_SOURCE_DIR}/api/oc_ri.c
    ${PROJECT_SOURCE_DIR}/api/oc_server_api.c
    ${PROJECT_SOURCE_DIR}/api/oc_session_events.c
    ${PROJECT_SOURCE_DIR}/api/oc_uuid.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_dev.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_fb.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_fp.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_sec.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_swu.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_compare.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_format.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_parse.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_tm.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_valid.c
)

target_include_directories(kis-api INTERFACE
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/api
    ${PROJECT_SOURCE_DIR}/api/c-timestamp
    ${PROJECT_SOURCE_DIR}/include
)

# Core functions used by the stack
add_library(kis-common INTERFACE)


# do not treat warnings as errors on Windows
# block should be defined after the target library
if(MSVC)
    target_compile_options(mbedtls PRIVATE /W1 /WX-)
    target_compile_options(mbedx509 PRIVATE /W1 /WX-)
    target_compile_options(mbedcrypto PRIVATE /W1 /WX-)
    target_compile_options(kis-api INTERFACE /W1 /WX-)
endif()

target_sources(kis-common INTERFACE
    # Utilities that are used deep within the stack
    ${PROJECT_SOURCE_DIR}/util/oc_etimer.c
    ${PROJECT_SOURCE_DIR}/util/oc_list.c
    ${PROJECT_SOURCE_DIR}/util/oc_memb.c
    ${PROJECT_SOURCE_DIR}/util/oc_mem_trace.c
    ${PROJECT_SOURCE_DIR}/util/oc_mmem.c
    ${PROJECT_SOURCE_DIR}/util/oc_process.c
    ${PROJECT_SOURCE_DIR}/util/oc_timer.c
    # Security
    ${PROJECT_SOURCE_DIR}/security/oc_acl.c
    ${PROJECT_SOURCE_DIR}/security/oc_ael.c
    ${PROJECT_SOURCE_DIR}/security/oc_audit.c
    ${PROJECT_SOURCE_DIR}/security/oc_certs.c
    ${PROJECT_SOURCE_DIR}/security/oc_cred.c
    ${PROJECT_SOURCE_DIR}/security/oc_csr.c
    ${PROJECT_SOURCE_DIR}/security/oc_doxm.c
    ${PROJECT_SOURCE_DIR}/security/oc_keypair.c
    #${PROJECT_SOURCE_DIR}/security/oc_obt.c
    ${PROJECT_SOURCE_DIR}/security/oc_obt_certs.c
    #${PROJECT_SOURCE_DIR}/security/oc_obt_otm_cert.c
    #${PROJECT_SOURCE_DIR}/security/oc_obt_otm_justworks.c
    #${PROJECT_SOURCE_DIR}/security/oc_obt_otm_randompin.c
    ${PROJECT_SOURCE_DIR}/security/oc_pki.c
    ${PROJECT_SOURCE_DIR}/security/oc_pstat.c
    ${PROJECT_SOURCE_DIR}/security/oc_roles.c
    ${PROJECT_SOURCE_DIR}/security/oc_sdi.c
    ${PROJECT_SOURCE_DIR}/security/oc_sp.c
    ${PROJECT_SOURCE_DIR}/security/oc_store.c
    ${PROJECT_SOURCE_DIR}/security/oc_svr.c
    ${PROJECT_SOURCE_DIR}/security/oc_tls.c

)

target_include_directories(kis-common INTERFACE
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/deps/tinycbor/src
    ${PROJECT_SOURCE_DIR}/security
)

target_link_libraries(kis-common INTERFACE kis-port mbedtls tinycbor-master)

# stack implementation of CoAP
add_library(kis-coap INTERFACE)

target_sources(kis-coap INTERFACE
    ${PROJECT_SOURCE_DIR}/messaging/coap/coap.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/coap_signal.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/engine.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/observe.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/separate.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/transactions.c
)

target_include_directories(kis-coap INTERFACE
    ${PROJECT_SOURCE_DIR}/messaging/coap/
)

target_link_libraries(kis-coap INTERFACE kis-port)

if(OC_DYNAMIC_ALLOCATION_ENABLED)
    target_compile_definitions(mbedcrypto PUBLIC OC_DYNAMIC_ALLOCATION)
endif()

if(OC_SECURITY_ENABLED)
   # target_compile_definitions(mbedcrypto PUBLIC OC_SECURITY)
endif()

if(OC_PKI_ENABLED)
    target_compile_definitions(mbedcrypto PUBLIC OC_PKI)
endif()

if(OC_DEBUG_ENABLED)
    target_compile_definitions(kis-common INTERFACE OC_DEBUG)
    target_compile_definitions(mbedcrypto PUBLIC OC_DEBUG)
endif()


# Client and server versions
add_library(kis-client)
target_compile_definitions(kis-client PUBLIC OC_CLIENT)
target_link_libraries(kis-client PRIVATE
    kis-coap
    kis-api
    kis-port
)
target_link_libraries(kis-client PUBLIC kis-common)

add_library(kis-server)
target_compile_definitions(kis-server PUBLIC OC_SERVER)
target_link_libraries(kis-server PRIVATE
    kis-coap
    kis-api
    kis-port
)
target_link_libraries(kis-server PUBLIC kis-common)

add_library(kisClientServer)
target_compile_definitions(kisClientServer PUBLIC OC_SERVER OC_CLIENT)
target_link_libraries(kisClientServer PRIVATE
    kis-coap
    kis-api
    kis-port
)
target_link_libraries(kisClientServer PUBLIC kis-common)

include (GenerateExportHeader)

add_library(kisClientServer_shared)
GENERATE_EXPORT_HEADER( kisClientServer_shared
             BASE_NAME kisClientServer_shared
             EXPORT_MACRO_NAME kisClientServer_shared_EXPORT
             EXPORT_FILE_NAME kisClientServer_shared_Export.h
             STATIC_DEFINE kisClientServer_shared_BUILT_AS_STATIC
)

target_compile_definitions(kisClientServer_shared PUBLIC OC_SERVER OC_CLIENT)
target_link_libraries(kisClientServer PRIVATE
    kis-coap
    kis-api
    kis-port
)
target_link_libraries(kisClientServer_shared SHARED kis-common)


if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # Add clang-format target
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/tools/clang-format.cmake
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endif()

add_subdirectory(port)
add_subdirectory(apps)
add_subdirectory(deps)
