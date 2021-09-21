SED = sed
INSTALL = install
OS = linux
PROJECT_ROOT_DIR = ../../..
OUT_DIR = $(PROJECT_ROOT_DIR)/port/$(OS)
MODULE = ri
#CHECK_SCRIPT = ../../tools/check.py


GTEST_DIR = $(PROJECT_ROOT_DIR)/deps/gtest
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h
GTEST = gtest_build
TEST_CPPFLAGS += -isystem $(GTEST_DIR)/include
TEST_CXXFLAGS += -g -Wall -Wextra -pthread -std=c++0x -fpermissive -DOC_CLIENT -DOC_SERVER -fprofile-arcs -ftest-coverage
PROJECT_HEADER_DIR = $(PROJECT_ROOT_DIR)/include

API_TEST_DIR = $(PROJECT_ROOT_DIR)/tests/itc/$(MODULE)
API_TEST_OBJ_DIR = $(API_TEST_DIR)/src/obj
API_TEST_HELPER_OBJ_DIR = $(API_TEST_DIR)/helper/obj

API_TEST_HELPER_FILES := $(wildcard $(API_TEST_DIR)/helper/*.cpp)
API_TEST_HEADER_DIR = $(API_TEST_DIR)/include/
API_TEST_SRC_FILES := $(wildcard $(API_TEST_DIR)/src/*.cpp)

API_TEST_OBJ_FILES := $(patsubst $(API_TEST_DIR)/src/%.cpp,$(API_TEST_OBJ_DIR)/%.o,$(API_TEST_SRC_FILES))
API_TEST_HELPER_OBJ_FILES := $(patsubst $(API_TEST_DIR)/helper/%.cpp,$(API_TEST_HELPER_OBJ_DIR)/%.o,$(API_TEST_HELPER_FILES))
INT_TESTS =  integration_test


CONSTRAINED_LIBS = libknxiot-constrained-server.a libknxiot-constrained-client.a \
		   libknxiot-constrained-server.so libknxiot-constrained-client.so \
		   libknxiot-constrained-client-server.so libknxiot-constrained-client-server.a
PC = knxiot-constrained-client.pc knxiot-constrained-server.pc \
     knxiot-constrained-client-server.pc

test: $(INT_TESTS)
all: test

.PHONY: $(GTEST) clean

$(GTEST): $(GTEST_DIR)
	$(MAKE) --directory=$</make

$(API_TEST_HELPER_OBJ_DIR)/%.o: $(API_TEST_DIR)/helper/%.cpp
	@mkdir -p ${@D}
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS) -I$(PROJECT_HEADER_DIR) -I$(PROJECT_ROOT_DIR) -I$(API_TEST_HEADER_DIR) -I$(OUT_DIR) -I$(PROJECT_ROOT_DIR)/deps/tinycbor/src -c $< -o $@


$(API_TEST_OBJ_DIR)/%.o: $(API_TEST_DIR)/src/%.cpp
	@mkdir -p ${@D}
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS) -I$(PROJECT_HEADER_DIR) -I$(PROJECT_ROOT_DIR) -I$(API_TEST_HEADER_DIR) -I$(OUT_DIR) -I$(PROJECT_ROOT_DIR)/deps/tinycbor/src -c $< -o $@

integration_test: $(API_TEST_OBJ_FILES) $(API_TEST_HELPER_OBJ_FILES) $(OUT_DIR)/libknxiot-constrained-client-server.a | $(GTEST)
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS)  -I$(PROJECT_HEADER_DIR) -I$(API_TEST_HEADER_DIR) -l:gtest_main.a -lknxiot-constrained-client-server -L$(OUT_DIR) -L$(GTEST_DIR)/make -lpthread $^ -o $@


clean:
	rm -rf $(API_TEST_OBJ_FILES) $(API_TEST_HELPER_OBJ_FILES) $(UNIT_TESTS)

cleanall: clean
	rm -rf $(UNIT_TESTS)
	${MAKE} -C ${GTEST_DIR}/make clean


