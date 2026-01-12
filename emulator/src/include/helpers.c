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
uint16_t read_hex_u16(void) {
  char buf[32];
  char *end;
  unsigned long value;

  if (!fgets(buf, sizeof(buf), stdin)) {
    fprintf(stderr, "Input error\n");
    exit(EXIT_FAILURE);
  }

  errno = 0;
  value = strtoul(buf, &end, 0);  
  // base 0 => handles 0x..., 0X..., or plain hex/decimal

  if (errno != 0 || end == buf) {
    fprintf(stderr, "Invalid hex number\n");
    exit(EXIT_FAILURE);
  }

  if (value > 0xFFFF) {
    fprintf(stderr, "Value out of 16-bit range\n");
    exit(EXIT_FAILURE);
  }

  return (uint16_t)value;
}

// Shows memory contents of the 6502 from startaddr to endaddr
void showmem(uint16_t startaddr, uint16_t endaddr) {
    uint16_t addr = startaddr;

    while (1) {
      printf("%04X: ", addr);

      for (uint16_t offset = 0; offset < 16; offset++) {
        uint16_t cur = addr + offset;
        if (cur < addr || cur > endaddr)  // overflow OR past end
            break;

        printf("%02X ", read6502(cur));
      }
      printf("\n");

      /* Stop if next step would overflow or exceed endaddr */
      if (addr > endaddr - 16)
        break;

      addr += 16;
  }
}

// Get asynchronous key input from the user
int getKeyAsync() {
  unsigned char c;
  if (read(STDIN_FILENO, &c, 1) == 1) {
    return c;
  }
  return -1;
}

static struct termios oldt;
static int old_stdin_flags;

// Set up terminal for async function
void setupTerminal(void) {
  struct termios newt;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  // atexit(restoreTerminal); -> We remove this because we have to restore the terminal when we drop back to the debugger

  setvbuf(stdout, NULL, _IONBF, 0);

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  old_stdin_flags = flags;
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  return;
}

// Restore the terminal to old mode before exiting program
void restoreTerminal() { 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); 
  setvbuf(stdout, NULL, _IOLBF, 0);
  fcntl(STDIN_FILENO, F_SETFL, old_stdin_flags);
}

// Dropack handler
// For C23 standard we need to define the fucntion such that it takes 1 integer arg
// We arent using this anywhere yet
void sigintHandler(int signum) {
  restoreTerminal();
  signal(SIGINT, SIG_DFL);  // Reset the sigint to quit
  printf("Ctrl-C Received! Returning to debugger\n");
}

// Function to get character sync-ly without messing up the stdin buffer
int getCharacter() {
  char buf[32];
  memset(buf, 0, sizeof(buf));
  if (!fgets(buf, sizeof(buf), stdin)) {
    fprintf(stdin, "Input buffering failure, crashing...\n");
    exit(1);
  }
  int c = buf[0];
  return c;
}

