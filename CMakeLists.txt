cmake_minimum_required (VERSION 3.10)
project(knx-iot-stack)

if(CMAKE_COMPILER_IS_GNUCC)
    add_compile_options(-fPIC)
endif()
add_compile_definitions(OC_DYNAMIC_ALLOCATION)

# Do not build anything except for the library
option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
option(ENABLE_TESTING "Build mbed TLS tests." OFF)

#set(OC_SECURITY_ENABLED ON CACHE BOOL "Enable security")

set(OC_OSCORE_ENABLED OFF CACHE BOOL "Enable oscore")
set(OC_PKI_ENABLED OFF CACHE BOOL "Enable PKI security")
set(OC_DEBUG_ENABLED OFF CACHE BOOL "Enable debug messages")
set(CLANG_TIDY_ENABLED OFF CACHE BOOL "Enable clang-tidy analysis during compilation.")

include(tools/clang-tidy.cmake)

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
    #${PROJECT_SOURCE_DIR}/security/oc_certs.c
    #${PROJECT_SOURCE_DIR}/security/oc_cred.c
    #${PROJECT_SOURCE_DIR}/security/oc_keypair.c
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_context.c
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_crypto.c
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_engine.c
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
   target_compile_definitions(mbedcrypto PUBLIC OC_OSCORE)
   target_compile_definitions(kis-common INTERFACE OC_OSCORE)
   target_compile_definitions(kis-common INTERFACE OC_SPAKE)
endif()

if(OC_PKI_ENABLED)
    target_compile_definitions(mbedcrypto PUBLIC OC_PKI)
endif()

if(OC_DEBUG_ENABLED)
    target_compile_definitions(kis-common INTERFACE OC_DEBUG)
    target_compile_definitions(mbedcrypto PUBLIC OC_DEBUG)
endif()


# Client and server versions
set(KNX_INCLUDE_DIRS 
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/deps/tinycbor/src
    ${PROJECT_SOURCE_DIR}/security
    ${API_INCLUDE_DIRS}
    ${COAP_INCLUDE_DIRS}
)

add_compile_definitions(OC_CLIENT OC_SERVER)
add_library(kisClientServer
    ${API_SOURCES}
    ${COAP_SOURCES}
    ${CORE_SOURCES}
)
target_compile_definitions(kisClientServer PUBLIC OC_CLIENT OC_SERVER)
target_link_libraries(kisClientServer PUBLIC
    kis-common
    mbedtls
    tinycbor-master
    kis-port
)
target_include_directories(kisClientServer PUBLIC ${KNX_INCLUDE_DIRS})

include (GenerateExportHeader)

add_library(kisCS SHARED 
    ${PROJECT_SOURCE_DIR}/pythonbinding/oc_python.c
    ${API_SOURCES}
    ${COAP_SOURCES}
    ${CORE_SOURCES}
)
GENERATE_EXPORT_HEADER( kisCS
             BASE_NAME kisCS
             EXPORT_MACRO_NAME kisCS_EXPORT
             EXPORT_FILE_NAME kisCS_Export.h
             STATIC_DEFINE kisCS_BUILT_AS_STATIC
)

target_compile_definitions(kisCS PUBLIC OC_SERVER OC_CLIENT)
target_link_libraries(kisCS PUBLIC
    kis-common
    mbedtls
    tinycbor-master
    kis-port
)
target_link_libraries(kisCS PUBLIC kis-common)

target_include_directories(kisCS PRIVATE ${PROJECT_BINARY_DIR} ${KNX_INCLUDE_DIRS})

if(MSVC)
    # do not treat warnings as errors on Windows
    add_compile_options(/W1 /WX-)
else()
    # enable warnings as errors on GCC-based toolchains
    add_compile_options(-Werror)
endif()

# copy Python binding files to build directory
file(COPY "${PROJECT_SOURCE_DIR}/pythonbinding/knx_stack.py" DESTINATION ${PROJECT_BINARY_DIR}/)
file(COPY "${PROJECT_SOURCE_DIR}/pythonbinding/start_shell.bat" DESTINATION ${PROJECT_BINARY_DIR}/)
file(COPY "${PROJECT_SOURCE_DIR}/pythonbinding/build_so_windows.sh" DESTINATION ${PROJECT_BINARY_DIR}/)
file(COPY "${PROJECT_SOURCE_DIR}/pythonbinding/README.md" DESTINATION ${PROJECT_BINARY_DIR}/)
file(COPY ${PROJECT_SOURCE_DIR}/pythonbinding/application_scripts DESTINATION ${PROJECT_BINARY_DIR})

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
