cmake_minimum_required (VERSION 3.15)
project(knx-iot-stack) 
 
if(CMAKE_COMPILER_IS_GNUCC AND UNIX)
    add_compile_options(-fPIC)
endif()
add_compile_definitions(OC_DYNAMIC_ALLOCATION)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Do not build anything except for the library
option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
option(ENABLE_TESTING "Build mbed TLS tests." OFF)
option(BUILD_TESTING "Build KNX tests" OFF)

#set(OC_SECURITY_ENABLED ON CACHE BOOL "Enable security")

set(OC_DNS_SD_ENABLED OFF CACHE BOOL "Enable DNS SD")
set(OC_PUBLISHER_TABLE_ENABLED ON CACHE BOOL "enable Publisher table")
set(OC_OSCORE_ENABLED ON CACHE BOOL "Enable oscore")
set(OC_IOT_ROUTER_ENABLED OFF CACHE BOOL "Enable IoT Router code")
set(OC_PKI_ENABLED OFF CACHE BOOL "Enable PKI security")
set(OC_PRINT_ENABLED ON CACHE BOOL "Enable print messages. Setting this to OFF will disable virtually all logging.")
set(OC_PRINT_APP_ENABLED ON CACHE BOOL "Enable application log messages. Independent from OC_PRINT_ENABLED")
set(OC_DEBUG_ENABLED ON CACHE BOOL "Enable debug messages")
set(OC_DEBUG_OSCORE_ENABLED OFF CACHE BOOL "Enable security debug messages")
set(OC_LOG_TO_FILE_ENABLED OFF CACHE BOOL "redirect debug messages to file")
set(CLANG_TIDY_ENABLED OFF CACHE BOOL "Enable clang-tidy analysis during compilation.")
set(OC_USE_STORAGE ON CACHE BOOL "Persistent storage of data.")
set(OC_USE_MULTICAST_SCOPE_2 ON CACHE BOOL "devices send also group multicast events with scope2.")

set(KNX_BUILTIN_MBEDTLS ON CACHE BOOL "Use built-in mbedTLS, as opposed to external lib from different project")
set(KNX_BUILTIN_TINYCBOR ON CACHE BOOL "Use built-in TinyCBOR, as opposed to external lib from different project")

set(KNX_LOG_CAST_64_BIT_INTS_ENABLED OFF CACHE BOOL "Cast int64_t and uint64_t values to (int) if platform doesn't support 64 bit int format specifiers")

set(KNX_GAMT_MAX_ENTRIES "" CACHE STRING "Maximum number of group address mapping table entries")
set(KNX_GOT_MAX_ENTRIES "" CACHE STRING "Maximum number of group object table entries")
set(KNX_GPT_MAX_ENTRIES "" CACHE STRING "Maximum number of group publisher table entries")
set(KNX_GRT_MAX_ENTRIES "" CACHE STRING "Maximum number of group recipient table entries")

set(KNX_SPAKE_MIN_IT "1000" CACHE STRING "Minimum number of SHA256 iterations used within the SPAKE2+ handshake")
set(KNX_SPAKE_MAX_IT "100000" CACHE STRING "Maximum number of SHA256 iterations used within the SPAKE2+ handshake")
mark_as_advanced(KNX_SPAKE_MIN_IT KNX_SPAKE_MAX_IT)

include(tools/clang-tidy.cmake)

if(KNX_BUILTIN_MBEDTLS)
    # If an mbedtls platform layer is defined, add it to the mbedtls list of libs
    if(TARGET mbedtls-plat)
        set(libs ${libs} mbedtls-plat)
    endif()

    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/mbedtls)

    target_include_directories(mbedcrypto PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/deps/mbedtls/include
        ${PROJECT_SOURCE_DIR}/security
    )

    target_compile_definitions(mbedcrypto
        PUBLIC
        MBEDTLS_CONFIG_FILE=\"knx_mbedtls_config.h\"
    )

    if(TARGET mbedcrypto-plat)
        target_link_libraries(mbedcrypto mbedcrypto-plat)
    endif()
endif()


# API
set(API_SOURCES
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
    ${PROJECT_SOURCE_DIR}/api/oc_knx_client.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_dev.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_fb.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_fp.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_gm.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_helpers.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_p.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_sec.c
    ${PROJECT_SOURCE_DIR}/api/oc_knx_swu.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_compare.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_format.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_parse.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_tm.c
    ${PROJECT_SOURCE_DIR}/api/c-timestamp/timestamp_valid.c
)

set(API_INCLUDE_DIR
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/api
    ${PROJECT_SOURCE_DIR}/api/c-timestamp
    ${PROJECT_SOURCE_DIR}/include
)

# enable clang-tidy 
enable_clang_tidy()

# Core functions used by the stack
set(CORE_SOURCES
    # Utilities that are used deep within the stack
    ${PROJECT_SOURCE_DIR}/util/oc_etimer.c
    ${PROJECT_SOURCE_DIR}/util/oc_list.c
    ${PROJECT_SOURCE_DIR}/util/oc_memb.c
    ${PROJECT_SOURCE_DIR}/util/oc_mem_trace.c
    ${PROJECT_SOURCE_DIR}/util/oc_mmem.c
    ${PROJECT_SOURCE_DIR}/util/oc_process.c
    ${PROJECT_SOURCE_DIR}/util/oc_timer.c
    # Security
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_context.c
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_crypto.c
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_engine.c
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_replay.c
    ${PROJECT_SOURCE_DIR}/security/oc_spake2plus.c
    ${PROJECT_SOURCE_DIR}/security/oc_tls.c

)

# stack implementation of CoAP
set(COAP_SOURCES
    ${PROJECT_SOURCE_DIR}/messaging/coap/coap.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/coap_signal.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/engine.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/observe.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/oscore.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/separate.c
    ${PROJECT_SOURCE_DIR}/messaging/coap/transactions.c
)

set(COAP_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/messaging/coap/
)

add_library(kis-common INTERFACE)

if(OC_OSCORE_ENABLED)
   target_compile_definitions(kis-common INTERFACE OC_OSCORE)
   target_compile_definitions(kis-common INTERFACE OC_SPAKE)
   target_compile_definitions(kis-common INTERFACE KNX_MIN_IT=${KNX_SPAKE_MIN_IT})
   target_compile_definitions(kis-common INTERFACE KNX_MAX_IT=${KNX_SPAKE_MAX_IT})
endif()

if(OC_IOT_ROUTER_ENABLED)
    target_compile_definitions(kis-common INTERFACE OC_IOT_ROUTER)
endif()

if(OC_PRINT_ENABLED)
    target_compile_definitions(kis-common INTERFACE OC_PRINT)
endif()

if(OC_PRINT_APP_ENABLED)
    target_compile_definitions(kis-common INTERFACE OC_PRINT_APP)
endif()

if(OC_DEBUG_ENABLED)
    target_compile_definitions(kis-common INTERFACE OC_DEBUG)
endif()

if(OC_DEBUG_OSCORE_ENABLED)
    target_compile_definitions(kis-common INTERFACE OC_DEBUG_OSCORE)
endif()



if(OC_DNS_SD_ENABLED)
    target_compile_definitions(kis-common INTERFACE OC_DNS_SD)
endif()

if(OC_PUBLISHER_TABLE_ENABLED)
    target_compile_definitions(kis-common INTERFACE OC_PUBLISHER_TABLE)
endif()

if(KNX_LOG_CAST_64_BIT_INTS_ENABLED)
    target_compile_definitions(kis-common INTERFACE KNX_LOG_CAST_64_BIT_INTS)
endif()

if(NOT ${KNX_GAMT_MAX_ENTRIES} EQUAL "")
    add_compile_definitions(GAMT_MAX_ENTRIES=${KNX_GAMT_MAX_ENTRIES})
endif()

if(NOT ${KNX_GOT_MAX_ENTRIES} EQUAL "")
    add_compile_definitions(GOT_MAX_ENTRIES=${KNX_GOT_MAX_ENTRIES})
endif()

if(NOT ${KNX_GPT_MAX_ENTRIES} EQUAL "")
    add_compile_definitions(GPT_MAX_ENTRIES=${KNX_GPT_MAX_ENTRIES})
endif()

if(NOT ${KNX_GRT_MAX_ENTRIES} EQUAL "")
    add_compile_definitions(GRT_MAX_ENTRIES=${KNX_GRT_MAX_ENTRIES})
endif()

# Client and server versions
set(KNX_INCLUDE_DIRS 
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/security
    ${API_INCLUDE_DIRS}
    ${COAP_INCLUDE_DIRS}
)

add_compile_definitions(OC_CLIENT OC_SERVER)
add_library(kisClientServer
    ${CORE_SOURCES}
    ${COAP_SOURCES}
    ${API_SOURCES}
)
target_compile_definitions(kisClientServer PUBLIC OC_CLIENT OC_SERVER)

if (OC_USE_STORAGE)
    target_compile_definitions(kisClientServer PUBLIC OC_USE_STORAGE)
endif()

if (OC_USE_MULTICAST_SCOPE_2)
    target_compile_definitions(kisClientServer PUBLIC OC_USE_MULTICAST_SCOPE_2)
endif()

target_link_libraries(kisClientServer PUBLIC
    kis-common
    mbedtls
    tinycbor-master
    kis-port
)
target_include_directories(kisClientServer PUBLIC ${KNX_INCLUDE_DIRS})

if(MSVC)
    # do not treat warnings as errors on Windows
    add_compile_options(/W1 /WX-)
elseif(NOT BUILD_TESTING)
    # enable warnings as errors on GCC-based toolchains

    # do not do this for the testing tree as there are warnings
    # within gtest
    add_compile_options(-Werror)
endif()


if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # Add clang-format target
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/tools/clang-format.cmake
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endif()



######## Units tests (UNIX only) ########
include(CTest)
if(BUILD_TESTING AND UNIX)
    enable_language(CXX)
    find_package(Threads REQUIRED)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Disable clang-tidy for gtest
    disable_clang_tidy()

    # Build googletest
    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/gtest)
    target_include_directories(gtest PUBLIC ${PROJECT_SOURCE_DIR}/deps/gtest/include)

    add_subdirectory(api/unittest)
    add_subdirectory(port/unittest)
    add_subdirectory(messaging/coap/unittest)
if(OC_OSCORE_ENABLED)
    add_subdirectory(security/unittest)
endif()
endif()

# https://stackoverflow.com/questions/37957583/how-to-use-gcov-with-cmake
if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    include(CodeCoverage)
    setup_target_for_coverage(${PROJECT_NAME}_coverage ${TEST_TARGET} coverage)

    SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
    SET(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
endif() #CMAKE_BUILD_TYPE STREQUAL "Coverage"

add_subdirectory(port)
add_subdirectory(apps)
add_subdirectory(deps)

if(OC_LOG_TO_FILE_ENABLED)
    target_compile_definitions(kis-port INTERFACE OC_LOG_TO_FILE)
endif()