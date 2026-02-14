# ==================================================
# OS Detection
# ==================================================
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    HOST_OS := linux
endif
ifeq ($(UNAME_S),Darwin)
    HOST_OS := macos
endif
ifeq ($(OS),Windows_NT)
    HOST_OS := windows
endif

ifndef HOST_OS
$(error Unsupported operating system)
endif

# ==================================================
# Toolchain (OS-specific)
# ==================================================
ifeq ($(HOST_OS),linux)
    CC      := gcc
    LLDB    := gdb
    SYS_LIBS := -lpthread -lncurses
endif

ifeq ($(HOST_OS),macos)
    CC      := gcc
    LLDB    := lldb
    SYS_LIBS := -lpthread -lncurses
endif

ifeq ($(HOST_OS),windows)
    CC      := x86_64-w64-mingw32-gcc
    LLDB    :=
    SYS_LIBS := -lws2_32
endif

# ==================================================
# Directories
# ==================================================
SRC_DIR := src
INC_DIR := src/include
BUILD   := build
OBJ_DIR := $(BUILD)/obj
BIN_DIR := $(BUILD)/bin

OBJ_DIR_REL := $(OBJ_DIR)/release
OBJ_DIR_DBG := $(OBJ_DIR)/debug

TEST_DIR      := tests
TEST_SRC_DIR  := $(TEST_DIR)/asmsrc
TEST_BUILD    := $(TEST_DIR)/build

# ==================================================
# Flags
# ==================================================
CFLAGS_BASE := -std=c2x -I$(INC_DIR) -DHOST_OS_$(shell echo $(HOST_OS) | tr a-z A-Z) \
               -fno-common -Wall -Werror -Wpedantic -pedantic

CFLAGS_REL  := $(CFLAGS_BASE) -O2 -DNDEBUG
CFLAGS_DBG  := $(CFLAGS_BASE) -g3 -O0 -fno-omit-frame-pointer

LDFLAGS := $(SYS_LIBS)

# ==================================================
# Toolchain
# ==================================================
VASM := tests/vasm

# ==================================================
# Targets
# ==================================================
TARGET      := $(BIN_DIR)/debug6502
TARGET_DBG  := $(BIN_DIR)/debug6502dbg

# ==================================================
# Sources
# ==================================================
SRCS := \
	$(SRC_DIR)/main.c \
	$(wildcard $(INC_DIR)/*.c)

OBJS_REL := $(SRCS:%.c=$(OBJ_DIR_REL)/%.o)
OBJS_DBG := $(SRCS:%.c=$(OBJ_DIR_DBG)/%.o)

# ==================================================
# Test Sources (FIXED ORDER)
# ==================================================
ASM_SRCS  := $(wildcard $(TEST_SRC_DIR)/*.s)
TEST_BINS := $(patsubst $(TEST_SRC_DIR)/%.s,$(TEST_BUILD)/%.out,$(ASM_SRCS))
TEST_SYMS := $(patsubst $(TEST_SRC_DIR)/%.s,$(TEST_BUILD)/%.sym,$(ASM_SRCS))
TEST_VOBJ := $(patsubst $(TEST_SRC_DIR)/%.s,$(TEST_BUILD)/%.vobj,$(ASM_SRCS))

# ==================================================
# VASM Flags
# ==================================================
VASM_ASM_FLAGS  := -Fbin  -dotdir -I$(TEST_SRC_DIR)/utils
VASM_SYM_FLAGS  := -dotdir -I$(TEST_SRC_DIR)/utils
VASM_VOBJ_FLAGS := -Fvobj -dotdir -I$(TEST_SRC_DIR)/utils

# ==================================================
# Phony Targets
# ==================================================
.PHONY: all release debug build_release build_debug clean dirs \
        tests test tests-bin tests-sym tests-vobj \
        run_test run_debug scripts gen_test_script gen_debug_script \
        native linux macos windows

# ==================================================
# OS Entry Points
# ==================================================
native:
	@echo "Detected OS: $(HOST_OS)"
	@$(MAKE) all

linux:
	@$(MAKE) HOST_OS=linux all

macos:
	@$(MAKE) HOST_OS=macos all

windows:
	@$(MAKE) HOST_OS=windows all

# ==================================================
# Default
# ==================================================
all: release test debug scripts

# ==================================================
# Directory Setup
# ==================================================
dirs:
	mkdir -p $(OBJ_DIR_REL)/$(SRC_DIR)
	mkdir -p $(OBJ_DIR_REL)/$(INC_DIR)
	mkdir -p $(OBJ_DIR_DBG)/$(SRC_DIR)
	mkdir -p $(OBJ_DIR_DBG)/$(INC_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(TEST_BUILD)

# ==================================================
# Emulator Build
# ==================================================
release: build_release
debug: build_debug

build_release: dirs $(TARGET)
build_debug: dirs $(TARGET_DBG)

$(TARGET): $(OBJS_REL)
	$(CC) $^ $(LDFLAGS) -o $@

$(TARGET_DBG): $(OBJS_DBG)
	$(CC) $^ $(LDFLAGS) -o $@

# Compile rules
$(OBJ_DIR_REL)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_REL) -c $< -o $@

$(OBJ_DIR_DBG)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_DBG) -c $< -o $@

# ==================================================
# Test Build (VASM)
# ==================================================
tests: tests-bin tests-sym tests-vobj
test: tests

tests-bin:  $(TEST_BINS)
tests-sym:  $(TEST_SYMS)
tests-vobj: $(TEST_VOBJ)

$(TEST_BUILD)/%.out: $(TEST_SRC_DIR)/%.s
	@mkdir -p $(TEST_BUILD)
	$(VASM) $(VASM_ASM_FLAGS) -o $@ $<

$(TEST_BUILD)/%.sym: $(TEST_SRC_DIR)/%.s
	@mkdir -p $(TEST_BUILD)
	$(VASM) $(VASM_SYM_FLAGS) -Llo -o $@ $<

$(TEST_BUILD)/%.vobj: $(TEST_SRC_DIR)/%.s
	@mkdir -p $(TEST_BUILD)
	$(VASM) $(VASM_VOBJ_FLAGS) -o $@ $<

# ==================================================
# Run
# ==================================================
run_test: release test
	$(TARGET) $(TEST_BUILD)/uart_test.out

run_debug: debug test
ifeq ($(HOST_OS),windows)
	@echo "Debugger not configured for Windows"
else
	$(LLDB) -- $(TARGET_DBG) $(TEST_BUILD)/uart_test.out
endif

# ==================================================
# Scripts (FIXED NAME COLLISION)
# ==================================================
scripts: gen_test_script gen_debug_script

gen_test_script:
	@echo '#!/bin/sh'        > test
	@echo 'make run_test'   >> test
	@chmod +x test

gen_debug_script:
	@echo '#!/bin/sh'        > debug
	@echo 'make run_debug'  >> debug
	@chmod +x debug

# ==================================================
# Clean
# ==================================================
clean:
	rm -rf $(BUILD)
	rm -rf $(TEST_BUILD)
	rm -f test debug
