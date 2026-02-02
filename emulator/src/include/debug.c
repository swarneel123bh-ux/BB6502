#include "debug.h"
#include "disasm.h"
#include "display.h"
#include "fake6502.h"
#include "helpers.h"
#include "shared.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void runDebugger() {
  uartInReg = read6502(UARTINREG_ADDR) | read6502(UARTINREG_ADDR + 1) << 8;
  uartOutReg = read6502(UARTOUTREG_ADDR) | read6502(UARTOUTREG_ADDR + 1) << 8;
  ixReg = read6502(IXFLAGREG_ADDR) | read6502(IXFLAGREG_ADDR + 1) << 8;

  DISPLAY_INITDISPLAY();

  // Sanity check before starting
  DISPLAY_CONSOLE_ECHO("uartInReg: %04hx\n", uartInReg);
  DISPLAY_CONSOLE_ECHO("uartOutReg: %04hx\n", uartOutReg);
  DISPLAY_CONSOLE_ECHO("ixReg: %04hx\n", ixReg);

  dbgRunning = true;
  currentlyAtBp = false;

  reset6502();
  while (dbgRunning) {

    DISPLAY_CONSOLE_GETCMD(buf);
    if (buf[0] == '\n' || strlen(buf) == 0)
      continue;
    size_t cmdTokArrSiz = 10;
    char *cmdtoks[cmdTokArrSiz];
    int cmdtoklen = getCmdTokens(buf, cmdtoks, cmdTokArrSiz);

    switch (parseCommand(cmdtoks[0])) {
    case CMD_REMOVEBP:
      break;

    case CMD_BREAKPOINT:
      addNewBreakpoint(cmdtoks, cmdtoklen);
      break;

    case CMD_CONTINUE:
      runDebuggerContinuous();
      break;

    case CMD_DISASSEMBLE:
      disassembleInstrs(cmdtoks, cmdtoklen);
      break;

    case CMD_HELP:
      displayHelp();
      break;

    case CMD_LISTBREAKPOINTS:
      listAllBreakpoints();
      break;

    case CMD_MEMORY:
      printMemRange(cmdtoks, cmdtoklen);
      break;

    case CMD_REGISTERS:
      printRegisters();
      break;

    case CMD_STEP:
      performStep(cmdtoks, cmdtoklen);
      break;

    case CMD_QUIT:
      dbgRunning = false;
      break;

    default:
      break;
    }
  }

  endwin();
}

void sendToUart(uint8_t k) {
  while (read6502(ixReg & 0b00000010)) {
  }
  write6502(uartInReg, k);
  irq6502();
}

uint8_t readFromUart() {
  uint8_t r = read6502(uartOutReg);
  write6502(ixReg,
            read6502(ixReg) & 0xFE); // Clear b0 to allow further put_c calls
  return r;
}

void displayHelp() {
  DISPLAY_CONSOLE_ECHO("Help : -\n");
  DISPLAY_CONSOLE_ECHO(
      "b [./addr/symbol]: Create a breakpoint at [curr instr/addr/symbol]\n");
  DISPLAY_CONSOLE_ECHO("c: Continue the program from current state\n");
  DISPLAY_CONSOLE_ECHO("d [n]: Disassemble next 5/[n] instructions\n");
  DISPLAY_CONSOLE_ECHO("h: Display this help\n");
  DISPLAY_CONSOLE_ECHO("l: List all breakpoints\n");
  DISPLAY_CONSOLE_ECHO(
      "m [start] [end]: Display memory contents from a start location to an "
      "end location\n");
  DISPLAY_CONSOLE_ECHO("r: Display the register contents\n");
  DISPLAY_CONSOLE_ECHO("s [n]: Step the program with one/[n] instruction\n");
  DISPLAY_CONSOLE_ECHO("\n");
  return;
}

void disassembleInstrs(char *cmdtoks[], size_t cmdtoksiz) {
  int nofLines = 5;
  if (cmdtoksiz < 2) {
    nofLines = 5;
  } else if (!strToInt(cmdtoks[1], &nofLines)) {
    DISPLAY_CONSOLE_ECHO("cmd parsing err... %s %s\n", cmdtoks[0], cmdtoks[1]);
    return;
  }

  uint16_t pc_ = pc;
  int instrlen;

  for (int i = 0; i < nofLines; i++) {
    char line[64];
    memset(line, 0, sizeof(line));
    instrlen = disassemble_6502(pc_, read6502, line);
    DISPLAY_CONSOLE_ECHO("\t%04hx\t%s\n", pc_, line);
    pc_ += (uint16_t)instrlen;
  }
}

void addNewBreakpoint(char *cmdtoks[], size_t cmdtoksiz) {

  if (cmdtoksiz < 2) {
    DISPLAY_CONSOLE_ECHO("  Setting new breakpoint at 0x%04hx\n", pc);
    setBreakpoint(pc); // If no addr/symbol given set at current addr
    return;
  }

  char *end;
  uint16_t bp = strtoul(cmdtoks[1], &end, 0);
  DISPLAY_CONSOLE_ECHO("  Setting new breakpoint at 0x%04hx\n", bp);
  setBreakpoint(bp);
  return;
}

void listAllBreakpoints() {

  if (nBreakpoints <= 0) {
    DISPLAY_CONSOLE_ECHO("		No breakpoints set\n");
    return;
  }

  for (int i = 0; i < nBreakpoints; i++) {
    char line[64];
    memset(line, 0, sizeof(line));
    int instrlen = disassemble_6502(bpList[i].address, read6502, line);
    DISPLAY_CONSOLE_ECHO("%04hx\t%s\n", bpList[i].address, line);
  }

  return;
}

void printMemRange(char *cmdtoks[], size_t cmdtoklen) {

  uint16_t startAddr, endAddr;

  // Both addrs not given
  if (cmdtoklen < 2) {
    DISPLAY_CONSOLE_ECHO("Give the starting address in hex [0x...]: ");
    startAddr = read_hex_u16();
    DISPLAY_CONSOLE_ECHO("Give the ending address in hex [0x...]: ");
    endAddr = read_hex_u16();
    if (endAddr < startAddr | endAddr > 0xFFFF) {
      DISPLAY_CONSOLE_ECHO("Invalid input %04hx\n", endAddr);
      return;
    }
    showmem(startAddr, endAddr);
    return;
  }

  // Start addr given but stop addr not given
  if (cmdtoklen < 3) {
    DISPLAY_CONSOLE_ECHO("Give the ending address in hex [0x...]: ");
    endAddr = read_hex_u16();
    showmem(startAddr, endAddr);
    if (endAddr < startAddr | endAddr > 0xFFFF) {
      DISPLAY_CONSOLE_ECHO("Invalid input %04hx\n", endAddr);
      return;
    }
    return;
  }

  // Both addrs given
  char *end_;
  startAddr = strtoul(cmdtoks[1], &end_, 0);
  endAddr = strtoul(cmdtoks[2], &end_, 0);
  if (endAddr < startAddr | endAddr > 0xFFFF) {
    DISPLAY_CONSOLE_ECHO("Invalid input %04hx\n", endAddr);
    return;
  }
  showmem(startAddr, endAddr);

  return;
}

void printRegisters() {
  DISPLAY_CONSOLE_ECHO(
      "PC=%04hx SP=%04hx A=%04hx X=%04hx Y=%04hx status=%04hx\n", pc, sp, a, x,
      y, status);
}

int performChecks() {
  if (read6502(ixReg) & 0b10000000) {
    return SIG_PROGRAM_EXITED;
  }

  if (read6502(ixReg) & 0x40) {
    return SIG_CONTROL_RETURNED;
  }

  if (read6502(ixReg) & 0x01) {
    DISPLAY_TERMINAL_PUTCHAR(readFromUart());
    return SIG_TEXTOUT;
  }

  if (read6502(ixReg) & 0x20) {
    // putcharVGA(readFromUart(), terminal);
    return SIG_VGAOUT;
  }

  return SIG_NOSIG;
}

bool checkIfAtBreakpoint(uint16_t pc, int instrlen, int *bpNum) {

  uint16_t instrStartAddr = pc;
  uint16_t instrEndAddr = pc + instrlen;

  for (int i = 0; i < nBreakpoints; i++) {
    if ((instrStartAddr <= bpList[i].address) &&
        (bpList[i].address < instrEndAddr)) {
      *bpNum = i;
      return true;
    }
  }
  *bpNum = -1;
  return false;
}

// NEED TO PRETTIFIY IT
int runContinuous(int *signal_) {
  signal(SIGINT, sigintHandlerTerminal);

  int atBreakpoint = -1;
  while (1) {
    int sig = performChecks();
    if (sig == SIG_PROGRAM_EXITED || sig == SIG_CONTROL_RETURNED) {
      *signal_ = sig;
      break;
    }

    int k = DISPLAY_TERMINAL_GETCHAR();
    if (k != -1)
      sendToUart(k);

    int instrlen;
    char line[64];
    memset(line, 0, sizeof(line));
    instrlen = disassemble_6502(pc, read6502, line);
    if (currentlyAtBp) {
      step6502();
      continue;
    }

    int bpnum;
    if (checkIfAtBreakpoint(pc, instrlen, &bpnum)) {
      atBreakpoint = bpnum;
      break;
    }

    step6502();
  }

  signal(SIGINT, sigintHandlerConsole);
  return atBreakpoint;
}

void runDebuggerContinuous() {
  noecho();
  int signal = SIG_NOSIG;
  int brkpt = runContinuous(&signal);
  echo();

  DISPLAY_CONSOLE_ECHO("\n  Control returned to debugger\n");
  if (signal == SIG_PROGRAM_EXITED) {
    DISPLAY_CONSOLE_ECHO("\n  Program exited. Restart/Exit? [r/E] :");
    int c = DISPLAY_CONSOLE_GETCHAR();
    if (c == 'r') {
      reset6502();
      write6502(ixReg, 0x00);
      DISPLAY_CONSOLE_ECHO("\n");
      dbgRunning = true;
      currentlyAtBp = false;
    } else {
      dbgRunning = false;
      return;
    }
  }

  if (brkpt >= 0) {
    char buf[32];
    memset(buf, 0, sizeof(buf));
    int instrlen = disassemble_6502(pc, read6502, buf);
    DISPLAY_CONSOLE_ECHO("  Breakpoint [%d] at addr %04hx: %s\n", brkpt,
                         bpList[brkpt].address, buf);
    currentlyAtBp = true;
  }

  return;
}

void performStep(char *cmdtoks[], size_t cmdtoksize) {
  int nsteps = 1;

  if (cmdtoksize < 2) {
    nsteps = 1;
  } else {
    if (!strToInt(cmdtoks[1], &nsteps)) {
      DISPLAY_CONSOLE_ECHO("cmd parsing err... %s %s\n", cmdtoks[0],
                           cmdtoks[1]);
      return;
    }
  }

  for (int i = 0; i < nsteps; i++) {
    int instrlen;
    char line[64];
    memset(line, 0, sizeof(line));
    instrlen = disassemble_6502(pc, read6502, line);

    if (currentlyAtBp) {
      DISPLAY_CONSOLE_ECHO("  Stepping through breakpoint...\n");
      DISPLAY_CONSOLE_ECHO("\t%04hx\t%s\n", pc, line);
      step6502();
      currentlyAtBp = false;
      performChecks();
      continue;
    }

    int bpNum = -1;
    if (checkIfAtBreakpoint(pc, instrlen, &bpNum)) {
      DISPLAY_CONSOLE_ECHO("  At Breakpoint [%d]: %04hx   %s\n", bpNum, pc,
                           line);
      currentlyAtBp = true;
      return;
    }

    DISPLAY_CONSOLE_ECHO("\t%04hx\t%s\n", pc, line);
    step6502();
    performChecks();
    continue;
  }
}
