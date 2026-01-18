#pragma once

#include "shared.h"
#include "fake6502.h"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

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
// For C23 standard we need to define the fucntion such that it takes 1 integer arg
// We arent using this anywhere yet
void sigintHandler(int signum);

// Function to get character sync-ly without messing up the stdin buffer
int getCharacter();

// Function to get a whole line
int getLine(char* linebuf, size_t linebufsiz);
