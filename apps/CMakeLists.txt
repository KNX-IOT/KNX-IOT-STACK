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

# Raspberry Pi demos, which embed Python in order to drive a Displayotron hat

find_package(Python3 COMPONENTS Interpreter Development)

# demo: sensor (e.g. sending command)
add_executable(LSSB_minimal_pi
    ${PROJECT_SOURCE_DIR}/LSSB_minimal_pi.c
)
target_link_libraries(LSSB_minimal_pi
        kisClientServer
        Python3::Python
    )
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/LSSB_minimal_pi_creds)


# demo: actuator (e.g. receiving command)
add_executable(LSAB_minimal_pi
    ${PROJECT_SOURCE_DIR}/LSAB_minimal_pi.c
)
target_link_libraries(LSAB_minimal_pi
        kisClientServer
        Python3::Python
    )
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/LSAB_minimal_pi_creds)

# Python script that corresponds to the executable must be copied to the same folder
file(COPY ${PROJECT_SOURCE_DIR}/lsab_minimal.py DESTINATION ${PROJECT_BINARY_DIR})

# copy credentials, used by example applications.
add_custom_target(copy-pki-certs ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/pki_certs
    ${PROJECT_BINARY_DIR}/pki_certs
)

# add_dependencies(kis-port copy-pki-credentials)