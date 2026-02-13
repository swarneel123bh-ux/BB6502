#pragma once

#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Full address space of the 6502
extern uint8_t *MEM6502;

// Address metadata (FIXED, ALL 6502 PROGS MUST WRITE ITS OWN ADDRS TO THESE
// ADDRS)
#define UARTINREG_ADDR 0xFFEA
#define UARTOUTREG_ADDR 0xFFEC
#define IXFLAGREG_ADDR 0xFFEE

/*
ixFlagReg description :-
b7: set    => Exit request
b6: set    => Program wants to return control to debugger but not exit yet
b5: set    => Video output
b4...b3    => Unused
b2: set    => Character byte (get_c) request
b1: reset  => UART INPUT REGISTER ready to write to.
b0: set    => UART OUTPUT REGISTER is waiting to be read from.
*/

// Shared data that both the 6502 and the debugger/emulator can modify
// Mostly for signal passing between debugger and emulator (SUBJECT TO CHANGE)
extern uint16_t uartInReg;  // Address of the UART Input Register
extern uint16_t uartOutReg; // Address of the UART Output Register
extern uint16_t ixReg;      // Address of the Interface Status Register

// Function to trigger an interrupt because of a keypress
// Sends a byte to uart and calls irq
// The emulator will respond to a get_c request when b3 of ixReg is set, the irq
// must clear the bit
extern void sendToUart(uint8_t k);

// Function to read from the uart when b0 ixReg flag is set
extern uint8_t readFromUart(void);
