project(port-unittest)

add_executable(platformtest
	${PROJECT_SOURCE_DIR}/clocktest.cpp
	${PROJECT_SOURCE_DIR}/platformtest.cpp
	${PROJECT_SOURCE_DIR}/storagetest.cpp
)

target_link_libraries(platformtest kis-port kisClientServer gtest_main)
