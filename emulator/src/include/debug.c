#include "shared.h"
#include "helpers.h"
#include "fake6502.h"
#include "disasm.h"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>


bool progExit = false;
bool dbgRunning;
bool currentlyAtBp = false;

// Send a keyhit to UART and cause interrupt
void sendToUart(uint8_t k) {
  while (read6502(ixReg & 0b00000010)) { }
  write6502(uartInReg, k);
  irq6502();
}


// Function to read from the uart when b0 ixReg flag is set
uint8_t readFromUart() {
  uint8_t r = read6502(uartOutReg);
  write6502(ixReg, read6502(ixReg) & 0xFE); // Clear b0 to allow further put_c calls
  return r;
}


// Controller for the run6502 thread when running in continuous mode
int runContinuous() {
  setupTerminal();

  runmode = RUNNING;
  int atBreakpoint = -1;

  while (runmode == RUNNING) {
    if (read6502(ixReg) & 0b10000000) {
      progExit = true;
      printf("\n  Program exited\n");
      runmode = STEPPING;
      break;
    }
    if (read6502(ixReg) & 0x40) break;
    if (read6502(ixReg) & 0x01) { putchar(readFromUart()); }
    int k = getKeyAsync();
    if (k != -1) sendToUart(k);

    int instrlen;
    char line[64];
    memset(line, 0, sizeof(line));
    instrlen = disassemble_6502(pc, read6502, line);
    if (currentlyAtBp) { step6502(); continue; }

    bool brkFound = false;

    // Check if a breakpoint lies between the bytes of the current instr
    for (int i = 0; (i < nBreakpoints) && !brkFound; i ++) {
      if (bpList[i].address >= pc && bpList[i].address <= pc + instrlen){
        // printf("  Breakpoint!\n");
        atBreakpoint = i;
        brkFound = true;
      }
    }

    if (brkFound) {
      runmode = STEPPING;
      break;
    }

    step6502();
  }

  restoreTerminal();
  return atBreakpoint;
}


// Run the debugger
void runDebugger() {
  uartInReg =   read6502(UARTINREG_ADDR)  | read6502(UARTINREG_ADDR + 1)  << 8;
  uartOutReg =  read6502(UARTOUTREG_ADDR) | read6502(UARTOUTREG_ADDR + 1) << 8;
  ixReg =       read6502(IXFLAGREG_ADDR)  | read6502(IXFLAGREG_ADDR + 1)  << 8;

  // Sanity check before starting
  printf("uartInReg: %04hx\n", uartInReg);
  printf("uartOutReg: %04hx\n", uartOutReg);
  printf("ixReg: %04hx\n", ixReg);

  uint16_t startAddr, endAddr;
  char buf[32];
  bool exited_ = false;

  reset6502();
  dbgRunning = true;
  while ( dbgRunning ) {

    // Check if program exited last step
    if (exited_) {
      printf("  Restart/Exit? [r/E] :");
      int c = getCharacter();
      if (c == 'r') {
        reset6502();
        write6502(ixReg, 0x00);
        exited_ = false;
      } else {
        dbgRunning = false;
      }
    }

    printf(" [bcdhlmrsq]>>");
    if (getLine(buf, sizeof(buf)) == 0) {
      fprintf(stderr, "getLine: input failure!\n");
      exit(1);
    }

    char* tok = strtok(buf, " \t\n");
    if (!tok) {
      fprintf(stderr, "strtok: cmd parsing failure!\n");
      exit(1);
    }
    int c = tok[0];


    switch(c) {
      // Add breakpoint
      case 'b': {
        tok = strtok(NULL, " \t\n");
        if (!tok) {
          printf("  Setting new breakpoint at 0x%04hx\n", pc);
          setBreakpoint(pc);  // If no addr/symbol given set at current addr
          break;
        }

        // Set at given addr (CHECK IF ADDR/SYMBOL FIRST)
        char* end;
        uint16_t bp = strtoul(tok, &end, 0);
        printf("  Setting new breakpoint at 0x%04hx\n", bp);
        setBreakpoint(bp);
      } break;

      // Run in continuous mode
      case 'c': {
        int brkpt = runContinuous();
        runmode = STEPPING;
        printf("  Control returned to debugger\n");

        // Check if exited
        if (progExit) {
          printf("  Restart/Exit? [r/E] :");
          int c = getCharacter();
          if (c == 'r') {
            reset6502();
            write6502(ixReg, 0x00);
          } else {
            dbgRunning = false;
          }
          continue;
        }

        if (brkpt >= 0) {
          char buf[32];
          memset(buf, 0, sizeof(buf));
          disassemble_6502(pc, read6502, buf);
          printf("  Breakpoint [%d] at addr %04hx: %s\n", brkpt, bpList[brkpt].address, buf);
          currentlyAtBp = true;
        }

      } break;


      // Decode current instruction
      case 'd': {
        int nofLines = 5;

        tok = strtok(NULL, " \t\n");
        if (!tok) { nofLines = 5; }
        else { nofLines = atoi(tok); }

        uint16_t pc_ = pc;
        int instrlen;

        for (int i = 0; i < nofLines; i ++) {
          char line[64];
          memset(line, 0, sizeof(line));
          instrlen = disassemble_6502(pc_, read6502, line);
          printf("\t%s\n", line);
          pc_ += (uint16_t)instrlen;
        }
      } break;


      // Display the help contents
      case 'h': {
        printf("Help : -\n");
        printf("b [./addr/symbol]: Create a breakpoint at [curr instr/addr/symbol]\n");
        printf("c: Continue the program from current state\n");
        printf("d [n]: Disassemble next 5/[n] instructions\n");
        printf("h: Display this help\n");
        printf("l: List all breakpoints\n");
        printf("m [start] [end]: Display memory contents from a start location to an end location\n");
        printf("r: Display the register contents\n");
        printf("s [n]: Step the program with one/[n] instruction\n");
        printf("\n");
      } break;

      // List all breakpoints
      case 'l':
      if (nBreakpoints <= 0) { printf("		No breakpoints set\n"); break; }
      for (int i = 0; i < nBreakpoints; i ++) {
        char line[64];
        memset(line, 0, sizeof(line));
        int instrlen = disassemble_6502(bpList[i].address, read6502, line);
        printf("%04hx\t%s\n", bpList[i].address, line);
      }
      break;


      // Display the 6502 memory contents in a given range
      case 'm': {
        bool getStartAddr = false, getStopAddr = false;

        // Get start addr
        tok = strtok(NULL, " \t\n");

        // NULL => Both addrs not given
        if (!tok) {
          printf("Give the starting address in hex [0x...]: ");
          startAddr = read_hex_u16();
          printf("Give the ending address in hex [0x...]: ");
          endAddr = read_hex_u16();
          showmem(startAddr, endAddr);
          break;
        }

        // Start addr given
        char* end;
        startAddr = strtoul(tok, &end, 0);
        if (startAddr < 0x0000 || startAddr > 0xFFFF) {
          printf("Invalid input %04hx\n", startAddr);
          break;
        }

        // Get endAddr
        tok = strtok(NULL, " \t\n");

        // NULL => Start addr given but stop addr not given
        if (!tok) {
          printf("Give the ending address in hex [0x...]: ");
          endAddr = read_hex_u16();
          showmem(startAddr, endAddr);
          break;
        }

        // Both addrs given
        char* end_;
        endAddr = strtoul(tok, &end_, 0);
        if (endAddr < startAddr | endAddr > 0xFFFF) {
          printf("Invalid input %04hx\n", endAddr);
          break;
        }
        showmem(startAddr, endAddr);
      } break;


      // Show the 6502 Register contents and the UART register contents
      case 'r':
      printf("PC=%04hx SP=%04hx A=%04hx X=%04hx Y=%04hx status=%04hx\n", pc, sp, a, x, y, status);
      break;

      // Step the 6502 with the current [n] instruction
        case 's': {
        int nsteps;

        tok = strtok(NULL, " \t\n");
        if (!tok) { nsteps = 1; }
        else { nsteps = atoi(tok); }

        for (int i = 0; (i < nsteps) && !exited_; i ++) {
          int instrlen;
          char line[64];
          memset(line, 0, sizeof(line));
          instrlen = disassemble_6502(pc, read6502, line);
          bool brkptFound = false;

          if (currentlyAtBp) {
            printf("  Stepping through breakpoint...\n");
            currentlyAtBp = false;
            step6502();
            continue;
          }

          // Check if a breakpoint lies between the bytes of the current instr
          // Need to optimize breakpoint search
          for (int i = 0; i < nBreakpoints && !brkptFound; i ++) {
            // Breakpoint reached, dont step the instr yet
            if (bpList[i].address >= pc && bpList[i].address <= pc + instrlen){ brkptFound = true; }

            if (brkptFound) {
              printf("  Breakpoint [%d] at addr %04hx: %s\n", i, bpList[i].address, line);
              currentlyAtBp = true;
              break;
            }

            // No breakpoint, current instruction can be stepped
            printf("\t%s\n", line);
            step6502();
            if (read6502(ixReg) & 0b10000000) {
              progExit = true;
              printf("  Program exited\n");
              exited_ = true;
              break;
            }
            if (read6502(ixReg) & 0x40) break;
            if (read6502(ixReg) & 0x01) { putchar(readFromUart()); }

          }

        } } break;

      // Exit the debugger
      case 'q':
      dbgRunning = false;
      break;

      default:
      break;
    }
  }
}
