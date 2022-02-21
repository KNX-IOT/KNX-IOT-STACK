project(coap-unittest)

add_executable(messagingtest
	${PROJECT_SOURCE_DIR}/messagingtest.cpp
)

target_link_libraries(messagingtest kisClientServer gtest_main)
