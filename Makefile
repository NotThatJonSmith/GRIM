CXXFLAGS=-O3 -std=c++20 -Wall -Wextra -Wno-unused-parameter -Werror -pedantic

all: grim run_tests

grim: grim.cpp
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
CROSS_BASH = $(DOCKER_BASH_PREFIX) "cd /host_mount_dir && $(1)"

GEN_HEADERS_PATH=tests/build/gen_headers
RV32GC_BLOB_DIR=$(GEN_HEADERS_PATH)/rv32gc
RV32GC_ASM_FILES=$(wildcard tests/rv32gc/*.S)
RV32GC_ASM_BLOBS=$(patsubst tests/rv32gc/%.S,$(RV32GC_BLOB_DIR)/%.h,$(RV32GC_ASM_FILES))
RV64GC_BLOB_DIR=$(GEN_HEADERS_PATH)/rv64gc
RV64GC_ASM_FILES=$(wildcard tests/rv64gc/*.S)
RV64GC_ASM_BLOBS=$(patsubst tests/rv64gc/%.S,$(RV64GC_BLOB_DIR)/%.h,$(RV64GC_ASM_FILES))

ALL_ASM_BLOBS=$(RV32GC_ASM_BLOBS) $(RV64GC_ASM_BLOBS)

tests/build/rv32gc-obj/%.o: tests/rv32gc/%.S
	mkdir -p $(dir $@)
	$(call CROSS_BASH,/cross/bin/riscv32-unknown-elf-gcc -c $< -o $@)

tests/build/rv64gc-obj/%.o: tests/rv64gc/%.S
	mkdir -p $(dir $@)
	$(call CROSS_BASH,/cross/bin/riscv64-unknown-elf-gcc -c $< -o $@)

tests/build/rv32gc-obj/%.bin: tests/build/rv32gc-obj/%.o
	mkdir -p $(dir $@)
	$(call CROSS_BASH,/cross/bin/riscv32-unknown-elf-objcopy -O binary $< $@)

tests/build/rv64gc-obj/%.bin: tests/build/rv64gc-obj/%.o
	mkdir -p $(dir $@)
	$(call CROSS_BASH,/cross/bin/riscv64-unknown-elf-objcopy -O binary $< $@)

$(RV32GC_BLOB_DIR)/%.h: tests/build/rv32gc-obj/%.bin
	mkdir -p $(dir $@)
	python3 ./scripts/bin_to_blob.py $< -o $@

$(RV64GC_BLOB_DIR)/%.h: tests/build/rv64gc-obj/%.bin
	mkdir -p $(dir $@)
	python3 ./scripts/bin_to_blob.py $< -o $@

TEST_SRC_FILES=$(wildcard tests/src/*.cpp)

run_tests: $(GTEST_LIB_MAIN) $(ALL_ASM_BLOBS) $(TEST_SRC_FILES)
	$(CXX) $(CXXFLAGS) \
	-Iinclude \-I$(GTEST_INCLUDE) -I$(GEN_HEADERS_PATH) \
	$(TEST_SRC_FILES) $(GTEST_LIB) $(GTEST_LIB_MAIN) \
	-o $@

clean:
	rm -rf tests/build grim run_tests
