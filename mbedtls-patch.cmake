# This is the stack patch tool (originally written in make) ported to cmake so
# that it can run cross-platform. It applies the collection of patches in <project>/patches/
# to mbedtls. This should be run in the project source directory.
cmake_minimum_required (VERSION 3.10)

# Find git
find_package(Git)

if(NOT Git_FOUND)
	message(FATAL_ERROR "Could not find 'git' tool for stack mbedtls patching")
endif()

message("stack patch utils found")

set(STACK_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(MBEDTLS_SRC_DIR "${STACK_SRC_DIR}/deps/mbedtls")
set(STACK_PATCH_DIR "${STACK_SRC_DIR}/patches")

if(EXISTS "${MBEDTLS_SRC_DIR}/.git")
	message("cleaning mbedtls...")
	execute_process(COMMAND ${GIT_EXECUTABLE} -C ${MBEDTLS_SRC_DIR} clean -fdx)
	execute_process(COMMAND ${GIT_EXECUTABLE} -C ${MBEDTLS_SRC_DIR} reset --hard)
	message("mbedtls cleaned")
endif()

execute_process(COMMAND ${GIT_EXECUTABLE} -C ${IOTIVITY_SRC_DIR} submodule update --init)

message("submodules initialised")

file(GLOB PATCHES "${STACK_PATCH_DIR}/*.patch")

foreach(PATCH IN LISTS PATCHES)
	message("Running patch ${PATCH}")
	execute_process(
		COMMAND ${GIT_EXECUTABLE} apply ${PATCH}
		WORKING_DIRECTORY ${MBEDTLS_SRC_DIR}
	)
endforeach()
