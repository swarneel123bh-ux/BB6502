// ============================================================================
// main.c — BB6502 threaded emulator entry point
//
// Usage:  bb6502_emu <rom.bin> -f <floppy.bin>
//
// All the real work lives in fake6502Init() (fake6502.c): it loads the ROM,
// starts the device threads, runs the CPU on its own thread and turns the
// main thread into the SDL render loop. It returns when the emulator quits
// (ESC or closing the window).
// ============================================================================
#include "include/fake6502.h"

int main(int argc, char **argv) {
  fake6502Init(argc, argv);
  return 0;
}
