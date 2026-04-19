#pragma once

#include <stdint.h>

// ─── Display geometry ─────────────────────────────────────────────────────────
#define DISPGFX_COLS            40
#define DISPGFX_ROWS            30
#define DISPGFX_CHAR_W          8
#define DISPGFX_CHAR_H          8
#define DISPGFX_WIDTH           (DISPGFX_COLS * DISPGFX_CHAR_W)  // 320
#define DISPGFX_HEIGHT          (DISPGFX_ROWS * DISPGFX_CHAR_H)  // 240
#define DISPGFX_SCALE           3           // window = 960 × 720
#define DISPGFX_VRAM_SIZE       (DISPGFX_COLS * DISPGFX_ROWS)    // 1200

// ─── Commands (6502 → CMD register) ──────────────────────────────────────────
#define DISPGFX_CMD_NOP          0x00
#define DISPGFX_CMD_SET_VRAM     0x01  // DATA = 16-bit LE base of char VRAM
#define DISPGFX_CMD_SET_CRAM     0x02  // DATA = 16-bit LE base of color RAM
#define DISPGFX_CMD_CLEAR        0x03  // fill VRAM with spaces, CRAM with dflt
#define DISPGFX_CMD_SET_CURSOR   0x04  // DATA low = col, DATA high = row
#define DISPGFX_CMD_CURSOR_ON    0x05  // enable blinking cursor
#define DISPGFX_CMD_CURSOR_OFF   0x06  // disable cursor
#define DISPGFX_CMD_SET_BORDER   0x07  // DATA low = border colour index (0-15)

// ─── Status register bits ────────────────────────────────────────────────────
#define DISPGFX_STATUS_IDLE      0x01
#define DISPGFX_STATUS_BUSY      0x02
#define DISPGFX_STATUS_VBLANK    0x04  // set once per frame

// ─── Device-table addresses in ROM ───────────────────────────────────────────
//  $FF0A-$FF0B  →  address of dispgfx CMD register
//  $FF0C-$FF0D  →  address of dispgfx DATA register (2 bytes)
//  $FF0E-$FF0F  →  address of dispgfx STATUS register

// ─── 16-colour palette (CGA order) ──────────────────────────────────────────
//  0 Black      4 Dark red      8 Dark grey    12 Light red
//  1 Dark blue  5 Dark magenta  9 Blue         13 Magenta
//  2 Dark green 6 Brown        10 Green        14 Yellow
//  3 Dark cyan  7 Light grey   11 Cyan         15 White
#define DISPGFX_NUM_COLOURS      16

// Colour-RAM attribute byte layout:
//   bits 7-4  →  background colour index
//   bits 3-0  →  foreground colour index
// Default (if no CRAM set): light grey on black (0x07)

// ─── Register address globals (loaded from device table at init) ─────────────
extern uint16_t dispgfxCmdRegAddr;
extern uint16_t dispgfxDataRegAddr;
extern uint16_t dispgfxStatusRegAddr;

// ─── API ─────────────────────────────────────────────────────────────────────

// Called from the main thread BEFORE the CPU loop starts.
// Creates the SDL window and renderer, reads device-table entries,
// and spawns the command-processing worker thread.
extern void dispgfxInit(void);

// Command-processing worker thread (same pattern as other devices).
extern void *dispgfxWorker(void *args);

// Main-thread SDL event + render loop.  Blocks until running == 0.
// On macOS, SDL MUST be driven from the main thread.
extern void dispgfxRenderLoop(void);

// Clean up SDL resources.  Called after the render loop exits.
extern void dispgfxCleanup(void);
