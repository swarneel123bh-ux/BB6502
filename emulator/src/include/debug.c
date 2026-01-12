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
#include <pthread.h>
#include <stdbool.h>

bool progExit = false;
bool dbgRunning;

// Executes the 6502 processor step by step without stopping for anything
// This aims to replicate real processor behaviour to the bone
// Any control needs to be done from the debugger side
void* run6502(void* runmode) {
  return NULL;
}


// Function to trigger an interrupt because of a keypress
// Sends a byte to uart and calls irq
// The emulator will respond to a get_c request when b3 of ixReg is set, the irq must clear the bit
void sendToUart(uint8_t k) {
  while (read6502(ixReg & 0b00000010)) { printf(" Wait..."); }
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
void runContinuous() {
  pthread_t thrd;
  int thrdStatus = pthread_create(&thrd, NULL, run6502, NULL);
  
  setupTerminal();
  while (1) {
    // Read the ixReg and find out if any interfacing action needs to be taken
    // Check if the 6502 has requested exit
    if (read6502(ixReg) & 0b10000000) { progExit = true; printf("  Program exited\n"); break; }

    // Check if the 6502 wants to drop back to debugger
    if (read6502(ixReg) & 0x40) break;
    
    // Check if th3 6502 wants to print a character 
    if (read6502(ixReg) & 0x01) { putchar(readFromUart()); }

    int k = getKeyAsync();
    if (k != -1) sendToUart(k);

    step6502();
  }

  restoreTerminal();
  return;
}


// Main function of the debugger
// Spawns an emulator thread and communicates with the user
void runDebugger() {
  // Read the important addresses to get the register addrs
  uartInReg =   read6502(UARTINREG_ADDR)  | read6502(UARTINREG_ADDR + 1)  << 8;
  uartOutReg =  read6502(UARTOUTREG_ADDR) | read6502(UARTOUTREG_ADDR + 1) << 8;
  ixReg =       read6502(IXFLAGREG_ADDR)  | read6502(IXFLAGREG_ADDR + 1)  << 8;

  // Sanity check before starting
  printf("uartInReg: %04hx\n", uartInReg);
  printf("uartOutReg: %04hx\n", uartOutReg);
  printf("ixReg: %04hx\n", ixReg);

  pthread_t emu;    // The runner thread
  int status; 
  uint16_t startAddr, endAddr;
  char buf[32];


  reset6502();
  dbgRunning = true;
  while ( dbgRunning ) {  // Main debugger loop
    
    printf(" [chmrsq]>>");
    int c = getCharacter();

    switch(c) {
      case 'c':
      runContinuous();
      printf("  Control returned to debugger\n");
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
      break;

      // Display the help contents
      case 'h':
      printf("Help : -\n");
      printf("c: Continue the program from current state\n");
      printf("h: Display this help\n");
      printf("m: Display memory contents from a start location to an end location\n");
      printf("r: Display the register contents\n");
      printf("s: Step the program with one instruction\n");
      printf("\n");
      break;

      // Display the 6502 memory contents in a given range
      case 'm':
      printf("Give the starting address in hex: ");
      startAddr = read_hex_u16();
      printf("Give the ending address in hex: ");
      endAddr = read_hex_u16();
      showmem(startAddr, endAddr);
      break;

      // Show the 6502 Register contents and the UART register contents
      case 'r':
      printf("PC=%04hx SP=%04hx A=%04hx X=%04hx Y=%04hx status=%04hx\n", pc, sp, a, x, y, status);
      break;

      // Step the 6502 with the current instruction
      // Only when in stepping mode ofc
      case 's':
      step6502();
      // Check stuff
      // Read the ixReg and find out if any interfacing action needs to be taken
      // Check if the 6502 has requested exit
      if (read6502(ixReg) & 0b10000000) { progExit = true; printf("  Program exited\n"); break; }
      // Check if the 6502 wants to drop back to debugger
      if (read6502(ixReg) & 0x40) break;
      // Check if th3 6502 wants to print a character 
      if (read6502(ixReg) & 0x01) { putchar(readFromUart()); }
      break;

        // Exit the debugger
      case 'q':
      dbgRunning = false;
      break;

      default:
      break;
    }
  }
}
