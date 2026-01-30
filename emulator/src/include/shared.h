#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Full address space of the 6502
uint8_t* MEM6502;


// Address metadata (FIXED, ALL 6502 PROGS MUST WRITE ITS OWN ADDRS TO THESE ADDRS)
#define UARTINREG_ADDR  0xFFEA
#define UARTOUTREG_ADDR 0xFFEC
#define IXFLAGREG_ADDR  0xFFEE


// Shared data that both the 6502 and the debugger/emulator can modify
// Mostly for signal passing between debugger and emulator (SUBJECT TO CHANGE)
uint16_t uartInReg;   // Address of the UART Input Register
uint16_t uartOutReg;  // Address of the UART Output Register
uint16_t ixReg;       // Address of the Interface Status Register

// Function to trigger an interrupt because of a keypress
// Sends a byte to uart and calls irq
// The emulator will respond to a get_c request when b3 of ixReg is set, the irq must clear the bit
void sendToUart(uint8_t k);

// Function to read from the uart when b0 ixReg flag is set
uint8_t readFromUart();

// 6502 screen state tracker
typedef struct screen6502 {
  char screen[65][65];      // 65 strings act like a 64 x 64 wide screen
  int cursx, cursy;         // Cursor positions acting as indices
} screen6502;




