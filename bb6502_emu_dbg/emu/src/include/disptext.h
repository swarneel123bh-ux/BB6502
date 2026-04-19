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

extern void  disptextInit(void);
extern void *disptextWorker(void *args);
