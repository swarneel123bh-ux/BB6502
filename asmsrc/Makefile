# ============================================================
# Makefile — BB6502 OS build system
# Assembler: ca65   Linker: ld65
# ============================================================

CA65    := ca65
LD65    := ld65

# ca65 flags
# --cpu 65c02   : WDC 65C02 instruction set (change to 6502 if needed)
# -I            : include path so .include "vars.s" etc. resolve
CA65FLAGS := --cpu 65c02 -g -I src/utils

# ---- Directories ----
SRC_DIR   := src
BOOT_DIR  := $(SRC_DIR)/boot
UTIL_DIR  := $(SRC_DIR)/utils
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
DBG_DIR   := $(BUILD_DIR)/debug

# ---- Sources ----
ROM_SRC     := $(SRC_DIR)/rom.s          # includes bootloader.s + bios.s
KERNEL_SRC  := $(BOOT_DIR)/kernel.s

# ---- Object files ----
ROM_OBJ     := $(OBJ_DIR)/rom.o
KERNEL_OBJ  := $(OBJ_DIR)/kernel.o

# ---- Final binaries ----
ROM_BIN     := $(BUILD_DIR)/rom.bin
FLOPPY_BIN  := $(BUILD_DIR)/floppy.bin

# ---- Map files ----
ROM_DBG     := $(DBG_DIR)/rom.dbg
KERNEL_DBG  := $(DBG_DIR)/kernel.dbg

# ---- Linker configs ----
ROM_CFG     := rom.cfg
FLOPPY_CFG  := floppy.cfg

# ---- Platform-independent mkdir ----
ifeq ($(OS),Windows_NT)
  MKDIR_P = if not exist "$1" mkdir "$1"
  RM_RF   = rmdir /s /q
else
  MKDIR_P = mkdir -p $1
  RM_RF   = rm -rf
endif

# ============================================================
.PHONY: all clean dirs

all: dirs $(ROM_BIN) $(FLOPPY_BIN)

dirs:
	$(call MKDIR_P,$(BUILD_DIR))
	$(call MKDIR_P,$(OBJ_DIR))
	$(call MKDIR_P,$(DBG_DIR))

# ---- Assemble ROM (bootloader + bios) ----
$(ROM_OBJ): $(ROM_SRC) $(UTIL_DIR)/vars.s $(UTIL_DIR)/bios.s $(BOOT_DIR)/bootloader.s
	$(CA65) $(CA65FLAGS) -o $@ $<

# ---- Assemble kernel ----
$(KERNEL_OBJ): $(KERNEL_SRC) $(UTIL_DIR)/vars.s
	$(CA65) $(CA65FLAGS) -o $@  $<

# ---- Link ROM binary ----
# Pass both objects so ld65 can resolve _bootloader <-> floppy_read etc.
$(ROM_BIN): $(ROM_OBJ) $(ROM_CFG)
	$(LD65) -C $(ROM_CFG) --dbgfile $(ROM_DBG) -o $@ $(ROM_OBJ)

# ---- Link floppy binary ----
# Kernel imports puts/hang/floppy_read from BIOS.
# We pass rom.o as a reference so ld65 can resolve those imports.
# The linker config ensures only KERNEL+KERNELRODATA segments go into floppy.bin.
$(FLOPPY_BIN): $(KERNEL_OBJ) $(ROM_OBJ) $(FLOPPY_CFG)
	$(LD65) -C $(FLOPPY_CFG) --dbgfile $(KERNEL_DBG) -o $@ $(KERNEL_OBJ) $(ROM_OBJ)

# ---- Clean ----
clean:
	$(RM_RF) $(BUILD_DIR)
