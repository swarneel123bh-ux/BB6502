# ============================================================
# Makefile — BB6502 OS build system
# Assembler: ca65   Linker: ld65
# ============================================================
CA65      := ca65
LD65      := ld65
CA65FLAGS := --cpu 65c02 -g -I src/utils
# ---- Directories ----
SRC_DIR   := src
BOOT_DIR  := $(SRC_DIR)/boot
UTIL_DIR  := $(SRC_DIR)/utils
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
DBG_DIR   := $(BUILD_DIR)/debug
# ---- Sources ----
ROM_SRC    := $(SRC_DIR)/rom.s
KERNEL_SRC := $(BOOT_DIR)/kernel.s
# ---- Object files ----
ROM_OBJ    := $(OBJ_DIR)/rom.o
KERNEL_OBJ := $(OBJ_DIR)/kernel.o
# ---- Final binaries ----
ROM_BIN    := $(BUILD_DIR)/rom.bin
FLOPPY_BIN := $(BUILD_DIR)/floppy.bin
# ---- Debug files ----
ROM_DBG    := $(DBG_DIR)/rom.dbg
KERNEL_DBG := $(DBG_DIR)/kernel.dbg
# ---- Linker configs ----
ROM_CFG    := rom.cfg
FLOPPY_CFG := floppy.cfg
# ---- Floppy geometry (1.44MB) ----
FLOPPY_SIZE := 1474560
# ---- Platform-specific commands ----
ifeq ($(OS),Windows_NT)
  MKDIR_P    = if not exist "$(1)" mkdir "$(1)"
  RM_RF      = rmdir /s /q
  FLOPPY_PAD = powershell -Command \
    "$$f = [System.IO.File]::Open('$(FLOPPY_BIN)',[System.IO.FileMode]::Open,[System.IO.FileAccess]::Write); \
     $$f.SetLength($(FLOPPY_SIZE)); $$f.Close()"
else
  MKDIR_P    = mkdir -p $(1)
  RM_RF      = rm -rf
  FLOPPY_PAD = truncate -s $(FLOPPY_SIZE) $(FLOPPY_BIN) 2>/dev/null || \
               dd if=/dev/zero bs=1 count=0 seek=$(FLOPPY_SIZE) of=$(FLOPPY_BIN) 2>/dev/null
endif
# ============================================================
.PHONY: all clean dirs
all: dirs $(ROM_BIN) $(FLOPPY_BIN)
dirs:
	$(call MKDIR_P,$(BUILD_DIR))
	$(call MKDIR_P,$(OBJ_DIR))
	$(call MKDIR_P,$(DBG_DIR))
# ---- Assemble ROM ----
$(ROM_OBJ): $(ROM_SRC) $(UTIL_DIR)/vars.s $(UTIL_DIR)/bios.s $(BOOT_DIR)/bootloader.s
	$(CA65) $(CA65FLAGS) -o $@ $<
# ---- Assemble kernel ----
$(KERNEL_OBJ): $(KERNEL_SRC) $(UTIL_DIR)/vars.s
	$(CA65) $(CA65FLAGS) -o $@ $<
# ---- Link ROM binary ----
$(ROM_BIN): $(ROM_OBJ) $(ROM_CFG)
	$(LD65) -C $(ROM_CFG) --dbgfile $(ROM_DBG) -o $@ $(ROM_OBJ)
# ---- Link floppy image, then pad to 1.44MB ----
$(FLOPPY_BIN): $(KERNEL_OBJ) $(ROM_OBJ) $(FLOPPY_CFG)
	$(LD65) -C $(FLOPPY_CFG) --dbgfile $(KERNEL_DBG) -o $@ $(KERNEL_OBJ) $(ROM_OBJ)
	$(FLOPPY_PAD)
	@echo "Floppy image: $(FLOPPY_BIN) ($(FLOPPY_SIZE) bytes)"
# ---- Clean ----
clean:
	$(RM_RF) $(BUILD_DIR)
