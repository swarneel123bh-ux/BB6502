// Rewriting as a single Header+C file because of all the complicated header stuff
// Global includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <ncurses.h>
// Project includes
#include "debugger.h"
#include "vobjdump.h"
#include "fake6502.h"

// Data Macros
#define UARTINREG_ADDR 	0xFFEA
#define UARTOUTREG_ADDR 0xFFEC
#define IXFLAGREG_ADDR 	0xFFEE

// Convenience Macros
#define dbgStrStartsWith(s1, s2) dbg_strStartsWith(s1, strlen(s1), s2, strlen(s2))
#define dbgStrIsAllCaps(s) dbg_strIsAllCaps(s, strlen(s))

// DECLARATIONS
// Internal functions
static void dbgReset(void);
static void dbgSendToUart(uint8_t k);
static uint8_t dbgReadFromUart(void);
static int dbgRunProgCont(void);
static void dbgDisplayHelp(void);
static void dbgDisasmInstr(char **cmdtoks, size_t cmdtoksiz);
static void dbgAddBp(char **cmdtoks, size_t cmdtoksiz);
static void dbgRunDbgCont(void);
static void dbgListBps(void);
static void dbgPrintMemRange(char *cmdtoks[], size_t cmdtoklen);
static void dbgPrintRegisters(void);
//static bool dbgCheck6502Exit(void);
static void dbgStep6502(char *cmdtoks[], size_t cmdtoksize);
static int dbgPerformChecks(void);
static bool dbgCheckIfAtBp(uint16_t pc, int instrlen, int *bpnum);
static void dbgSigintHandlerTerminal(int num);
static void dbgSigintHandlerConsole(int num);
static bool dbg_strStartsWith(const char *s1, size_t s1len, const char *s2, size_t s2len);
static bool dbg_strIsAllCaps(const char* s, size_t slen);
static uint16_t dbgReadHexU16(void);
static void dbgShowMem(uint16_t startaddr, uint16_t endaddr);
// static int dbgGetKeyAsync(void);
// static int dbgGetCharacter(void);
// static int dbgGetLine(char *linebuf, size_t linebufsiz);
static void dbgParseCmdLineArgs(int argc, char **argv);
static void dbgSetBp(uint16_t bp, char symbol[MAX_SYMBOL_LENGTH]);
static void dbgRmvBp(uint16_t bp);
static void dbgReadDbgSyms(const char *fname);
static int dbgStrToInt(const char *s, int *out);
static int dbgGetCmdTokens(char *cmdString, char *tokenArray[], size_t tokenArraySize);
static command_t dbgParseCommand(const char *cmd);
static void dbgSendToUart(uint8_t k);
static uint8_t dbgReadFromUart(void);
static void dbgInitDisplay(void);
static char dbgConsoleGetchar(void);
static void dbgConsoleEcho(const char *fmt, ...);
static void dbgConsoleGetCmd(char *buf, size_t bufsiz);
static void dbgTerminalPutchar(char c);
static char dbgTerminalGetchar(void);
static int dbgDisasmInstrFromPc(uint16_t pc, uint8_t (*read)(uint16_t), char *out);
static void dbgRemoveBreakpoint(char **cmdtoks, size_t cmdtoksiz);

// Variables
static bool dbgRunning, dbgInsideTerminal, dbgCurrentlyAtBp;
static char dbgCmdBuf[100], *dbgBinFileName, *dbgSrcFileName, *dbgSymFileName;
static int dbgSignal, dbgMainWinMaxLines, dbgMainWinMaxCols, dbgUiType, dbgBpListSize, dbgNofBps, dbgNofSyms;
static window_t dbgDbgWin, dbgTermWin, dbgSrcWin;
static breakpoint_t *dbgBpList;
static uint8_t *dbgMEM6502;
static uint16_t dbgUartInReg, dbgUartOutReg, dbgIxReg;
static struct vobj_symbol *dbgSymbols;

// DEFINITIONS:-

uint8_t read6502(uint16_t address) {
	return dbgMEM6502[address];
}

void write6502(uint16_t address, uint8_t value) {
	dbgMEM6502[address] = value;
	return;
}

void dbgInit(int argc, char** argv) {
	dbgParseCmdLineArgs(argc, argv);

	FILE* f = fopen(dbgBinFileName, "rb");
	if (!f) {
		fprintf(stderr, "Failed to open binary file::");
		perror("fopen(): ");
		exit(1);
	}

	dbgMEM6502 = (uint8_t*) calloc(0x10000, sizeof(uint8_t));
	if (!dbgMEM6502) {
		fprintf(stderr, "Failed to allocate memory for 6502 addrspace::");
		perror("calloc(): ");
		fclose(f);
		exit(1);
	}

	uint8_t tempbyte = 0x00;
	uint16_t tempaddr = 0x0000;
	while (fread(&tempbyte, sizeof(uint8_t), 1, f)) {
		write6502(tempaddr, tempbyte);
    tempaddr++;
	}
	fclose(f);

	dbgUartInReg = 0; dbgUartOutReg = 0; dbgIxReg = 0;
	dbgUartInReg = 	read6502(UARTINREG_ADDR) 	| (read6502(UARTINREG_ADDR + 1) 	<< 8);
	dbgUartOutReg = read6502(UARTOUTREG_ADDR) | (read6502(UARTOUTREG_ADDR + 1) 	<< 8);
	dbgIxReg = 			read6502(IXFLAGREG_ADDR) 	| (read6502(IXFLAGREG_ADDR + 1) 	<< 8);
	dbgReadDbgSyms(dbgSymFileName);
	dbgInitDisplay();
	dbgConsoleEcho("Loading debug symbols from %s\n", dbgSymFileName);
	dbgConsoleEcho("uartInReg=%04hx\n", dbgUartInReg);
	dbgConsoleEcho("uartOutReg=%04hx\n", dbgUartOutReg);
	dbgConsoleEcho("dbgIxReg=%04hx\n", dbgIxReg);
	dbgRunning = false;
	dbgCurrentlyAtBp = false;
	dbgInsideTerminal = false;
	memset(dbgCmdBuf, 0, sizeof(dbgCmdBuf));
	dbgSignal = SIG_NOSIG;
	return;
}

void dbgCleanup(void) {
	free(dbgMEM6502);
	for (size_t i = 0; i < dbgNofBps; i ++) {
		if (dbgBpList[i].hasSymbol) {
			free(dbgBpList[i].symbol);
		}
	}
	free(dbgBpList);
	free(dbgSymbols);
	return;
}

void dbgMainLoop(void) {
  dbgReset();
  while (dbgRunning) {

    dbgConsoleGetCmd(dbgCmdBuf, sizeof(dbgCmdBuf));

    if (dbgCmdBuf[0] == '\n' || strlen(dbgCmdBuf) == 0)
      continue;
    size_t cmdTokArrSiz = 10;
    char *cmdtoks[cmdTokArrSiz];
    int cmdtoklen = dbgGetCmdTokens(dbgCmdBuf, cmdtoks, cmdTokArrSiz);

    switch (dbgParseCommand(cmdtoks[0])) {
    case CMD_BREAKPOINT:
      dbgAddBp(cmdtoks, cmdtoklen);
      break;

    case CMD_CONTINUE:
      dbgRunDbgCont();
      break;

    case CMD_DISASSEMBLE:
      dbgDisasmInstr(cmdtoks, cmdtoklen);
      break;

    case CMD_HELP:
      dbgDisplayHelp();
      break;

    case CMD_LISTBREAKPOINTS:
      dbgListBps();
      break;

    case CMD_MEMORY:
      dbgPrintMemRange(cmdtoks, cmdtoklen);
      break;

    case CMD_REGISTERS:
      dbgPrintRegisters();
      break;

    case CMD_STEP:
      dbgStep6502(cmdtoks, cmdtoklen);
      break;

    case CMD_QUIT:
      dbgRunning = false;
      break;

    case CMD_REMOVEBP:
    	dbgRemoveBreakpoint(cmdtoks, cmdtoklen);

    default:
      break;
    }
  }
}

static void dbgParseCmdLineArgs(int argc, char **argv) {
	if (argc < 2) {
    fprintf(stdout, "usage: %s <binary> [flags]\n", argv[0]);
    fprintf(stdout, "\tflags:-\n");
    fprintf(stdout, "\t\t-d <filename>: load debug symbols\n");
    fprintf(stdout, "\t\t-s <filename>: load source code\n");
    fprintf(stdout, "\t\t-u <type[tui/gui]>: interface type\n");
    exit(0);
  }

  dbgBinFileName = argv[1];

  if (argc < 3) {
    dbgSrcFileName = NULL;
    dbgSymFileName = NULL;
    dbgUiType = 0;
    return;
  }

  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      if (i >= argc - 1 || argv[i + 1][0] == '-') {
        fprintf(stderr, "Missing argument file: -d <filename>\n");
        exit(1);
      }
      dbgSymFileName = argv[i + 1];
    }

    // Load source code
    if (strcmp(argv[i], "-s") == 0) {
      if (i >= argc - 1 || argv[i + 1][0] == '-') {
        fprintf(stderr, "Missing argument file: -s <filename>\n");
        exit(1);
      }
      dbgSrcFileName = argv[i];
    }

    // Set ui type
    if (strcmp(argv[i], "-u") == 0) {
      if (i >= argc - 1 || argv[i + 1][0] == '-') {
        fprintf(stderr, "Missing argument option: -u <TUI/gui>\n");
        fprintf(stderr, "Defaulting to TUI\n");
        dbgUiType = 0;
      } else if (strcmp(argv[i + 1], "gui") == 0) {
        dbgUiType = 1;
      } else if (strcmp(argv[i + 1], "tui") == 0) {
        dbgUiType = 0;
      } else {
        fprintf(stderr, "Invalid option for arg -u: %s\n", argv[i + 1]);
        fprintf(stderr, "Defaulting to TUI\n");
        dbgUiType = 0;
      }
    }
  }
  return;
}

static uint16_t dbgReadHexU16(void) {
	char buf[32];
  char *end;
  unsigned long value;

  if (!fgets(buf, sizeof(buf), stdin)) {
    dbgConsoleEcho("\tInput error\n");
    exit(EXIT_FAILURE);
  }

  errno = 0;
  value = strtoul(buf, &end, 0);

  if (errno != 0 || end == buf) {
    dbgConsoleEcho("Invalid hex number\n");
    exit(EXIT_FAILURE);
  }

  if (value > 0xFFFF) {
    dbgConsoleEcho("Value out of 16-bit range\n");
    exit(EXIT_FAILURE);
  }

  return (uint16_t)value;
}

static void dbgShowMem(uint16_t startaddr, uint16_t endaddr){
  uint16_t addr = startaddr;
  while (1) {
    dbgConsoleEcho("%04X: ", addr);
    for (uint16_t offset = 0; offset < 16; offset++) {
      uint16_t cur = addr + offset;
      if (cur < addr || cur > endaddr) break;
      dbgConsoleEcho("%02X ", read6502(cur));
    }
    dbgConsoleEcho("\n");
    if (addr > endaddr - 16) break;
    addr += 16;
  }
  return;
}

static void dbgReset(void) {
	reset6502();
  write6502(dbgIxReg, 0x00);
  dbgConsoleEcho("\n");
  dbgRunning = true;
  dbgCurrentlyAtBp = false;
  dbgInsideTerminal = false;
  return;
}

static void dbgSendToUart(uint8_t k) {
	while (read6502(dbgIxReg & 0x01)) { }
  write6502(dbgUartInReg, k);
  irq6502();
  return;
}

static uint8_t dbgReadFromUart(void) {
	uint8_t r = read6502(dbgUartOutReg);
  write6502(dbgIxReg, read6502(dbgIxReg) & 0xFE); // Clear b0 to allow further put_c calls
  return r;
}

static int dbgRunProgCont(void) {
	signal(SIGINT, dbgSigintHandlerTerminal);

  static int atBreakpoint = -1;
  dbgInsideTerminal = true;
  while (dbgInsideTerminal) {

    if (dbgSignal == SIG_INTERRUPT_TERMINAL) {
      dbgInsideTerminal = false;
      break;
    }

    int sig = dbgPerformChecks();
    if (sig == SIG_PROGRAM_EXITED || sig == SIG_CONTROL_RETURNED) {
      dbgSignal = sig;
      dbgInsideTerminal = false;
      break;
    }

    int k = dbgTerminalGetchar();
    if (k != -1) dbgSendToUart(k);

    int instrlen=0;
    char line[64];
    memset(line, 0, sizeof(line));
    dbgDisasmInstrFromPc(pc, read6502, line);
    if (dbgCurrentlyAtBp) {
      dbgCurrentlyAtBp = false;
      step6502();
      continue;
    }

    int bpnum;
    if (dbgCheckIfAtBp(pc, instrlen, &bpnum)) {
      atBreakpoint = bpnum;
      break;
    }

    step6502();
  }

  signal(SIGINT, dbgSigintHandlerConsole);
  return atBreakpoint;
}

static void dbgDisplayHelp(void) {
	dbgConsoleEcho("Help : -\n");
  dbgConsoleEcho( "b [./addr/symbol]: Create a breakpoint at [curr instr/addr/symbol]\n");
  dbgConsoleEcho("c: Continue the program from current state\n");
  dbgConsoleEcho("d [n]: Disassemble next 5/[n] instructions\n");
  dbgConsoleEcho("h: Display this help\n");
  dbgConsoleEcho("l: List all breakpoints\n");
  dbgConsoleEcho( "m [start] [end]: Display memory contents from a start location to an " "end location\n");
  dbgConsoleEcho("r: Display the register contents\n");
  dbgConsoleEcho("s [n]: Step the program with one/[n] instruction\n");
  dbgConsoleEcho("\n");
  return;
}

static void dbgDisasmInstr(char **cmdtoks, size_t cmdtoksiz) {
	int nofLines = 5;
  if (cmdtoksiz < 2) {
    nofLines = 5;
  } else if (!dbgStrToInt(cmdtoks[1], &nofLines)) {
    dbgConsoleEcho("cmd parsing err... %s %s\n", cmdtoks[0], cmdtoks[1]);
    return;
  }

  uint16_t pc_ = pc;
  int instrlen;

  for (int i = 0; i < nofLines; i++) {
    char line[64];
    memset(line, 0, sizeof(line));
    instrlen = dbgDisasmInstrFromPc(pc_, read6502, line);
    dbgConsoleEcho("\t%04hx\t%s\n", pc_, line);
    pc_ += (uint16_t)instrlen;
  }

  return;
}

static void dbgAddBp(char **cmdtoks, size_t cmdtoksiz) {
  // No symbol/addr, set at curret pc
  if (cmdtoksiz < 2) {
    dbgConsoleEcho("  Setting new breakpoint at 0x%04hx\n", pc);
    dbgSetBp(pc, NULL); // If no addr/symbol given set at current addr
    return;
  }

  // Check whether symbol/addr is given
  if (dbgStrStartsWith("0x", cmdtoks[1])) { // Addr given
    char *end;
    uint16_t bp = strtoul(cmdtoks[1], &end, 0);
    dbgConsoleEcho("  Setting new breakpoint at 0x%04hx\n", bp);
    dbgSetBp(bp, NULL);
    return;
  }

  // Symbol given
  for (int i = 0; i < dbgNofSyms; i++) {
    if (strcmp(cmdtoks[1], dbgSymbols[i].symbolname) == 0) {
      dbgConsoleEcho( "  Setting new breakpoint at name=%s, addr=0x%04hx\n", dbgSymbols[i].symbolname, BPTMASK(dbgSymbols[i].val));
      dbgSetBp((uint16_t)dbgSymbols[i].val, dbgSymbols[i].symbolname);
      return;
    }
  }

  dbgConsoleEcho("\tsymbol=%s not found!\n", cmdtoks[1]);
  return;
}

static void dbgRunDbgCont(void) {
	noecho();
  dbgSignal = SIG_NOSIG;
  dbgConsoleEcho("\n\tGoing into continuous execution\n");
  int brkpt = dbgRunProgCont();
  echo();

  dbgConsoleEcho("\n\tBack into debugger\n");
  if (dbgSignal == SIG_PROGRAM_EXITED) {
    dbgConsoleEcho("\n\tProgram exited. Restart/Exit? [r/E] :");
    int c = dbgConsoleGetchar();
    if (c == 'r') {
      dbgReset();
    } else {
      dbgRunning = false;
      return;
    }
  }

  if (brkpt >= 0) {
    char buf[32];
    memset(buf, 0, sizeof(buf));
    dbgDisasmInstrFromPc(pc, read6502, buf);
    dbgConsoleEcho("  Breakpoint [%d]:%s at addr %04hx: %s\n", brkpt, (dbgBpList[brkpt].hasSymbol) ? dbgBpList[brkpt].symbol : "", dbgBpList[brkpt].address, buf);
    dbgCurrentlyAtBp = true;
  }

  return;
}

static void dbgListBps(void) {

  if (dbgNofBps <= 0) {
    dbgConsoleEcho("		No breakpoints set\n");
    return;
  }

  for (int i = 0; i < dbgNofBps; i++) {
    char line[64];
    memset(line, 0, sizeof(line));
    dbgDisasmInstrFromPc(dbgBpList[i].address, read6502, line);
    if (dbgBpList[i].symbol) {
      dbgConsoleEcho("\t%s: 0x%04hx\t%s\n", dbgBpList[i].symbol, dbgBpList[i].address, line);
    } else {
      dbgConsoleEcho("\t\t0x%04hx\t%s\n", dbgBpList[i].address, line);
    }
  }

  return;
}

static void dbgPrintMemRange(char *cmdtoks[], size_t cmdtoklen) {

  uint16_t startAddr, endAddr;

  // Both addrs not given
  if (cmdtoklen < 2) {
    dbgConsoleEcho("Give the starting address in hex [0x...]: ");
    startAddr = dbgReadHexU16();
    dbgConsoleEcho("Give the ending address in hex [0x...]: ");
    endAddr = dbgReadHexU16();
    if (endAddr < startAddr) {
      dbgConsoleEcho("Invalid input %04hx\n", endAddr);
      return;
    }
    dbgShowMem(startAddr, endAddr);
    return;
  }

  // Start addr given but stop addr not given
  if (cmdtoklen < 3) {
    char* end_;
    startAddr = strtoul(cmdtoks[1], &end_, 0);
    dbgConsoleEcho("Give the ending address in hex [0x...]: ");
    endAddr = dbgReadHexU16();
    dbgShowMem(startAddr, endAddr);
    if (endAddr < startAddr) {
      dbgConsoleEcho("Invalid input %04hx\n", endAddr);
      return;
    }
    return;
  }
}

static void dbgPrintRegisters(void) {
	dbgConsoleEcho( "PC=%04hx SP=%04hx A=%04hx X=%04hx Y=%04hx status=%04hx\n", pc, sp, a, x, y, status);
	return;
}

static void dbgStep6502(char *cmdtoks[], size_t cmdtoksize) {
	int nsteps = 1;

  if (cmdtoksize < 2) {
    nsteps = 1;
  } else {
    if (!dbgStrToInt(cmdtoks[1], &nsteps)) {
      dbgConsoleEcho("cmd parsing err... %s %s\n", cmdtoks[0], cmdtoks[1]);
      return;
    }
  }

  for (int i = 0; i < nsteps; i++) {
    int instrlen;
    char line[64];
    memset(line, 0, sizeof(line));
    instrlen = dbgDisasmInstrFromPc(pc, read6502, line);

    if (dbgCurrentlyAtBp) {
      dbgConsoleEcho("  Stepping through breakpoint...\n");
      dbgConsoleEcho("\t%04hx\t%s\n", pc, line);
      step6502();
      dbgCurrentlyAtBp = false;
      dbgPerformChecks();
      continue;
    }

    int bpNum = -1;
    if (dbgCheckIfAtBp(pc, instrlen, &bpNum)) {
      dbgConsoleEcho("  At Breakpoint [%d] %s: %04hx   %s\n", (dbgBpList[bpNum].symbol) ? dbgBpList[bpNum].symbol : "", bpNum, pc, line);
      dbgCurrentlyAtBp = true;
      return;
    }

    dbgConsoleEcho("\t%04hx\t%s\n", pc, line);
    step6502();
    dbgPerformChecks();
    continue;
  }
}

static int dbgPerformChecks(void) {
	if (read6502(dbgIxReg) & 0x80) {
    return SIG_PROGRAM_EXITED;
  }

  if (read6502(dbgIxReg) & 0x40) {
    return SIG_CONTROL_RETURNED;
  }

  if (read6502(dbgIxReg) & 0x01) {
    dbgTerminalPutchar(dbgReadFromUart());
    return SIG_TEXTOUT;
  }

  if (read6502(dbgIxReg) & 0x20) {
    return SIG_VGAOUT;
  }

  return SIG_NOSIG;
}

static bool dbgCheckIfAtBp(uint16_t pc, int instrlen, int *bpnum) {
  uint16_t instrStartAddr = pc;
  uint16_t instrEndAddr = pc + instrlen;

  for (int i = 0; i < dbgNofBps; i++) {
    if ((instrStartAddr <= dbgBpList[i].address) &&
        (dbgBpList[i].address < instrEndAddr)) {
      *bpnum = i;
      return true;
    }
  }
  *bpnum = -1;
  return false;
}

static void dbgSigintHandlerTerminal(int num) {
	dbgSignal = SIG_INTERRUPT_TERMINAL;
  signal(SIGINT, SIG_DFL); // Reset the sigint to quit
  dbgConsoleEcho("\tSIG_INTERRUPT_TERMINAL\n");
  return;
}

static void dbgSigintHandlerConsole(int num) {
	return;
}

static bool dbg_strStartsWith(const char *s1, size_t s1len, const char *s2, size_t s2len) {
	if (s1len > s2len)
    return false;

  for (int i = 0; i < s1len; i++) {
    if (s1[i] != s2[i])
      return false;
  }

  return true;
}

static bool dbg_strIsAllCaps(const char* s, size_t slen) {
  for (int i = 0; i < slen; i++) {
    if (isdigit(s[i]))
      continue;

    if (isalpha(s[i]) && !(s[i] >= 'A' && s[i] <= 'Z'))
      return 0;
  }

  return 1;
}

// static int dbgGetKeyAsync(void);
// static int dbgGetCharacter(void);
// static int dbgGetLine(char *linebuf, size_t linebufsiz);

static void dbgSetBp(uint16_t addr, char symbol[MAX_SYMBOL_LENGTH]) {
  if (dbgBpListSize <= 0) {
    dbgBpListSize = 10;
    dbgBpList = (breakpoint_t *)malloc(sizeof(breakpoint_t) * dbgBpListSize);
    memset(dbgBpList, 0x0000, sizeof(breakpoint_t) * dbgBpListSize);
  }

  if ((double)(dbgNofBps / dbgBpListSize) > 0.7f) {
    dbgBpListSize *= 2;
    breakpoint_t *tmp = realloc(dbgBpList, dbgBpListSize * sizeof(breakpoint_t));
    if (!tmp) {
      dbgConsoleEcho("setBreakpoint [fatal]: realloc:");
      perror(" ");
      exit(1);
    }
    dbgBpList = tmp;
  }

  dbgBpList[dbgNofBps].address = addr;

  // Check if a valid symbol is given
  if (!symbol) {
    dbgBpList[dbgNofBps].symbol = NULL;
    dbgNofBps++;
    return;
  }

  // Write symbol if valid
  dbgBpList[dbgNofBps].symbol = (char *)malloc(sizeof(char) * MAX_SYMBOL_LENGTH);
  strncpy(dbgBpList[dbgNofBps].symbol, symbol, MAX_SYMBOL_LENGTH - 1);
  dbgSymbols[dbgNofBps++].symbolname[strlen(symbol)] = 0;
  return;
}

static void dbgRmvBp(uint16_t bp) {
	int l = 0, r = dbgNofBps - 1;
  int mid = (l + r) / 2;

  while (dbgBpList[mid].address != bp && l < r) {
    if (bp < dbgBpList[mid].address)
      r = mid - 1;
    else if (bp > dbgBpList[mid].address)
      l = mid + 1;
  }

  if (l >= r) {
    return;
  }
  if (dbgBpList[mid].address == bp) {
    dbgBpList[mid].address = 0xFFFF; // Removed breakpoint because in 6502 mem 0xFFFF is irq's vector
    // Need to take care of this somehow later
  }
  return;
}

static void dbgReadDbgSyms(const char *fname) {
	if (!fname) {
    fprintf(stderr, "vobjdump: Not a valid file");
    return;
  }

  // Open the file and read symbols using vobjdump functions
  FILE *f = fopen(fname, "rb");
  if (!f) {
    fprintf(stderr, "vobjdump: Cannot open \"%s\" for reading!\n", fname);
    return;
  }

  vlen = filesize(f, fname);
  if (!vlen) {
    fprintf(stderr, "vobjdump: Read error on \"%s\"!\n", fname);
    fclose(f);
    return;
  }

  vobj = malloc(vlen);
  if (!vobj) {
    fprintf(stderr,
            "vobjdump: Unable to allocate %lu bytes "
            "to buffer file \"%s\"!\n",
            vlen, fname);
    fclose(f);
    return;
  }

  if (fread(vobj, 1, vlen, f) != vlen) {
    fprintf(stderr, "vobjdump: Read error on \"%s\"!\n", fname);
    free(vobj);
    fclose(f);
    return;
  }

  struct vobj_symbol *vsymbols = NULL;
  int nsymsRead;
  vsymbols = vobjdump(&nsymsRead); // Edited vobjdump function, see vobjdump.c
  // printf("%p\n", (void *)vsymbols);
  // printf("Read %d symbols from file %s\n", nsymsRead, fname);
  dbgNofSyms = 0;

  // Loop once to find out how many debug symbols exist
  for (int i = 0; i < nsymsRead; i++) {
    struct vobj_symbol s = vsymbols[i];

    if (dbgStrStartsWith(".", s.name) || dbgStrStartsWith("org", s.name) ||
        dbgStrIsAllCaps(s.name))
      continue;

    dbgNofSyms++;
  }

  // Allocate mem
  dbgSymbols = (struct vobj_symbol *)malloc(sizeof(struct vobj_symbol) * dbgNofSyms);

  // Loop again to populate dbgSymbols
  for (int i = 0, k = 0; i < nsymsRead && k < dbgNofSyms; i++) {
    struct vobj_symbol s = vsymbols[i];

    // Logic to determine if label goes here
    if (dbgStrStartsWith(".", s.name) || dbgStrStartsWith("org", s.name) ||
        dbgStrIsAllCaps(s.name))
      continue;

    dbgSymbols[k] = s;
    strncpy(dbgSymbols[k].symbolname, s.name, MAX_SYMBOL_LENGTH - 1);
    dbgSymbols[k].symbolname[strlen(s.name)] = 0; // append 0 at the end of the symbol name
    k++;
  }

  free(vobj);
  fclose(f);
  return;
}

static int dbgStrToInt(const char *s, int *out) {
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

static int dbgGetCmdTokens(char *cmdString, char *tokenArray[], size_t tokenArraySize) {
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

static command_t dbgParseCommand(const char *cmd) {
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

static void dbgInitDisplay(void) {
	initscr();
  keypad(stdscr, TRUE);
  cbreak();
  nl();
  refresh();
  getmaxyx(stdscr, dbgMainWinMaxLines, dbgMainWinMaxCols);
  dbgDbgWin.winw = dbgMainWinMaxCols - 2;
  dbgDbgWin.winh = dbgMainWinMaxLines - (dbgMainWinMaxLines / 2) - 3;
  dbgDbgWin.winx = 1;
  dbgDbgWin.winy = (dbgMainWinMaxLines / 2) + 2;
  dbgDbgWin.winbg = newwin(dbgDbgWin.winh + 2, dbgDbgWin.winw + 2, dbgDbgWin.winy - 1, dbgDbgWin.winx - 1);
  dbgDbgWin.win = newwin(dbgDbgWin.winh, dbgDbgWin.winw, dbgDbgWin.winy, dbgDbgWin.winx);
  box(dbgDbgWin.winbg, 0, 0);
  scrollok(dbgDbgWin.win, TRUE);
  keypad(dbgDbgWin.win, TRUE);
  wrefresh(dbgDbgWin.winbg);
  wrefresh(dbgDbgWin.win);
  dbgTermWin.winw = dbgMainWinMaxCols - (dbgMainWinMaxCols / 2) - 2;
  dbgTermWin.winh = dbgMainWinMaxLines - (dbgMainWinMaxLines / 2) - 2;
  dbgTermWin.winx = 1;
  dbgTermWin.winy = 1;
  dbgTermWin.winbg = newwin(dbgTermWin.winh + 2, dbgTermWin.winw + 2, dbgTermWin.winy - 1, dbgTermWin.winx - 1);
  dbgTermWin.win = newwin(dbgTermWin.winh, dbgTermWin.winw, dbgTermWin.winy, dbgTermWin.winx);
  box(dbgTermWin.winbg, 0, 0);
  scrollok(dbgTermWin.win, TRUE);
  nodelay(dbgTermWin.win, TRUE);
  keypad(dbgTermWin.win, TRUE);
  wrefresh(dbgTermWin.winbg);
  wrefresh(dbgTermWin.win);
  dbgSrcWin.winw = dbgMainWinMaxCols - (dbgMainWinMaxCols / 2) - 2;
  dbgSrcWin.winh = dbgMainWinMaxLines - (dbgMainWinMaxLines / 2) - 2;
  dbgSrcWin.winx = dbgTermWin.winx + dbgTermWin.winw + 2;
  dbgSrcWin.winy = 1;
  dbgSrcWin.winbg = newwin(dbgSrcWin.winh + 2, dbgSrcWin.winw + 2, dbgSrcWin.winy - 1, dbgSrcWin.winx - 1);
  dbgSrcWin.win = newwin(dbgSrcWin.winh, dbgSrcWin.winw, dbgSrcWin.winy, dbgSrcWin.winx);
  box(dbgSrcWin.winbg, 0, 0);
  scrollok(dbgSrcWin.win, TRUE);
  keypad(dbgSrcWin.win, TRUE);
  wrefresh(dbgSrcWin.winbg);
  wrefresh(dbgSrcWin.win);
}

static char dbgConsoleGetchar(void) {
	return wgetch(dbgDbgWin.win);
}

static void dbgConsoleEcho(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int y = getcury(dbgDbgWin.win);
    int x = getcurx(dbgDbgWin.win);
    wmove(dbgDbgWin.win, y, x);
    vw_printw(dbgDbgWin.win, fmt, args);
    va_end(args);
    wrefresh(dbgDbgWin.win);
}

static void dbgConsoleGetCmd(char *buf, size_t bufsiz) {
	dbgConsoleEcho(">>");
  wgetnstr(dbgDbgWin.win, buf, bufsiz);
  return;
}

static void dbgTerminalPutchar(char c) {
	waddch(dbgTermWin.win, (c == '\r') ? '\n' : c);
  wrefresh(dbgTermWin.win);
  return;
}

static char dbgTerminalGetchar(void) {
	return wgetch(dbgTermWin.win);
}

static int dbgDisasmInstrFromPc(uint16_t pc, uint8_t (*read)(uint16_t), char *out) {
    uint8_t opcode = read(pc);
    const opcode_t *op = &optable[opcode];

    uint8_t b1 = read(pc + 1);
    uint8_t b2 = read(pc + 2);
    uint16_t addr = (uint16_t)(b1 | (b2 << 8));

    switch (op->mode) {
        case AM_IMP:  sprintf(out, "%s", op->mnemonic); break;
        case AM_ACC:  sprintf(out, "%s A", op->mnemonic); break;
        case AM_IMM:  sprintf(out, "%s #$%02X", op->mnemonic, b1); break;
        case AM_ZP:   sprintf(out, "%s $%02X", op->mnemonic, b1); break;
        case AM_ZPX:  sprintf(out, "%s $%02X,X", op->mnemonic, b1); break;
        case AM_ZPY:  sprintf(out, "%s $%02X,Y", op->mnemonic, b1); break;
        case AM_ABS:  sprintf(out, "%s $%04X", op->mnemonic, addr); break;
        case AM_ABSX: sprintf(out, "%s $%04X,X", op->mnemonic, addr); break;
        case AM_ABSY: sprintf(out, "%s $%04X,Y", op->mnemonic, addr); break;
        case AM_IND:  sprintf(out, "%s ($%04X)", op->mnemonic, addr); break;
        case AM_INDX: sprintf(out, "%s ($%02X,X)", op->mnemonic, b1); break;
        case AM_INDY: sprintf(out, "%s ($%02X),Y", op->mnemonic, b1); break;
        case AM_REL: {
            int8_t off = (int8_t)b1;
            uint16_t target = pc + 2 + off;
            sprintf(out, "%s $%04X", op->mnemonic, target);
            break;
        }
    }

    return op->bytes;
}

static void dbgRemoveBreakpoint(char **cmdtoks, size_t cmdtoksiz){
	// No symbol/addr, set at curret pc
  if (cmdtoksiz < 2) {
    dbgConsoleEcho("\tusage: rb(or alternate command) <symbol/0x(address)>\n");
    return;
  }

  // Check whether symbol/addr is given
  if (dbgStrStartsWith("0x", cmdtoks[1])) { // Addr given
    char *end;
    uint16_t bp = strtoul(cmdtoks[1], &end, 0);
    dbgConsoleEcho("\tRemoved breakpoint at addr=0x%04hx\n");
    dbgRmvBp(bp);
    return;
  }

  // Symbol given
  for (int i = 0; i < dbgNofSyms; i++) {
    if (strcmp(cmdtoks[1], dbgSymbols[i].symbolname) == 0) {
      dbgConsoleEcho( "\tRemoved breakpoint at name=%s, addr=0x%04hx\n", dbgSymbols[i].symbolname, BPTMASK(dbgSymbols[i].val));
      dbgRmvBp((uint16_t)dbgSymbols[i].val);
      return;
    }
  }

  dbgConsoleEcho("\tsymbol=%s not found!\n", cmdtoks[1]);
  return;
}
