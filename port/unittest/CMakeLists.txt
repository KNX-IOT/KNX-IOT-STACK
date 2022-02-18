project(port-unittest)

add_executable(platformtest
	${PROJECT_SOURCE_DIR}/clocktest.cpp
	${PROJECT_SOURCE_DIR}/platformtest.cpp
	${PROJECT_SOURCE_DIR}/storagetest.cpp
)

# fails because include dir is missing 
# add it to gtest_main target in top level cmakescript
target_link_libraries(platformtest kisClientServer gtest_main)