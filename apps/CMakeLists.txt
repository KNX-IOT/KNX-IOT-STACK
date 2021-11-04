project(knx-iot-stack-apps)

add_executable(simpleserver_all
    ${PROJECT_SOURCE_DIR}/simpleserver_all.c
    ${PROJECT_SOURCE_DIR}/../api/oc_introspection.c
)
target_link_libraries(simpleserver_all
        kis-server-client
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleserver_all_creds)

add_executable(simpleclient_all
    ${PROJECT_SOURCE_DIR}/simpleclient_all.c
    ${PROJECT_SOURCE_DIR}/../api/oc_introspection.c
)
target_link_libraries(simpleclient_all
        kis-server-client
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleclient_all_creds)


if(UNIX)
    add_executable(simpleserver
        ${PROJECT_SOURCE_DIR}/simpleserver.c
        ${PROJECT_SOURCE_DIR}/../api/oc_introspection.c
    )
    target_link_libraries(simpleserver
        kis-server-client
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleserver_creds)

    add_executable(simpleserver_pki
        ${PROJECT_SOURCE_DIR}/simpleserver_pki.c
        ${PROJECT_SOURCE_DIR}/../api/oc_introspection.c
    )
    target_link_libraries(simpleserver_pki
        kis-server-client
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleserver_pki_creds)

    add_executable(simpleclient
        ${PROJECT_SOURCE_DIR}/simpleclient.c
        ${PROJECT_SOURCE_DIR}/../api/oc_introspection.c
    )
    target_link_libraries(simpleclient
        kis-server-client
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleclient_creds)

    if(EXISTS ${PROJECT_SOURCE_DIR}/device_builder_server.c)
        add_executable(device_builder_server
            ${PROJECT_SOURCE_DIR}/device_builder_server.c
            ${PROJECT_SOURCE_DIR}/../api/oc_introspection.c
        )
        target_link_libraries(device_builder_server
            kis-server-client
        )
        file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/device_builder_server_creds)
    endif()


elseif(WIN32)
    add_executable(simpleserver
        ${PROJECT_SOURCE_DIR}/simpleserver_windows.c
        ${PROJECT_SOURCE_DIR}/../api/oc_introspection.c
    )
    target_link_libraries(simpleserver
        kis-server-client
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleserver_creds)

    add_executable(simpleclient
        ${PROJECT_SOURCE_DIR}/simpleclient_windows.c
        ${PROJECT_SOURCE_DIR}/../api/oc_introspection.c
    )
    target_link_libraries(simpleclient
        kis-server-client
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleclient_creds)
endif()

# copy credentials, used by example applications.
add_custom_target(copy-pki-certs ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/pki_certs
    ${PROJECT_BINARY_DIR}/pki_certs
)

add_dependencies(kis-port copy-pki-credentials)