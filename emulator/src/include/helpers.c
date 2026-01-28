#include "shared.h"
#include "helpers.h"
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
        if (cur < addr || cur > endaddr) break;
        printf("%02X ", read6502(cur));
      } printf("\n");

      if (addr > endaddr - 16) break;
      addr += 16;
  }
}

// Get asynchronous key input from the user
int getKeyAsync() {
  unsigned char c;
  if (read(STDIN_FILENO, &c, 1) == 1) { return c; }
  return -1;
}

static struct termios oldt;
static int old_stdin_flags;

// Restore the terminal to old mode before exiting program
void restoreTerminal() {
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  setvbuf(stdout, NULL, _IOLBF, 0);
  fcntl(STDIN_FILENO, F_SETFL, old_stdin_flags);
}


// Dropack handler
void sigintHandler(int signum) {
  signal(SIGINT, SIG_DFL);      // Reset the sigint to quit
  runmode = STEPPING;
}

// Set up terminal for async function
void setupTerminal(void) {
  struct termios newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  setvbuf(stdout, NULL, _IONBF, 0);
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  old_stdin_flags = flags;
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
  signal(SIGINT, sigintHandler);
  return;
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


// Function to get a line
// Returns the size of the line read
int getLine(char* linebuf, size_t linebufsiz) {
  if (!fgets(linebuf, linebufsiz, stdin)) {
    return 0;
  }

  return strlen(linebuf);
}

int bpListSize = 0;
int nBreakpoints = 0;

// Creates a breakpoint by address in mem
// Symbol breakpoints need to be taken care of by caller
void setBreakpoint(uint16_t bp) {
  if (bpListSize <= 0) {
    bpListSize = 10;
    bpList = (breakpoint*) malloc(sizeof(breakpoint) * bpListSize);
    memset(bpList, 0xFFFF, sizeof(breakpoint) * bpListSize);
  }

  if ((float)(nBreakpoints / bpListSize) > 0.7f) {
    bpListSize *= 2;
    breakpoint* tmp = realloc(bpList, bpListSize * sizeof(breakpoint));
    if (!tmp) {
      fprintf(stderr, "setBreakpoint [fatal]: realloc:");
      perror(" ");
      exit(1);
    }
    bpList = tmp;
  }

  bpList[nBreakpoints++].address = bp;
  return;
}

// Removes created breakpoint by address
// Symbol breakpoints need to be taken care of by caller
void rmBreakpoint(uint16_t bp) {
  int l = 0, r = nBreakpoints - 1;
  int mid = (l + r) / 2;

  while (bpList[mid].address != bp && l < r) {
    if (bp < bpList[mid].address) r = mid - 1;
    else if (bp > bpList[mid].address) l = mid + 1;
  }

  if (l >= r) { return; }
  if (bpList[mid].address == bp) {
    bpList[mid].address = 0xFFFF; // Removed breakpoint because in 6502 mem 0xFFFF is irq's vector
    // Need to take care of this somehow later
  }
  return;
}


// Read debug symbols (only when -dsym flag given at entry or user points
// the debugger to the file internally). Parses the symbols and writes
// to a global symbol table. Returns the number of bytes read.
int readDbgSyms(FILE* f) {
	if (!f) {
		perror("fopen: ");
		return -1;
	}

	int linesRead = 0;
	char buf[256];
  while (fgets(buf, sizeof(buf), f)) {
    // parseDbgSymLine();
    linesRead ++;
  }
  printf("Read %d lines\n", linesRead);
  fclose(f);
	fclose(f);

	return linesRead;
}
