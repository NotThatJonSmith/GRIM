CXXFLAGS=-O3 -std=c++20 -Wall -Wextra -Wno-unused-parameter -Werror -pedantic

all: grim run_tests

debug: CXXFLAGS=-O0 -g -std=c++20 -Wall -Wextra -Wno-unused-parameter -Werror -pedantic
debug: grim run_tests

GRIM_HEADERS=$(wildcard include/*.hpp) $(wildcard include/Devices/*.hpp)

grim: grim.cpp $(GRIM_HEADERS)
	${CXX} ${CXXFLAGS} -Iinclude -Iexternal/cxxopts/include -Iexternal/HighELF/include $< -o $@

GTEST_DIR := external/googletest
GTEST_BUILD_DIR := $(GTEST_DIR)/build
GTEST_LIB_MAIN := $(GTEST_BUILD_DIR)/lib/libgtest_main.a
GTEST_LIB := $(GTEST_BUILD_DIR)/lib/libgtest.a
GTEST_INCLUDE := external/googletest/googletest/include

$(GTEST_LIB_MAIN):
	mkdir -p $(GTEST_BUILD_DIR)
	cd $(GTEST_BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Release $(GTEST_DIR)
	$(MAKE) -C $(GTEST_BUILD_DIR)

# 1. Separate out the RISC-V crosstools docker stuff as a separate repo
# 2. Ensure docker daemon is up and crosstools image is set up in this makefile
DOCKER_CROSSTOOLS_IMAGE_NAME=crosstools
DOCKER_RUN_IN_CROSSTOOLS=docker run --rm --mount type=bind,source="$(PWD)",target=/host_mount_dir $(DOCKER_CROSSTOOLS_IMAGE_NAME)
DOCKER_BASH_PREFIX=$(DOCKER_RUN_IN_CROSSTOOLS) /bin/bash -c
CROSS_BASH=$(DOCKER_BASH_PREFIX) "cd /host_mount_dir && $(1)"
# Detect if Docker is running and if the crosstools image exists
DOCKER_RUNNING=$(shell docker info >/dev/null 2>&1 && echo "true" || echo "false")
DOCKER_CROSSTOOLS_IMAGE_EXISTS=$(shell docker image inspect $(DOCKER_CROSSTOOLS_IMAGE_NAME) >/dev/null 2>&1 && echo "true" || echo "false")

check_docker:
    $(eval DOCKER_ERROR :=)
    ifeq ($(DOCKER_RUNNING),false)
        $(eval DOCKER_ERROR += Docker is not running. Please start Docker and try again.\n)
    endif
    ifeq ($(DOCKER_CROSSTOOLS_IMAGE_EXISTS),false)
        $(eval DOCKER_ERROR += Docker image we'd like to use for cross compilers $(DOCKER_CROSSTOOLS_IMAGE_NAME) does not exist.\n)
    endif
    ifneq ($(strip $(DOCKER_ERROR)),)
        $(error $(DOCKER_ERROR))
    endif

%.rv32gc.o: %.rv32gc.S check_docker
	$(call CROSS_BASH,/cross/bin/riscv32-unknown-elf-gcc -c $< -o $@)

%.rv64gc.o: %.rv64gc.S check_docker
	$(call CROSS_BASH,/cross/bin/riscv64-unknown-elf-gcc -c $< -o $@)

%.rv32gc.bin: %.rv32gc.o check_docker
	$(call CROSS_BASH,/cross/bin/riscv32-unknown-elf-objcopy -O binary $< $@)

%.rv64gc.bin: %.rv64gc.o check_docker
	$(call CROSS_BASH,/cross/bin/riscv64-unknown-elf-objcopy -O binary $< $@)

%.rv32gc.h: %.rv32gc.bin
	python3 ./scripts/bin_to_blob.py $< -o $@

%.rv64gc.h: %.rv64gc.bin
	python3 ./scripts/bin_to_blob.py $< -o $@

tests/obj/%.o: tests/src/%.cpp
	mkdir -p $(dir $@)
	python3 ./scripts/extract_target_asm.py -od tests/obj $<
	$(eval GEN_ASMS := $(shell python3 ./scripts/extract_target_asm.py -od tests/obj --filenames $<))
	$(eval GEN_HEADERS := $(patsubst %.S,%.h,$(GEN_ASMS)))
	echo $(GEN_HEADERS)
	if [ -n "$(GEN_HEADERS)" ]; then \
		$(MAKE) $(GEN_HEADERS); \
	fi
	$(CXX) $(CXXFLAGS) \
	-Iinclude -I$(GTEST_INCLUDE) -Itests/obj \
	-c $< -o $@

TEST_SRC_FILES=$(wildcard tests/src/*.cpp)
TEST_OBJ_FILES=$(patsubst tests/src/%.cpp,tests/obj/%.o,$(TEST_SRC_FILES))

run_tests: $(GTEST_LIB_MAIN) $(TEST_OBJ_FILES) $(GRIM_HEADERS)
	$(CXX) $(CXXFLAGS) \
	-Iinclude -I$(GTEST_INCLUDE) -I$(GEN_HEADERS_PATH) \
	$(TEST_OBJ_FILES) $(GTEST_LIB) $(GTEST_LIB_MAIN) \
	-o $@

test: run_tests
	./run_tests

clean:
	rm -rf tests/obj grim run_tests

.PHONY: all clean check_docker test
