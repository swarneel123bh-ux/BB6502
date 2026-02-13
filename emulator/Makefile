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
    LLDB    := lldb
    LDFLAGS := -lpthread -lncurses
endif

ifeq ($(HOST_OS),macos)
    CC      := gcc
    LLDB    := lldb
    LDFLAGS := -lpthread -lncurses
endif

ifeq ($(HOST_OS),windows)
    CC      := x86_64-w64-mingw32-gcc
    LLDB    :=
    LDFLAGS := -lws2_32 -lpthread
endif

# ==================================================
# Common Flags
# ==================================================
CFLAGS   := -std=c2x -O2 -I$(INC_DIR) -DHOST_OS_$(shell echo $(HOST_OS) | tr a-z A-Z) -fno-common -Wall -Werror -Wpedantic -pedantic
DBGFLAGS := -g3 -O0


# ==================================================
# Toolchain
# ==================================================
CC      := gcc
LLDB    := lldb
VASM    := tests/vasm

# ==================================================
# Directories
# ==================================================
# Emulator
SRC_DIR := src
INC_DIR := src/include
BUILD   := build
OBJ_DIR := $(BUILD)/obj
BIN_DIR := $(BUILD)/bin

# Tests
TEST_DIR      := tests
TEST_SRC_DIR  := $(TEST_DIR)/asmsrc
TEST_BUILD    := $(TEST_DIR)/build

# ==================================================
# Targets
# ==================================================
TARGET      := $(BIN_DIR)/debug6502
TARGET_DBG  := $(BIN_DIR)/debug6502dbg

# ==================================================
# Sources
# ==================================================
# C sources
SRCS := \
	$(SRC_DIR)/main.c \
	$(wildcard $(INC_DIR)/*.c)

OBJS := $(SRCS:%.c=$(OBJ_DIR)/%.o)

# Test sources
ASM_SRCS := $(wildcard $(TEST_SRC_DIR)/*.s)
TEST_BINS := $(patsubst $(TEST_SRC_DIR)/%.s,$(TEST_BUILD)/%.out,$(ASM_SRCS))
TEST_SYMS := $(patsubst $(TEST_SRC_DIR)/%.s,$(TEST_BUILD)/%.sym,$(ASM_SRCS))
TEST_VOBJ := $(patsubst $(TEST_SRC_DIR)/%.s,$(TEST_BUILD)/%.vobj,$(ASM_SRCS))


# ==================================================
# Flags
# ==================================================
LDFLAGS  := -lpthread -lncurses

# vasm flags
VASM_ASM_FLAGS  := -Fbin  -dotdir -I$(TEST_SRC_DIR)/utils
VASM_SYM_FLAGS  := -dotdir -I$(TEST_SRC_DIR)/utils
VASM_VOBJ_FLAGS := -Fvobj -dotdir -I$(TEST_SRC_DIR)/utils


# ==================================================
# Phony Targets
# ==================================================
.PHONY: all release debug clean dirs tests test run_test run_debug scripts native linux macos windows

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
	mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	mkdir -p $(OBJ_DIR)/$(INC_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(TEST_BUILD)


# ==================================================
# Emulator Build
# ==================================================
# Release flags
release: CFLAGS += -DNDEBUG
release: dirs $(TARGET)
# Debug flags
debug: CFLAGS += $(DBGFLAGS)
debug: dirs $(TARGET_DBG)

# Build commands
$(TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(TARGET_DBG): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ==================================================
# Test Build (VASM)
# ==================================================
# Build all asm outputs
tests: tests-bin tests-sym tests-vobj

# User-facing test command
test: tests
tests-bin:  $(TEST_BINS)
tests-sym:  $(TEST_SYMS)
tests-vobj: $(TEST_VOBJ)

# Build commands
$(TEST_BUILD)/%.out: $(TEST_SRC_DIR)/%.s | dirs
	$(VASM) $(VASM_ASM_FLAGS) -o $@ $<

$(TEST_BUILD)/%.sym: $(TEST_SRC_DIR)/%.s | dirs
	$(VASM) $(VASM_SYM_FLAGS) -Llo -o $@ $<

$(TEST_BUILD)/%.vobj: $(TEST_SRC_DIR)/%.s | dirs
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
# Convenience Scripts
# ==================================================
scripts: test debug

test:
		@echo '#!/bin/sh'        >  test
		@echo 'make run_test'   >> test
		@chmod +x test

debug:
		@echo '#!/bin/sh'        >  debug
		@echo 'make run_debug'  >> debug
		@chmod +x debug

# ==================================================
# Clean
# ==================================================
clean:
	rm -rf $(BUILD)
	rm -rf $(TEST_BUILD)
	rm test
	rm debug
