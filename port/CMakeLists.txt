# This CMakeLists can only build the port for Linux & Windows
if(UNIX OR WIN32)
	project(knx-iot-stack-port)

	# Detect the platform and pick the right port
	if(UNIX)
        set(PORT_DIR ${PROJECT_SOURCE_DIR}/linux)
	elseif(WIN32)
        set(PORT_DIR ${PROJECT_SOURCE_DIR}/windows)
	endif()


    add_library(kis-port INTERFACE)

    target_include_directories(kis-port INTERFACE 
        ${PORT_DIR}
        ${PROJECT_SOURCE_DIR}
    )

    target_include_directories(mbedcrypto PUBLIC
        ${PORT_DIR}
        ${PROJECT_SOURCE_DIR}
    )
    
    target_sources(kis-port INTERFACE
        ${PORT_DIR}/abort.c
        ${PORT_DIR}/clock.c
        ${PORT_DIR}/ipadapter.c
        ${PORT_DIR}/random.c
        ${PORT_DIR}/storage.c
        ${PORT_DIR}/tcpadapter.c
    )
    
    if(WIN32)
        target_sources(kis-port INTERFACE
            ${PORT_DIR}/mutex.c
            ${PORT_DIR}/network_addresses.c
        )
        target_compile_definitions(kis-port INTERFACE OC_IPV4)
    endif()
    
    if(UNIX)
        target_link_libraries(kis-port INTERFACE pthread m)
    elseif(WIN32)
        target_link_libraries(kis-port INTERFACE bcrypt iphlpapi)
    endif()
endif()