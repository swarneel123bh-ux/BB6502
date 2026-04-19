# ── Compiler ────────────────────────────────────────────────────────────────
CC = gcc

# ── Platform detection ───────────────────────────────────────────────────────
ifeq ($(OS),Windows_NT)
    TARGET_EXT  = .exe
    MKDIR       = if not exist "$1" mkdir "$1"
    RMDIR       = rmdir /S /Q
    RM          = del /Q /F
else
    TARGET_EXT  =
    MKDIR       = mkdir -p $1
    RMDIR       = rm -rf
    RM          = rm -f
endif

# ── Directories ──────────────────────────────────────────────────────────────
SRC_DIR     = src
INC_DIR     = src/include
BUILD_DIR   = build
REL_DIR     = $(BUILD_DIR)/release
DBG_DIR     = $(BUILD_DIR)/debug

# ── Targets ──────────────────────────────────────────────────────────────────
TARGET_REL  = $(REL_DIR)/bb6502_emu$(TARGET_EXT)
TARGET_DBG  = $(DBG_DIR)/bb6502_emu_dbg$(TARGET_EXT)

# ── Auto-discover sources ────────────────────────────────────────────────────
SRCS        = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(INC_DIR)/*.c)

OBJS_REL    = $(patsubst %.c,$(REL_DIR)/%.o,$(notdir $(SRCS)))
OBJS_DBG    = $(patsubst %.c,$(DBG_DIR)/%.o,$(notdir $(SRCS)))

# Dependency files (.d) generated alongside each .o by -MMD -MP
DEPS_REL    = $(OBJS_REL:.o=.d)
DEPS_DBG    = $(OBJS_DBG:.o=.d)

# VPATH lets make locate sources by filename across directories
VPATH       = $(SRC_DIR):$(INC_DIR)

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs)

# ── Flags ────────────────────────────────────────────────────────────────────
# -MMD  : generate a .d file listing all headers this .c depends on
# -MP   : add phony targets for headers so a deleted header doesn't break make
CFLAGS_CMN  = $(SDL_CFLAGS) -Wall -Wextra -pedantic -I$(INC_DIR) -MMD -MP
CFLAGS_REL  = $(CFLAGS_CMN) -O2 -DNDEBUG
CFLAGS_DBG  = $(CFLAGS_CMN) -O0 -g3 -DDEBUG

LDFLAGS     = $(SDL_LIBS) -lpthread


# ── Phony targets ────────────────────────────────────────────────────────────
.PHONY: all release debug clean

all: release debug

release: $(TARGET_REL)

debug: $(TARGET_DBG)

# ── Link ─────────────────────────────────────────────────────────────────────
$(TARGET_REL): $(OBJS_REL)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_DBG): $(OBJS_DBG)
	$(CC) $^ -o $@ $(LDFLAGS)

# ── Compile: release ─────────────────────────────────────────────────────────
$(REL_DIR)/%.o: %.c
	$(call MKDIR,$(REL_DIR))
	$(CC) $(CFLAGS_REL) -c $< -o $@

# ── Compile: debug ───────────────────────────────────────────────────────────
$(DBG_DIR)/%.o: %.c
	$(call MKDIR,$(DBG_DIR))
	$(CC) $(CFLAGS_DBG) -c $< -o $@

# ── Clean ────────────────────────────────────────────────────────────────────
clean:
	$(RMDIR) $(BUILD_DIR)

# ── Pull in generated header dependencies ────────────────────────────────────
# The leading - suppresses errors when .d files don't exist yet (first build)
-include $(DEPS_REL)
-include $(DEPS_DBG)
