#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "helpers.h"
#include "display.h"

// Global vars
extern bool dbgRunning;
extern bool insideTerminal;
extern bool currentlyAtBp;
extern char buf[100];
extern int signal_;          // G// Address of the Interface Status Registerlobal signal tracker

// Creates debugger window with ncurses
extern void initDbg(void);

// Deallocates any data and ensures a clean exit
extern void cleanUpDbg(void);

// Basically reinitializes the debugger without intializing ncurses
// Breakpoints remain
// processor is resetted
extern void resetDbg(void);

// Send a keyhit to UART and cause interrupt
extern void sendToUart(uint8_t k);

// Function to read from the uart when b0 ixReg flag is set
extern uint8_t readFromUart(void);

// Run the 6502 instructions asynchronously
extern int runContinuous(void);

// Run the debugger
extern void runDebugger(void);

// Display the help
extern void displayHelp(void);

// Decode the instruction current pc is at into assembly
extern void disassembleInstrs(char *cmdtoks[], size_t cmdtoksiz);

// Add a breakpoint
extern void addNewBreakpoint(char *cmdtoks[], size_t cmdtoksiz);

// Handles runContinuous command
extern void runDebuggerContinuous(void);

// List all the breakpoints currently set
extern void listAllBreakpoints(void);

// Print memory contents
extern void printMemRange(char *cmdtoks[], size_t cmdtoklen);

// Show register contents
extern void printRegisters(void);

// Chekc if 6502 program called for exit
extern bool progExited(void);

// Step the 6502 for given number of steps
extern void performStep(char *cmdtoks[], size_t cmdtoksize);

// Perform all interfacing checks and returns the signal for the action performed
extern int performChecks(void);

// Check if current pc is a breakpoint
extern bool checkIfAtBreakpoint(uint16_t pc, int instrlen, int* bpnum);

// Print to 6502 terminal
//void putcharVGA(char c, window_t terminal);

// Handle Ctrl+C signal when inside console/terminal
extern void sigintHandlerTerminal(int num);
extern void sigintHandlerConsole(int num);
