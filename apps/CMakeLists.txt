project(knx-iot-stack-apps)

if(NOT WIN32 AND NOT UNIX)
    return()
endif()

# test example
add_executable(testserver_all
    ${PROJECT_SOURCE_DIR}/testserver_all.c
)
target_link_libraries(testserver_all
        kisClientServer
    )
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/testserver_all_creds)

# test example
add_executable(testclient_all
    ${PROJECT_SOURCE_DIR}/testclient_all.c
)
target_link_libraries(testclient_all
        mbedtls
        kisClientServer
    )
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/testclient_all_creds)

add_executable(simpleclient_spake_all
    ${PROJECT_SOURCE_DIR}/simpleclient_spake_all.c
)
target_link_libraries(simpleclient_spake_all
        mbedtls
        kisClientServer
    )
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleclient_spake_all_creds)

# demo: sensor (e.g. sending command)
add_executable(lssb_minimal_all
    ${PROJECT_SOURCE_DIR}/lssb_minimal_all.c
)
target_link_libraries(lssb_minimal_all
        mbedtls
        kisClientServer
    )
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lssb_minimal_all_creds)

if(EXISTS ${PROJECT_SOURCE_DIR}/knx_iot_device.c)
    add_executable(knx_iot_device
        ${PROJECT_SOURCE_DIR}/knx_iot_device.c
    )
    target_link_libraries(knx_iot_device kisClientServer)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/knx_iot_device_creds)
endif()

# demo: actuator (e.g. receiving command)
add_executable(lsab_minimal_all
    ${PROJECT_SOURCE_DIR}/lsab_minimal_all.c
)
target_link_libraries(lsab_minimal_all
        mbedtls
        kisClientServer
    )
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lsab_minimal_all_creds)
