#pragma once

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
  CMD_INVALIDCMD,
  CMD_REMOVEBP,
  CMD_BREAKPOINT,
  CMD_CONTINUE,
  CMD_DISASSEMBLE,
  CMD_HELP,
  CMD_LISTBREAKPOINTS,
  CMD_MEMORY,
  CMD_REGISTERS,
  CMD_STEP,
  CMD_QUIT
} CommandType;

// Enum to represent all possible signals
typedef enum {
  SIG_NOSIG,
  SIG_PROGRAM_EXITED,
  SIG_CONTROL_RETURNED,
  SIG_OUTPUT_REQUEST
} SignalType;

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
void sigintHandler(int signum);

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

// Creates a breakpoint via address
// Symbol breakpoints need to be converted to address by caller
void setBreakpoint(uint16_t bp);

// Removes breakpoint by addr
// Symbol breakpoints need to be converted to address by caller
void rmBreakpoint(uint16_t bp);

// Read debug symbols (only when -dsym flag given at entry or user points
// the debugger to the file internally)
int readDbgSyms(FILE *file);

// Takes a string, converts to integer if possible, returns flag
int strToInt(const char *s, int *out);

// Get a command string, parse into tokens
// Returns the number of tokens
int getCmdTokens(char *cmdString, char *tokenArray[], size_t tokenArraySize);

// Parses command string, returns a CommandType to be used inside a switch
// statement
CommandType parseCommand(const char *cmd);
