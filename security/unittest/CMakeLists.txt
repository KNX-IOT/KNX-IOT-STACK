project(security-unittest)

if(OC_OSCORE_ENABLED)
	add_executable(securitytest
		${PROJECT_SOURCE_DIR}/oscore_hkdf_test.cpp
		${PROJECT_SOURCE_DIR}/oscore_test.cpp
		${PROJECT_SOURCE_DIR}/securitytest.cpp
		${PROJECT_SOURCE_DIR}/spake_test.cpp
	)

	target_link_libraries(securitytest kis-port kisClientServer gtest_main kisClientServer)
endif()