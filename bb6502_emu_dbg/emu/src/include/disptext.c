#include "disptext.h"
#include "fake6502.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

static pthread_t workerThread;
uint16_t disptextDataRegAddr = 0; // loaded from device table at init

void disptextInit(void) {
  // Device table entry at EMU_DISPTEXT_BASE ($FF08-$FF09):
  //   2-byte LE address of the disptext DATA register in RAM
  disptextDataRegAddr = (uint16_t)read6502(EMU_DISPTEXT_BASE) |
                        ((uint16_t)read6502(EMU_DISPTEXT_BASE + 1) << 8);

  pthread_create(&workerThread, NULL, &disptextWorker, NULL);
  pthread_detach(workerThread);
}

// Protocol (matches test.s):
//   CPU writes non-zero char to DATA reg
//   Worker sees non-zero → reads char, outputs it, writes 0 back ("consumed")
//   CPU polls DATA reg until it reads 0 before sending the next char
//   DISPL_EXIT (0xFF) causes the worker to exit cleanly
void *disptextWorker(void *args) {
  (void)args;

  while (1) {
    // Wait until the CPU deposits a non-zero byte
    pthread_mutex_lock(&disptextLock);
    while (mem6502[disptextDataRegAddr] == 0) {
      pthread_cond_wait(&disptextCond, &disptextLock);
    }
    uint8_t data = mem6502[disptextDataRegAddr];
    pthread_mutex_unlock(&disptextLock);

    if (data == DISPL_EXIT) {
      break;
    }

    putc(data, stdout);
    fflush(stdout);

    // Write 0 back: signals to the CPU that the register is free
    write6502(disptextDataRegAddr, 0);
  }

  return NULL;
}
