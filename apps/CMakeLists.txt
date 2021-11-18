project(knx-iot-stack-apps)

# code example
add_executable(simpleserver_all
    ${PROJECT_SOURCE_DIR}/simpleserver_all.c
)
target_link_libraries(simpleserver_all
        kisClientServer
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleserver_all_creds)

# code example
add_executable(simpleclient_all
    ${PROJECT_SOURCE_DIR}/simpleclient_all.c
)
target_link_libraries(simpleclient_all
        kisClientServer
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleclient_all_creds)

# demo: sensor (e.g. sending command)
add_executable(LSSB_minimal_all
    ${PROJECT_SOURCE_DIR}/LSSB_minimal_all.c
)
target_link_libraries(LSSB_minimal_all
        kisClientServer
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/LSSB_minimal_all_creds)


# demo: actuator (e.g. receiving command)
add_executable(LSAB_minimal_all
    ${PROJECT_SOURCE_DIR}/LSAB_minimal_all.c
)
target_link_libraries(LSAB_minimal_all
        kisClientServer
    )
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/LSAB_minimal_all_creds)


# copy credentials, used by example applications.
add_custom_target(copy-pki-certs ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/pki_certs
    ${PROJECT_BINARY_DIR}/pki_certs
)

# add_dependencies(kis-port copy-pki-credentials)