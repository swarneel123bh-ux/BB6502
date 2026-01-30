#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Global vars
bool dbgRunning;
bool currentlyAtBp;
char buf[100];

// Send a keyhit to UART and cause interrupt
void sendToUart(uint8_t k);

// Function to read from the uart when b0 ixReg flag is set
uint8_t readFromUart();

// Run the 6502 instructions asynchronously
int runContinuous(int* signal);

// Run the debugger
void runDebugger();

// Display the help
void displayHelp();

// Decode the instruction current pc is at into assembly
void disassembleInstrs(char *cmdtoks[], size_t cmdtoksiz);

// Add a breakpoint
void addNewBreakpoint(char *cmdtoks[], size_t cmdtoksiz);

// Handles runContinuous command
void runDebuggerContinuous();

// List all the breakpoints currently set
void listAllBreakpoints();

// Print memory contents
void printMemRange(char *cmdtoks[], size_t cmdtoklen);

// Show register contents
void printRegisters();

// Chekc if 6502 program called for exit
bool progExited();

// Step the 6502 for given number of steps
void performStep(char *cmdtoks[], size_t cmdtoksize);

// Perform all interfacing checks and returns the signal for the action performed
int performChecks();

bool checkIfAtBreakpoint(uint16_t pc, int instrlen, int* bpnum);
