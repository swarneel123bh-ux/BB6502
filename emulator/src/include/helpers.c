#include "helpers.h"
#include "fake6502.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void parseCmdLineArgs(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stdout, "usage: %s <binary> [flags]\n", argv[0]);
    fprintf(stdout, "\tflags:-\n");
    fprintf(stdout, "\t\t-d <filename>: load debug symbols\n");
    fprintf(stdout, "\t\t-s <filename>: load source code\n");
    fprintf(stdout, "\t\t-u <type[tui/gui]>: interface type\n");
    exit(0);
  }

  binfilename = argv[1];

  if (argc < 3) {
    srcfilename = NULL;
    dbgsymfilename = NULL;
    uitype = 0;
    return;
  }

  for (int i = 2; i < argc; i++) {
    // Load debug symbols file
    if (strcmp(argv[i], "-d") == 0) {
      if (i >= argc - 1 || argv[i + 1][0] == '-') {
        fprintf(stderr, "Missing argument file: -d <filename>\n");
        exit(1);
      }
      dbgsymfilename = argv[i + 1];
    }

    // Load source code
    if (strcmp(argv[i], "-s") == 0) {
      if (i >= argc - 1 || argv[i + 1][0] == '-') {
        fprintf(stderr, "Missing argument file: -s <filename>\n");
        exit(1);
      }
      srcfilename = argv[i];
    }

    // Set ui type
    if (strcmp(argv[i], "-u") == 0) {
      if (i >= argc - 1 || argv[i + 1][0] == '-') {
        fprintf(stderr, "Missing argument option: -u <TUI/gui>\n");
        fprintf(stderr, "Defaulting to TUI\n");
        uitype = 0;
      } else if (strcmp(argv[i + 1], "gui") == 0) {
        uitype = 1;
      } else if (strcmp(argv[i + 1], "tui") == 0) {
        uitype = 0;
      } else {
        fprintf(stderr, "Invalid option for arg -u: %s\n", argv[i+1]);
        fprintf(stderr, "Defaulting to TUI\n");
        uitype = 0;
      }
    }
  }

  return;
}

uint16_t read_hex_u16(void) {
  char buf[32];
  char *end;
  unsigned long value;

  if (!fgets(buf, sizeof(buf), stdin)) {
    DISPLAY_CONSOLE_ECHO("Input error\n");
    exit(EXIT_FAILURE);
  }

  errno = 0;
  value = strtoul(buf, &end, 0);

  if (errno != 0 || end == buf) {
    DISPLAY_CONSOLE_ECHO("Invalid hex number\n");
    exit(EXIT_FAILURE);
  }

  if (value > 0xFFFF) {
    DISPLAY_CONSOLE_ECHO("Value out of 16-bit range\n");
    exit(EXIT_FAILURE);
  }

  return (uint16_t)value;
}

void showmem(uint16_t startaddr, uint16_t endaddr) {
  uint16_t addr = startaddr;

  while (1) {
    DISPLAY_CONSOLE_ECHO("%04X: ", addr);

    for (uint16_t offset = 0; offset < 16; offset++) {
      uint16_t cur = addr + offset;
      if (cur < addr || cur > endaddr)
        break;
      DISPLAY_CONSOLE_ECHO("%02X ", read6502(cur));
    }
    DISPLAY_CONSOLE_ECHO("\n");

    if (addr > endaddr - 16)
      break;
    addr += 16;
  }
}

int bpListSize = 0;
int nBreakpoints = 0;

// Creates a breakpoint by address in mem
// Symbol breakpoints need to be taken care of by caller
void setBreakpoint(uint16_t addr) {
  // Check if memory alloced
  if (bpListSize <= 0) {
    bpListSize = 10;
    bpList = (breakpoint *)malloc(sizeof(breakpoint) * bpListSize);
    memset(bpList, 0xFFFF, sizeof(breakpoint) * bpListSize);
  }

  // Check if were close to filling up alloced memory
  if ((float)(nBreakpoints / bpListSize) > 0.7f) {
    bpListSize *= 2;
    breakpoint *tmp = realloc(bpList, bpListSize * sizeof(breakpoint));
    if (!tmp) {
      DISPLAY_CONSOLE_ECHO("setBreakpoint [fatal]: realloc:");
      perror(" ");
      exit(1);
    }
    bpList = tmp;
  }

  // Find closest instruction that is smaller than or equal to given addr,
  // set breakpoint to that address
  //
  //
  bpList[nBreakpoints++].address = addr;
  return;
}

// Removes created breakpoint by address
// Symbol breakpoints need to be taken care of by caller
void rmBreakpoint(uint16_t bp) {
  int l = 0, r = nBreakpoints - 1;
  int mid = (l + r) / 2;

  while (bpList[mid].address != bp && l < r) {
    if (bp < bpList[mid].address)
      r = mid - 1;
    else if (bp > bpList[mid].address)
      l = mid + 1;
  }

  if (l >= r) {
    return;
  }
  if (bpList[mid].address == bp) {
    bpList[mid].address =
        0xFFFF; // Removed breakpoint because in 6502 mem 0xFFFF is irq's vector
    // Need to take care of this somehow later
  }
  return;
}

// Read debug symbols (only when -dsym flag given at entry or user points
// the debugger to the file internally). Parses the symbols and writes
// to a global symbol table. Returns the number of bytes read.
void readDbgSyms(const char *fname) {
  FILE *f;
  char filename[32];

  if (fname) {
    strncpy(filename, fname, 32);
  } else {
    DISPLAY_CONSOLE_ECHO("Give symbol file name: ");
    DISPLAY_CONSOLE_GETCMD(filename);
  }

  f = fopen(filename, "rb");
  if (!f) {
    DISPLAY_CONSOLE_ECHO("fopen error! Couldnt open file: ");
    perror(" ");
    return;
  }

  char buf[256];
  while (fgets(buf, sizeof(buf), f)) {
    // parseDbgSymLine();
  }

  fclose(f);
  return;
}

int strToInt(const char *s, int *out) {
  char *end;
  long val;

  errno = 0;
  val = strtol(s, &end, 10);

  if (end == s)
    return 0;

  if ((errno == ERANGE) || val > INT_MAX || val < INT_MIN)
    return 0;

  if (*end != '\0')
    return 0;

  *out = (int)val;
  return 1;
}

int getCmdTokens(char *cmdString, char *tokenArray[], size_t tokenArraySize) {
  size_t count = 0;
  char *token;

  if (!cmdString || !tokenArray || tokenArraySize == 0)
    return 0;

  token = strtok(cmdString, " \t\n");

  while (token != NULL && count < tokenArraySize - 1) {
    tokenArray[count++] = token;
    token = strtok(NULL, " \t\n");
  }

  tokenArray[count] = NULL;
  return count;
}

CommandType parseCommand(const char *cmd) {
  if (!strcmp(cmd, "b"))
    return CMD_BREAKPOINT;
  if (!strcmp(cmd, "bp"))
    return CMD_BREAKPOINT;
  if (!strcmp(cmd, "break"))
    return CMD_BREAKPOINT;
  if (!strcmp(cmd, "breakpoint"))
    return CMD_BREAKPOINT;

  if (!strcmp(cmd, "c"))
    return CMD_CONTINUE;
  if (!strcmp(cmd, "cont"))
    return CMD_CONTINUE;
  if (!strcmp(cmd, "continue"))
    return CMD_CONTINUE;

  if (!strcmp(cmd, "d"))
    return CMD_DISASSEMBLE;
  if (!strcmp(cmd, "dis"))
    return CMD_DISASSEMBLE;
  if (!strcmp(cmd, "disas"))
    return CMD_DISASSEMBLE;
  if (!strcmp(cmd, "disassemble"))
    return CMD_DISASSEMBLE;

  if (!strcmp(cmd, "h"))
    return CMD_HELP;
  if (!strcmp(cmd, "help"))
    return CMD_HELP;

  if (!strcmp(cmd, "l"))
    return CMD_LISTBREAKPOINTS;
  if (!strcmp(cmd, "ls"))
    return CMD_LISTBREAKPOINTS;
  if (!strcmp(cmd, "list"))
    return CMD_LISTBREAKPOINTS;
  if (!strcmp(cmd, "lbp"))
    return CMD_LISTBREAKPOINTS;
  if (!strcmp(cmd, "listbp"))
    return CMD_LISTBREAKPOINTS;

  if (!strcmp(cmd, "ld"))
    return CMD_LOADSRC;
  if (!strcmp(cmd, "lds"))
    return CMD_LOADSRC;
  if (!strcmp(cmd, "load"))
    return CMD_LOADSRC;
  if (!strcmp(cmd, "ldsrc"))
    return CMD_LOADSRC;

  if (!strcmp(cmd, "m"))
    return CMD_MEMORY;
  if (!strcmp(cmd, "mem"))
    return CMD_MEMORY;
  if (!strcmp(cmd, "memory"))
    return CMD_MEMORY;

  if (!strcmp(cmd, "r"))
    return CMD_REGISTERS;
  if (!strcmp(cmd, "reg"))
    return CMD_REGISTERS;
  if (!strcmp(cmd, "regis"))
    return CMD_REGISTERS;
  if (!strcmp(cmd, "register"))
    return CMD_REGISTERS;
  if (!strcmp(cmd, "registers"))
    return CMD_REGISTERS;

  if (!strcmp(cmd, "rb"))
    return CMD_REMOVEBP;
  if (!strcmp(cmd, "rbp"))
    return CMD_REMOVEBP;
  if (!strcmp(cmd, "rmb"))
    return CMD_REMOVEBP;
  if (!strcmp(cmd, "rmbp"))
    return CMD_REMOVEBP;
  if (!strcmp(cmd, "remb"))
    return CMD_REMOVEBP;
  if (!strcmp(cmd, "rembp"))
    return CMD_REMOVEBP;
  if (!strcmp(cmd, "remove"))
    return CMD_REMOVEBP;
  if (!strcmp(cmd, "removebp"))
    return CMD_REMOVEBP;

  if (!strcmp(cmd, "s"))
    return CMD_STEP;
  if (!strcmp(cmd, "step"))
    return CMD_STEP;

  if (!strcmp(cmd, "q"))
    return CMD_QUIT;
  if (!strcmp(cmd, "quit"))
    return CMD_QUIT;
  if (!strcmp(cmd, "exit"))
    return CMD_QUIT;

  return CMD_INVALIDCMD;
}
