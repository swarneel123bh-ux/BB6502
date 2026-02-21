.PHONY: all release debug clean dirs tests test run_test run_debug scripts

all: release test debug scripts post_build_cleanup

ifeq ($(OS),Windows_NT)
    HOST_OS := windows
    EXE := .exe
    SYS_LIBS := -lncurses
    MKDIR_P = if not exist "$(1)" mkdir "$(1)"
    RM_DIR  = if exist "$(1)" rmdir /s /q "$(1)"
    RM_FILE = if exist "$(1)" del /q "$(1)"
    GDB := $(shell where gdb 2>NUL)
else
    HOST_OS := unix
    EXE :=
    SYS_LIBS := -lncurses
    MKDIR_P = mkdir -p $(1)
    RM_DIR  = rm -rf $(1)
    RM_FILE = rm -f $(1)
    GDB := $(shell which gdb 2>/dev/null)
endif

DELETE_TEMP := .c_test.c .c_test$(EXE) .n_t.c .n_t.o
.DELETE_ON_ERROR:
post_build_cleanup:
ifeq ($(HOST_OS),windows)
	-@for %%f in ($(DELETE_TEMP)) do if exist %%f del /q %%f
else
	-@rm -f $(DELETE_TEMP)
endif

CC := gcc

C2X_SUPPORTED := $(shell echo "int main(){}" > .c_test.c && \
    $(CC) -std=c2x .c_test.c -o .c_test$(EXE) >nul 2>&1 && echo yes)

ifeq ($(C2X_SUPPORTED),yes)
    CSTD := -std=c2x
else
    CSTD := -std=c1x
endif

$(shell $(RM_FILE) .c_test.c)
$(shell $(RM_FILE) .c_test$(EXE))

SRC_DIR := src
INC_DIR := src/include
BUILD   := build
OBJ_DIR := $(BUILD)/obj
BIN_DIR := $(BUILD)/bin

OBJ_REL := $(OBJ_DIR)/release
OBJ_DBG := $(OBJ_DIR)/debug

TEST_DIR     := tests
TEST_SRC_DIR := $(TEST_DIR)/asmsrc
TEST_BUILD   := $(TEST_DIR)/build

HOST_OS_UPPER := $(if $(filter windows,$(HOST_OS)),WINDOWS,UNIX)

CFLAGS_BASE := $(CSTD) -I$(INC_DIR) -DHOST_OS_$(HOST_OS_UPPER) \
               -fno-common -Wall -Werror -Wpedantic -pedantic
CFLAGS_REL := $(CFLAGS_BASE) -O2 -DNDEBUG
CFLAGS_DBG := $(CFLAGS_BASE) -g3 -O0 -fno-omit-frame-pointer
LDFLAGS := $(SYS_LIBS)

VASM := tests/vasm$(EXE)

TARGET     := $(BIN_DIR)/debug6502$(EXE)
TARGET_DBG := $(BIN_DIR)/debug6502dbg$(EXE)

SRCS := $(SRC_DIR)/main.c $(wildcard $(INC_DIR)/*.c)
OBJS_REL := $(SRCS:%.c=$(OBJ_REL)/%.o)
OBJS_DBG := $(SRCS:%.c=$(OBJ_DBG)/%.o)

ASM_SRCS  := $(wildcard $(TEST_SRC_DIR)/*.s)
TEST_BINS := $(patsubst $(TEST_SRC_DIR)/%.s,$(TEST_BUILD)/%.out,$(ASM_SRCS))
TEST_SYMS := $(patsubst $(TEST_SRC_DIR)/%.s,$(TEST_BUILD)/%.sym,$(ASM_SRCS))
TEST_VOBJ := $(patsubst $(TEST_SRC_DIR)/%.s,$(TEST_BUILD)/%.vobj,$(ASM_SRCS))

VASM_ASM_FLAGS  := -Fbin  -dotdir -I$(TEST_SRC_DIR)/utils
VASM_SYM_FLAGS  := -dotdir -I$(TEST_SRC_DIR)/utils
VASM_VOBJ_FLAGS := -Fvobj -dotdir -I$(TEST_SRC_DIR)/utils

dirs:
	$(call MKDIR_P,$(OBJ_REL))
	$(call MKDIR_P,$(OBJ_DBG))
	$(call MKDIR_P,$(BIN_DIR))
	$(call MKDIR_P,$(TEST_BUILD))

release: dirs $(TARGET) post_build_cleanup
debug: dirs $(TARGET_DBG) post_build_cleanup

$(TARGET): $(OBJS_REL)
	$(CC) $^ $(LDFLAGS) -o $@

$(TARGET_DBG): $(OBJS_DBG)
	$(CC) $^ $(LDFLAGS) -o $@

$(OBJ_REL)/%.o: %.c
	$(call MKDIR_P,$(dir $@))
	$(CC) $(CFLAGS_REL) -c $< -o $@

$(OBJ_DBG)/%.o: %.c
	$(call MKDIR_P,$(dir $@))
	$(CC) $(CFLAGS_DBG) -c $< -o $@

tests: $(TEST_BINS) $(TEST_SYMS) $(TEST_VOBJ)
test: tests

$(TEST_BUILD)/%.out: $(TEST_SRC_DIR)/%.s
	$(call MKDIR_P,$(TEST_BUILD))
	$(VASM) $(VASM_ASM_FLAGS) -o $@ $<

$(TEST_BUILD)/%.sym: $(TEST_SRC_DIR)/%.s
	$(call MKDIR_P,$(TEST_BUILD))
	$(VASM) $(VASM_SYM_FLAGS) -Llo -o $@ $<

$(TEST_BUILD)/%.vobj: $(TEST_SRC_DIR)/%.s
	$(call MKDIR_P,$(TEST_BUILD))
	$(VASM) $(VASM_VOBJ_FLAGS) -o $@ $<

run_test: release test
	$(TARGET) $(TEST_BUILD)/uart_test.out

run_debug: debug test
ifeq ($(GDB),)
	@echo No debugger found
else
	$(GDB) -- $(TARGET_DBG) $(TEST_BUILD)/uart_test.out
endif

scripts:
ifeq ($(HOST_OS),windows)
	@echo @echo off > test.bat
	@echo make run_test >> test.bat
	@echo @echo off > debug.bat
	@echo make run_debug >> debug.bat
else
	@echo '#!/bin/sh' > test
	@echo 'make run_test' >> test
	@chmod +x test
	@echo '#!/bin/sh' > debug
	@echo 'make run_debug' >> debug
	@chmod +x debug
endif

clean:
	$(call RM_DIR,$(BUILD))
	$(call RM_DIR,$(TEST_BUILD))
ifeq ($(HOST_OS),windows)
	$(call RM_FILE,test.bat)
	$(call RM_FILE,debug.bat)
else
	$(call RM_FILE,test)
	$(call RM_FILE,debug)
endif