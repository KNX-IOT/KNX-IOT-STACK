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
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# Enable missing algorithms
# TODO add this to Makefiles, or regenerate mbedtls patches
add_compile_definitions(MBEDTLS_HKDF_C)
add_compile_options(-fPIC)

# Do not build anything except for the library
option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
option(ENABLE_TESTING "Build mbed TLS tests." OFF)

set(OC_DYNAMIC_ALLOCATION_ENABLED ON CACHE BOOL "Enable dynamic memory allocation within the OCF stack and MBedtls.")
#set(OC_SECURITY_ENABLED ON CACHE BOOL "Enable security")

set(OC_OSCORE_ENABLED ON CACHE BOOL "Enable oscore")
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
    ${PROJECT_SOURCE_DIR}/api/oc_knx_client.c
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

# enable clang-tidy 
enable_clang_tidy()

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
    ${PROJECT_SOURCE_DIR}/security/oc_certs.c
    ${PROJECT_SOURCE_DIR}/security/oc_cred.c
    ${PROJECT_SOURCE_DIR}/security/oc_keypair.c
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_context.c
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_crypto.c
    ${PROJECT_SOURCE_DIR}/security/oc_oscore_engine.c
    ${PROJECT_SOURCE_DIR}/security/oc_spake2plus.c
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
    ${PROJECT_SOURCE_DIR}/messaging/coap/oscore.c
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
target_compile_definitions(kisClientServer PUBLIC OC_SERVER=1 OC_CLIENT=1)
target_link_libraries(kisClientServer PRIVATE
    kis-coap
    kis-api
    kis-port
)
target_link_libraries(kisClientServer PUBLIC kis-common)

add_compile_definitions(OC_CLIENT OC_SERVER)
include (GenerateExportHeader)

add_library(kisCS SHARED ${PROJECT_SOURCE_DIR}/pythonbinding/oc_python.c)
GENERATE_EXPORT_HEADER( kisCS
             BASE_NAME kisCS
             EXPORT_MACRO_NAME kisCS_EXPORT
             EXPORT_FILE_NAME kisCS_Export.h
             STATIC_DEFINE kisCS_BUILT_AS_STATIC
)

target_compile_definitions(kisCS PUBLIC OC_SERVER OC_CLIENT)
target_link_libraries(kisCS PUBLIC
    kis-coap
    kis-api
    kis-port
)
target_link_libraries(kisCS PUBLIC kis-common)

target_include_directories(kisCS PRIVATE ${PROJECT_BINARY_DIR})

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
    list(APPEND TEST_COMPILE_OPTIONS "-Wall" "-Wextra"  "-fpermissive")
    list(APPEND TEST_LINK_LIBS kisClientServer gtest_main Threads::Threads)
    if(CMAKE_COMPILER_IS_GNUCXX)
        list(APPEND TEST_COMPILE_OPTIONS "-fprofile-arcs" "-ftest-coverage")
        list(APPEND TEST_LINK_LIBS gcov)
    endif()

    # Disable clang-tidy for gtest
    disable_clang_tidy()

    # Build googletest
    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/gtest gtest)

    # Helper macro to build unit test
    macro(package_add_test TESTNAME FOLDER)
        add_executable(${TESTNAME} ${PROJECT_SOURCE_DIR}/${FOLDER}/${TESTNAME}.cpp)
        target_compile_options(${TESTNAME} PRIVATE ${TEST_COMPILE_OPTIONS})
        target_compile_features(${TESTNAME} PRIVATE cxx_nullptr)
        target_compile_definitions(${TESTNAME} PRIVATE ${PUBLIC_COMPILE_DEFINITIONS} "OC_CLIENT" "OC_SERVER")
        target_include_directories(${TESTNAME} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/deps/gtest/include)
        target_include_directories(${TESTNAME} PRIVATE
            ${PROJECT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/port/linux
            ${PORT_INCLUDE_DIR}
            ${PROJECT_SOURCE_DIR}/messaging/coap
        )
        if(OC_OSCORE_ENABLED)
            target_include_directories(${TESTNAME} PRIVATE
                ${PROJECT_SOURCE_DIR}/security
                ${PROJECT_SOURCE_DIR}/deps/mbedtls/include
            )
        endif()
        target_link_libraries(${TESTNAME} PRIVATE ${TEST_LINK_LIBS})
        add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
        set_target_properties(${TESTNAME} PROPERTIES FOLDER unittests)
    endmacro()

    # Unit tests
    file(GLOB APITEST_SRC api/unittest/*.cpp)
    package_add_test(apitest api/unittest ${APITEST_SRC})

    file(GLOB PLATFORMTEST_SRC port/unittest/*.cpp)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/storage_test)
    package_add_test(platformtest port/unittest ${PLATFORMTEST_SRC} )

    file(GLOB MESSAGINGTEST_SRC messaging/coap/unittest/*.cpp)
    package_add_test(messagingtest messaging/coap/unittest ${MESSAGINGTEST_SRC})

    if(OC_OSCORE_ENABLED)
        file(GLOB SECURITYTEST_SRC security/unittest/*.cpp)
        package_add_test(securitytest security/unittest ${SECURITYTEST_SRC})
    endif()

    # reenable clang-tidy for any remaining targets
    enable_clang_tidy()
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
