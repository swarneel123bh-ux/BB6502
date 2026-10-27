// ============================================================================
// kbd.h — keyboard device (host terminal → 6502)
//
// One 6502-visible register, KBD DATA (address from the device table entry
// at $FF06). Protocol:
//   Device writes a key byte into DATA and raises an IRQ.
//   BIOS IRQ handler reads DATA, stores it in KBD_LAST, writes 0 back (ack).
//
// Keys come from two places: the terminal that launched the emulator (this
// file, raw-mode stdin) and the SDL window (dispgfx.c). There is no FIFO:
// a new key overwrites DATA even if the previous one was not acked yet.
// ============================================================================
#pragma once

#include <stdint.h>

// Intended size of a future key FIFO (not used yet).
#define KBD_FIFOSZ 0xFF

// Read the DATA register address, put the terminal in raw mode and start
// the worker thread.
extern void kbdInit(void);
// Worker thread: blocking-polls stdin and forwards each byte to the 6502.
// A lone ESC shuts the emulator down; ESC + 2 bytes is forwarded as-is.
extern void *kbdWorker(void *args);
