.PHONY: build test clean

.DEFAULT_GOAL := build

CURR_DIR := $(shell realpath ./)
PARALLEL_COUNT=$(shell nproc)

# EXAMPLE_CC=$(CURR_DIR)/../../musl-gcc/bin/x86_64-linux-musl-gcc -static
EXAMPLE_CC=gcc
EXAMPLE_FLAGS=-O0 -g
EXAMPLE_GEM5_TRACE_FLAG=
# EXAMPLE_GEM5_TRACE_FLAG=--debug-flags=Exec
# EXAMPLE_GEM5_TRACE_FLAG=--debug-flags=HFI
# EXAMPLE_GEM5_TRACE_FLAG=--debug-flags=Exec,HFI
# EXAMPLE_GEM5_TRACE_FLAG=--debug-break=1355932000
# EXAMPLE_GEM5_TRACE_FLAG=--debug-flags=SyscallVerbose
# EXAMPLE_GEM5_TRACE_FLAG=--debug-flags=SyscallVerbose,HFI
RUN_SIM=$(shell realpath ./run-gem5.sh) $(EXAMPLE_GEM5_TRACE_FLAG)
EXPECT_FAIL=./mybuild/expectfail.sh
TEST_FILES := $(wildcard ../tests/test-progs/hfi/test-*.c)
TEST_NAMES := $(basename $(notdir $(TEST_FILES)))

build-gem5:
	cd .. && scons build/X86/gem5.opt -j $(PARALLEL_COUNT) --ignore-style

build-gem5-debug:
	cd .. && scons build/X86/gem5.debug -j $(PARALLEL_COUNT) --ignore-style

.PRECIOUS: mybuildout/test-%
mybuildout/test-%: ../tests/test-progs/hfi/test-%.c ../tests/test-progs/hfi/hfi_load_store.S
	mkdir -p mybuildout
	$(EXAMPLE_CC) $^ $(EXAMPLE_FLAGS) -o $@

test-%-success: mybuildout/test-%-success
	cd .. && $(RUN_SIM) mybuild/$^

test-%-fail: mybuildout/test-%-fail
	cd .. && $(EXPECT_FAIL) $(RUN_SIM) mybuild/$^

build-tests-serial: $(addprefix mybuildout/,$(TEST_NAMES))

build-tests:
	$(MAKE) -j$(PARALLEL_COUNT) build-tests-serial

build: build-gem5 build-tests

test-rlbox:
	cd ../../rlbox_hfi_wasm2c_sandbox/build_debug && $(RUN_SIM) ./test_rlbox_glue

test: $(TEST_NAMES)
	@echo "!!!!!!!!!Tests successfully completed!!!!!!!!!"

clean-gem5:
	cd .. && scons -c

clean-tests:
	-rm -rf mybuildout

clean: clean-gem5 clean-tests
