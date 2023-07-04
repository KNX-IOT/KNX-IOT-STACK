# This CMakeLists can only build the port for Linux & Windows



if(UNIX OR WIN32)
    project(knx-iot-stack-port)

    # Detect the platform and pick the right port
    if(UNIX)
        set(PORT_DIR ${PROJECT_SOURCE_DIR}/linux)
    elseif(WIN32)
        set(PORT_DIR ${PROJECT_SOURCE_DIR}/windows)
    endif()

    add_library(kis-port
        ${PROJECT_SOURCE_DIR}/oc_log.c
        ${PORT_DIR}/abort.c
        ${PORT_DIR}/clock.c
        ${PORT_DIR}/dns-sd.c
        ${PORT_DIR}/ipadapter.c
        ${PORT_DIR}/random.c
        ${PORT_DIR}/storage.c
        ${PORT_DIR}/tcpadapter.c
        ${PORT_DIR}/oc_spake.c
    )

    if(OC_LOG_TO_FILE_ENABLED)
        target_compile_definitions(kis-port INTERFACE OC_LOG_TO_FILE)
    endif()

    if (OC_USE_STORAGE)
        target_compile_definitions(kis-port PUBLIC OC_USE_STORAGE)
    endif()

    target_include_directories(kis-port PUBLIC 
        ${PORT_DIR}
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/../
        ${PROJECT_SOURCE_DIR}/../include
    )

    target_include_directories(mbedcrypto PUBLIC
        ${PORT_DIR}
        ${PROJECT_SOURCE_DIR}
    )
    
    if(WIN32)
        target_sources(kis-port PRIVATE
            ${PORT_DIR}/mutex.c
            ${PORT_DIR}/network_addresses.c
        )
        #target_compile_definitions(kis-port PUBLIC OC_IPV4)
    endif()

    target_link_libraries(kis-port PUBLIC kis-common mbedtls)
    
    if(UNIX)
        target_link_libraries(kis-port PUBLIC pthread m)
    elseif(WIN32)
        target_link_libraries(kis-port PUBLIC bcrypt iphlpapi)
    endif()
endif()