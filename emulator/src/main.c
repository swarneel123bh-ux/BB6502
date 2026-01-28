#include "include/helpers.h"
#include "include/shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definitions required for the fake6502 emulator
uint8_t read6502(uint16_t address) { return MEM6502[address]; }
void write6502(uint16_t address, uint8_t value) { MEM6502[address] = value; }
extern void runDebugger();

// Main program entry point
int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stdout, "Usage: %s <BINFILE>\n", argv[0]);
    return 0;
  }

  if (!(argc >= 3)) {
    for (int i = 0; i < argc - 1; i++) {
      if (strcmp(argv[i], "-d") == 0) {
        FILE *f = fopen(argv[i + 1], "rb");
        int result = readDbgSyms(f);
        if (result < 0) {
          fprintf(stderr, "Failed to open file %s\n", argv[i + 1]);
        }
      }
    }
  }

  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    fprintf(stderr, "Failed to open file %s: ", argv[1]);
    perror("fopen:");
    return 1;
  }

  // Allocate memory for RAM and load the binary file
  MEM6502 = (uint8_t *)calloc(0x10000, sizeof(uint8_t));
  if (!MEM6502) {
    fprintf(stderr, "Failed to allocate memory for 6502! Crashing...\n");
    return 1;
  }

  uint8_t tempbyte = 0x00;
  uint16_t tempaddr = 0x0000;
  int bytesRead = 0;
  while (fread(&tempbyte, sizeof(uint8_t), 1, f)) {
    write6502(tempaddr, tempbyte);
    tempaddr++;
    bytesRead++;
  }
  printf("Read %d bytes\n", bytesRead);
  fclose(f);

  runDebugger();

  free(MEM6502);
  return 0;
}
