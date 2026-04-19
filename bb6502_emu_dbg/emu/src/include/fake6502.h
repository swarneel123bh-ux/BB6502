// Modification of fake6502 emulator by Mike Chambers
#pragma once

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include "dispgfx.h"
#include "disptext.h"
#include "floppy.h"
#include "kbd.h"

// ─── Device thread argument structure ────────────────────────────────────────
typedef struct threadArgs {
  pthread_mutex_t *lock;
  pthread_cond_t *cond;
} threadArgs;

// ─── Device table (0xFF00–0xFF0F): 2-byte LE pointers to actual registers ───
//     Each entry is the address stored in the table, not the register itself.
//
//  $FF00–$FF01  →  address of floppy STATUS reg
//  $FF02–$FF03  →  address of floppy CMD reg
//  $FF04–$FF05  →  address of floppy DATA reg
//  $FF06–$FF07  →  address of kbd DATA reg
//  $FF08–$FF09  →  address of disptext DATA reg
//  $FF0A–$FF0B  →  address of dispgfx CMD reg
//  $FF0C–$FF0D  →  address of dispgfx DATA reg
//  $FF0E–$FF0F  →  address of dispgfx STATUS reg

#define EMU_FLOPPY_BASE (0xFF00)
#define EMU_FLOPPY_STATUS_REG (EMU_FLOPPY_BASE + 0)
#define EMU_FLOPPY_CMD_REG (EMU_FLOPPY_BASE + 2)
#define EMU_FLOPPY_DATA_REG (EMU_FLOPPY_BASE + 4)

#define EMU_KBD_BASE (0xFF06)
#define EMU_KBD_DATA_REG (EMU_KBD_BASE + 0)

#define EMU_DISPTEXT_BASE (0xFF08)

#define EMU_DISPGFX_BASE (0xFF0A)

// ─── Mutex + condition variables ─────────────────────────────────────────────
extern pthread_mutex_t kbdLock;
extern pthread_cond_t kbdCond;
extern pthread_mutex_t floppyLock;
extern pthread_cond_t floppyCond;
extern pthread_mutex_t disptextLock;
extern pthread_cond_t disptextCond;
extern pthread_mutex_t dispgfxLock;
extern pthread_cond_t dispgfxCond;

// ─── Shared state ────────────────────────────────────────────────────────────
extern uint8_t *mem6502;
extern volatile _Atomic int running;
// Set to 1 after all devices are initialized; gates MMIO checks in
// read6502/write6502 so binary loading doesn't false-match reg addresses.
extern volatile int devicesReady;
// Device threads set this to 1 to request an IRQ; the CPU main loop
// delivers it between instructions (avoids data race on pc/sp/status).
extern volatile _Atomic int irqPending;

// ─── 6502 defines ────────────────────────────────────────────────────────────
#define UNDOCUMENTED // enable undocumented opcodes
#define NES_CPU      // disable BCD (2A03 style)

#define FLAG_CARRY 0x01
#define FLAG_ZERO 0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL 0x08
#define FLAG_BREAK 0x10
#define FLAG_CONSTANT 0x20
#define FLAG_OVERFLOW 0x40
#define FLAG_SIGN 0x80

#define BASE_STACK 0x100

#define saveaccum(n) a = (uint8_t)((n) & 0x00FF)

// flag modifier macros
#define setcarry() status |= FLAG_CARRY
#define clearcarry() status &= (~FLAG_CARRY)
#define setzero() status |= FLAG_ZERO
#define clearzero() status &= (~FLAG_ZERO)
#define setinterrupt() status |= FLAG_INTERRUPT
#define clearinterrupt() status &= (~FLAG_INTERRUPT)
#define setdecimal() status |= FLAG_DECIMAL
#define cleardecimal() status &= (~FLAG_DECIMAL)
#define setoverflow() status |= FLAG_OVERFLOW
#define clearoverflow() status &= (~FLAG_OVERFLOW)
#define setsign() status |= FLAG_SIGN
#define clearsign() status &= (~FLAG_SIGN)

// flag calculation macros
#define zerocalc(n)                                                            \
  {                                                                            \
    if ((n) & 0x00FF)                                                          \
      clearzero();                                                             \
    else                                                                       \
      setzero();                                                               \
  }

#define signcalc(n)                                                            \
  {                                                                            \
    if ((n) & 0x0080)                                                          \
      setsign();                                                               \
    else                                                                       \
      clearsign();                                                             \
  }

#define carrycalc(n)                                                           \
  {                                                                            \
    if ((n) & 0xFF00)                                                          \
      setcarry();                                                              \
    else                                                                       \
      clearcarry();                                                            \
  }

#define overflowcalc(n, m, o)                                                  \
  {                                                                            \
    if (((n) ^ (uint16_t)(m)) & ((n) ^ (o)) & 0x0080)                          \
      setoverflow();                                                           \
    else                                                                       \
      clearoverflow();                                                         \
  }

// ─── 6502 CPU registers
// ───────────────────────────────────────────────────────
extern uint16_t pc;
extern uint8_t sp, a, x, y, status;

// ─── Helper variables
// ─────────────────────────────────────────────────────────
extern uint32_t instructions;
extern uint32_t clockticks6502, clockgoal6502;
extern uint16_t oldpc, ea, reladdr, value, result;
extern uint8_t opcode, oldstatus;

// ─── Externally supplied memory access (defined in fake6502.c) ───────────────
extern uint8_t read6502(uint16_t address);
extern void write6502(uint16_t address, uint8_t value);

// ─── Stack helpers
// ────────────────────────────────────────────────────────────
extern void push16(uint16_t pushval);
extern void push8(uint8_t pushval);
extern uint16_t pull16(void);
extern uint8_t pull8(void);

// ─── CPU control
// ──────────────────────────────────────────────────────────────
extern void reset6502(void);
extern void nmi6502(void);
extern void irq6502(void);

extern uint8_t penaltyop, penaltyaddr;
extern uint8_t callexternal;
extern void (*loopexternal)(void);

extern void exec6502(uint32_t tickcount);
extern void step6502(void);
extern void hookexternal(void *funcptr);

extern void fake6502Init(int argc, char **argv);
