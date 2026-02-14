// Rewriting as a single Header+C file because of all the complicated header stuff
// Global includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <ncurses.h>
// Project includes
#include "debugger.h"
#include "vobjdump.h"
#include "fake6502.h"

// Data Macros
#define UARTINREG_ADDR 0xFFEA
#define UARTOUTREG_ADDR 0xFFEC
#define IXFLAGREG_ADDR 0xFFEE

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
static bool dbgCheck6502Exit(void);
static void dbgStep6502(char *cmdtoks[], size_t cmdtoksize);
static int dbgPerformChecks(void);
static bool dbgCheckIfAtBp(uint16_t pc, int instrlen, int *bpnum);
static void dbgSigintHandlerTerminal(int num);
static void dbgSigintHandlerConsole(int num);
static int dbg_strStartsWith(const char *s1, size_t s1len, const char *s2, size_t s2len);
static int dbg_strIsAllCaps(const char* s, size_t slen);
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
void dbgInit(int argc, char** argv) {
	dbgParseCmdLineArgs(argc, argv);

	FILE* f = fopen(dbgBinFileName, "rb");
	if (!f) {
		fprintf(stderr, "Failed to open binary file::");
		perror("fopen(): ");
		exit(1);
	}

	dbgMEM6502 = (uint8_t*) calloc(0x1000, sizeof(uint8_t));
	if (!dbgMEM6502) {
		fprintf(stderr, "Failed to allocate memory for 6502 addrspace::");
		perror("calloc(): ");
		fclose(f);
		exit(1);
	}

	static uint8_t tempbyte = 0x00;
	static uint16_t tempaddr = 0x0000;
	while (fread(&tempbyte, sizeof(uint8_t), 1, f)) {
		write6502(tempaddr, tempbyte);
    tempaddr++;
	}
	fclose(f);

	dbgUartInReg = read6502(UARTINREG_ADDR) | (read6502(UARTINREG_ADDR + 1) << 8);
	dbgUartOutReg = read6502(UARTOUTREG_ADDR) | (read6502(UARTOUTREG_ADDR + 1) << 8);
	dbgIxReg = read6502(IXFLAGREG_ADDR) | (read6502(IXFLAGREG_ADDR) << 8);
	dbgReadDbgSyms(dbgSymFileName);
	dbgInitDisplay();
	dbgConsoleEcho("Loading debug symbolsd from %s\n", dbgSymFileName);
	dbgConsoleEcho("uartInReg=%04hx\n", dbgUartInReg);
	dbgConsoleEcho("uartOutReg=%04hx\n", dbgUartOutReg);
	dbgConsoleEcho("ixReg=%04hx\n", dbgIxReg);
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
    dbgConsoleEcho("Input error\n");
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

    int instrlen;
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

static void dbgListBps(void);
static void dbgPrintMemRange(char *cmdtoks[], size_t cmdtoklen);
static void dbgPrintRegisters(void);
static bool dbgCheck6502Exit(void);
static void dbgStep6502(char *cmdtoks[], size_t cmdtoksize);
static int dbgPerformChecks(void);
static bool dbgCheckIfAtBp(uint16_t pc, int instrlen, int *bpnum);
static void dbgSigintHandlerTerminal(int num);
static void dbgSigintHandlerConsole(int num);
static int dbg_strStartsWith(const char *s1, size_t s1len, const char *s2, size_t s2len);
static int dbg_strIsAllCaps(const char* s, size_t slen);
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
