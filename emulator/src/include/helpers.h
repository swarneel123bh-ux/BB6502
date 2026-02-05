#pragma once

#include "display.h"
#include "fake6502.h"
#include "shared.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// Macro to get an input line
#define getCmd(cmbuf)                                                          \
  do {                                                                         \
    printf(" [bcdhlmrsq]>>");                                                  \
    if (getLine(cmbuf, sizeof(buf)) == 0) {                                    \
      fprintf(stderr, "getLine: input failure!\n");                            \
      exit(1);                                                                 \
    }                                                                          \
  } while (0);

// Enum to represent all possible commands
typedef enum {
  CMD_INVALIDCMD,      // Not a real command
  CMD_REMOVEBP,        // Remove a breakpoint
  CMD_BREAKPOINT,      // Add a new breakpoint
  CMD_CONTINUE,        // Continue execution of assembly from current PC
  CMD_DISASSEMBLE,     // Disassemble routine
  CMD_HELP,            // Display the help strings
  CMD_LISTBREAKPOINTS, // List all breakpoints currently set
  CMD_MEMORY,          // Display the 6502 memory within a given range
  CMD_REGISTERS,       // Display the 6502 register contents
  CMD_STEP,            // Step the 6502 a given number of steps
  CMD_QUIT,            // Quit the debugger
  CMD_LOADSRC,         // Load the 6502 assembly src from a given file
  CMD_LOADSYMS // Load the 6502 assembly's debug symbols from a given file
} CommandType;

// Enum to represent all possible signals
typedef enum {
  SIG_NOSIG,             // No signal from 6502 received
  SIG_PROGRAM_EXITED,    // 6502 sent program exit signal
  SIG_CONTROL_RETURNED,  // 6502 sent request to start stepping again
  SIG_TEXTOUT,           // 6502 sent request to output text to emulator console
  SIG_VGAOUT,            // 6502 sent request to output text to own terminal
  SIG_INTERRUPT_TERMINAL // Ctrl+C signal sent while inside terminal
} SignalType;

// Cmd line arg options
// binfilename[required] :
//  Path to the raw binary file
// srcfilename:
//  Path to the source code file for the currently executing binary
//  Default: NULL
// dbgsymfilename:
//  Path to the debug symbol file
//  Default: NULL
// uitype (tui [0] /gui [1]):
//  Type of UI to use
//  Default: TUI
char *binfilename, *srcfilename, *dbgsymfilename;
int uitype;

// Reads a hex value from the user and returns it as a uint16_t type
uint16_t read_hex_u16(void);

// Shows memory contents of the 6502 from startaddr to endaddr
void showmem(uint16_t startaddr, uint16_t endaddr);

// Get asynchronous key input from the user
int getKeyAsync();

// Termios data
static struct termios oldt;
static int old_stdin_flags;

// Set up terminal for async function
void setupTerminal(void);

// Restore the terminal to old mode before exiting program
void restoreTerminal();

// Dropack handler
// For C23 standard we need to define the fucntion such that it takes 1 integer
// arg We arent using this anywhere yet
// Function to get character sync-ly without messing up the stdin buffer
int getCharacter();

// Function to get a whole line
int getLine(char *linebuf, size_t linebufsiz);

// Breakpoint struct to track bps and if at bp
typedef struct Breakpoint {
  uint16_t address;
} breakpoint;

// Stores the list of currently active breakpoints
breakpoint *bpList;
int bpListSize;
int nBreakpoints;

// Parse command line args and set flags for initialization
void parseCmdLineArgs(int argc, char **argv);

// Creates a breakpoint via address
// Symbol breakpoints need to be converted to address by caller
void setBreakpoint(uint16_t bp);

// Removes breakpoint by addr
// Symbol breakpoints need to be converted to address by caller
void rmBreakpoint(uint16_t bp);

// Read debug symbols (only when -dsym flag given at entry or user points
// the debugger to the file internally)
void readDbgSyms(const char *fname);

// Takes a string, converts to integer if possible, returns flag
int strToInt(const char *s, int *out);

// Get a command string, parse into tokens
// Returns the number of tokens
int getCmdTokens(char *cmdString, char *tokenArray[], size_t tokenArraySize);

// Parses command string, returns a CommandType to be used inside a switch
// statement
CommandType parseCommand(const char *cmd);
