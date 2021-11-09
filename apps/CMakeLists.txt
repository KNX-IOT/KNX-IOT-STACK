project(knx-iot-stack-apps)

add_executable(simpleserver_all
    ${PROJECT_SOURCE_DIR}/simpleserver_all.c
)
target_link_libraries(simpleserver_all
        kis-server-client
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleserver_all_creds)

add_executable(simpleclient_all
    ${PROJECT_SOURCE_DIR}/simpleclient_all.c
)
target_link_libraries(simpleclient_all
        kis-server-client
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleclient_all_creds)


# copy credentials, used by example applications.
add_custom_target(copy-pki-certs ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/pki_certs
    ${PROJECT_BINARY_DIR}/pki_certs
)

# add_dependencies(kis-port copy-pki-credentials)