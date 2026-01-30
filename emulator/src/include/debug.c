#include "debug.h"
#include "disasm.h"
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

  // Sanity check before starting
  printf("uartInReg: %04hx\n", uartInReg);
  printf("uartOutReg: %04hx\n", uartOutReg);
  printf("ixReg: %04hx\n", ixReg);

  dbgRunning = true;
  currentlyAtBp = false;

  reset6502();
  while (dbgRunning) {

    getCmd(buf);
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
  printf("Help : -\n");
  printf(
      "b [./addr/symbol]: Create a breakpoint at [curr instr/addr/symbol]\n");
  printf("c: Continue the program from current state\n");
  printf("d [n]: Disassemble next 5/[n] instructions\n");
  printf("h: Display this help\n");
  printf("l: List all breakpoints\n");
  printf("m [start] [end]: Display memory contents from a start location to an "
         "end location\n");
  printf("r: Display the register contents\n");
  printf("s [n]: Step the program with one/[n] instruction\n");
  printf("\n");
  return;
}

void disassembleInstrs(char *cmdtoks[], size_t cmdtoksiz) {
  int nofLines = 5;
  if (cmdtoksiz < 2) {
    nofLines = 5;
  } else if (!strToInt(cmdtoks[1], &nofLines)) {
    fprintf(stderr, "cmd parsing err... %s %s\n", cmdtoks[0], cmdtoks[1]);
    return;
  }

  uint16_t pc_ = pc;
  int instrlen;

  for (int i = 0; i < nofLines; i++) {
    char line[64];
    memset(line, 0, sizeof(line));
    instrlen = disassemble_6502(pc_, read6502, line);
    printf("\t%04hx\t%s\n", pc_, line);
    pc_ += (uint16_t)instrlen;
  }
}

void addNewBreakpoint(char *cmdtoks[], size_t cmdtoksiz) {

  if (cmdtoksiz < 2) {
    printf("  Setting new breakpoint at 0x%04hx\n", pc);
    setBreakpoint(pc); // If no addr/symbol given set at current addr
    return;
  }

  char *end;
  uint16_t bp = strtoul(cmdtoks[1], &end, 0);
  printf("  Setting new breakpoint at 0x%04hx\n", bp);
  setBreakpoint(bp);
  return;
}

void listAllBreakpoints() {

  if (nBreakpoints <= 0) {
    printf("		No breakpoints set\n");
    return;
  }

  for (int i = 0; i < nBreakpoints; i++) {
    char line[64];
    memset(line, 0, sizeof(line));
    int instrlen = disassemble_6502(bpList[i].address, read6502, line);
    printf("%04hx\t%s\n", bpList[i].address, line);
  }

  return;
}

void printMemRange(char *cmdtoks[], size_t cmdtoklen) {

  uint16_t startAddr, endAddr;

  // Both addrs not given
  if (cmdtoklen < 2) {
    printf("Give the starting address in hex [0x...]: ");
    startAddr = read_hex_u16();
    printf("Give the ending address in hex [0x...]: ");
    endAddr = read_hex_u16();
    if (endAddr < startAddr | endAddr > 0xFFFF) {
      printf("Invalid input %04hx\n", endAddr);
      return;
    }
    showmem(startAddr, endAddr);
    return;
  }

  // Start addr given but stop addr not given
  if (cmdtoklen < 3) {
    printf("Give the ending address in hex [0x...]: ");
    endAddr = read_hex_u16();
    showmem(startAddr, endAddr);
    if (endAddr < startAddr | endAddr > 0xFFFF) {
      printf("Invalid input %04hx\n", endAddr);
      return;
    }
    return;
  }

  // Both addrs given
  char *end_;
  startAddr = strtoul(cmdtoks[1], &end_, 0);
  endAddr = strtoul(cmdtoks[2], &end_, 0);
  if (endAddr < startAddr | endAddr > 0xFFFF) {
    printf("Invalid input %04hx\n", endAddr);
    return;
  }
  showmem(startAddr, endAddr);

  return;
}

void printRegisters() {
  printf("PC=%04hx SP=%04hx A=%04hx X=%04hx Y=%04hx status=%04hx\n", pc, sp, a,
         x, y, status);
}

int performChecks() {
  if (read6502(ixReg) & 0b10000000) {
    return SIG_PROGRAM_EXITED;
  }

  if (read6502(ixReg) & 0x40) {
    return SIG_CONTROL_RETURNED;
  }

  if (read6502(ixReg) & 0x01) {
    putchar(readFromUart());
    return SIG_NOSIG;
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
int runContinuous(int *signal) {
  setupTerminal();

  int atBreakpoint = -1;
  while (1) {
    int sig = performChecks();
    if (sig == SIG_PROGRAM_EXITED || sig == SIG_CONTROL_RETURNED) {
      *signal = sig;
      break;
    }

    int k = getKeyAsync();
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

  restoreTerminal();
  return atBreakpoint;
}

void runDebuggerContinuous() {
  int signal = SIG_NOSIG;
  int brkpt = runContinuous(&signal);
  printf("\n  Control returned to debugger\n");
  if (signal == SIG_PROGRAM_EXITED) {
    printf("\n  Program exited. Restart/Exit? [r/E] :");
    int c = getCharacter();
    if (c == 'r') {
      reset6502();
      write6502(ixReg, 0x00);
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
    printf("  Breakpoint [%d] at addr %04hx: %s\n", brkpt,
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
      fprintf(stderr, "cmd parsing err... %s %s\n", cmdtoks[0], cmdtoks[1]);
      return;
    }
  }

  for (int i = 0; i < nsteps; i++) {
    int instrlen;
    char line[64];
    memset(line, 0, sizeof(line));
    instrlen = disassemble_6502(pc, read6502, line);

    if (currentlyAtBp) {
      printf("  Stepping through breakpoint...\n");
      printf("\t%04hx\t%s\n", pc, line);
      step6502();
      currentlyAtBp = false;
      performChecks();
      continue;
    }

    int bpNum = -1;
    if (checkIfAtBreakpoint(pc, instrlen, &bpNum)) {
      printf("  At Breakpoint [%d]: %04hx   %s\n", bpNum, pc, line);
      currentlyAtBp = true;
      return;
    }

    printf("\t%04hx\t%s\n", pc, line);
    step6502();
    performChecks();
    continue;
  }
}
