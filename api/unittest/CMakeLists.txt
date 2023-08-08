project(api-unittest)

add_executable(apitest
	${PROJECT_SOURCE_DIR}/apitest.cpp
	${PROJECT_SOURCE_DIR}/base64test.cpp
	${PROJECT_SOURCE_DIR}/coreresourcetest.cpp
	${PROJECT_SOURCE_DIR}/eptest.cpp
	${PROJECT_SOURCE_DIR}/linkformattest.cpp
	${PROJECT_SOURCE_DIR}/ocapitest.cpp
	${PROJECT_SOURCE_DIR}/reptest.cpp
	${PROJECT_SOURCE_DIR}/RITest.cpp
	${PROJECT_SOURCE_DIR}/uuidtest.cpp
	${PROJECT_SOURCE_DIR}/replaytest.cpp
)

target_link_libraries(apitest kisClientServer gtest_main)
target_compile_options(apitest PRIVATE -fpermissive)