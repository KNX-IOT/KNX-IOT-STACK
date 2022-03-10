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
        kisClientServer
    )
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/testclient_all_creds)

add_executable(simpleclient_spake_all
    ${PROJECT_SOURCE_DIR}/simpleclient_spake_all.c
)
target_link_libraries(simpleclient_spake_all
        kisClientServer
    )
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/simpleclient_spake_all_creds)

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

if(UNIX)

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

    add_custom_target(lsab_minimal_py)
    add_custom_command(
        TARGET lsab_minimal_py POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
        ${PROJECT_SOURCE_DIR}/lsab_minimal.py
        ${PROJECT_BINARY_DIR}/lsab_minimal.py
    )

    add_dependencies(LSAB_minimal_pi lsab_minimal_py)

    # Python script that corresponds to the executable must be copied to the same folder
    file(COPY ${PROJECT_SOURCE_DIR}/lsab_minimal.py DESTINATION ${PROJECT_BINARY_DIR})

    add_executable(simpleclient_hat
        ${PROJECT_SOURCE_DIR}/simpleclient_displayotron.c
    )

    target_link_libraries(simpleclient_hat
        kisClientServer
        Python3::Python
    )

    add_custom_target(simpleclient_py)
    add_custom_command(
        TARGET simpleclient_py POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
        ${PROJECT_SOURCE_DIR}/simpleclient.py
        ${PROJECT_BINARY_DIR}/simpleclient.py
    )

    add_dependencies(simpleclient_py simpleclient_py)
    add_dependencies(LSSB_minimal_pi simpleclient_py)
    # add_dependencies(kis-port copy-pki-credentials)
endif()


# copy credentials, used by example applications.
add_custom_target(copy-pki-certs ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/pki_certs
    ${PROJECT_BINARY_DIR}/pki_certs
)