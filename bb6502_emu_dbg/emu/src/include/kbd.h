#pragma once

#include <stdint.h>

#define KBD_FIFOSZ 0xFF

extern void kbdInit(void);
extern void *kbdWorker(void *args);
