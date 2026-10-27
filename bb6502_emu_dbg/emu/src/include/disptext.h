// ============================================================================
// disptext.h — text console device (6502 → host terminal stdout)
//
// This is the "serial console": the BIOS putc/puts routines write here and
// the characters appear in the terminal that launched the emulator. The
// graphical 40×30 screen is a separate device (dispgfx.h).
// ============================================================================
#pragma once

#include <stdint.h>

// Written to disptext DATA reg to request worker exit
#define DISPL_EXIT  0xFF

// The disptext device uses a SINGLE register (data reg).
// Protocol:
//   CPU writes non-zero byte  → display the character
//   Worker writes 0 back      → signals "consumed / ready for next"
//   CPU polls reg == 0        → knows worker is ready
//   CPU writes 0xFF (DISPL_EXIT) → worker exits

extern uint16_t disptextDataRegAddr;

// Read the DATA register address from the device table ($FF08) and start
// the worker thread.
extern void  disptextInit(void);
// Worker thread: sleeps until DATA != 0, prints it, writes 0 back.
extern void *disptextWorker(void *args);
