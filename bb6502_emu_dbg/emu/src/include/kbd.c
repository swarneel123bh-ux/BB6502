#include "kbd.h"
#include "fake6502.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

static pthread_t workerThread;
static uint16_t kbdDataRegAddr;
static struct termios orig_termios;

static void enableRawMode(void) {
  tcgetattr(STDIN_FILENO, &orig_termios);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void disableRawMode(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void kbdDataWrite(uint8_t k) {
  write6502(kbdDataRegAddr, k);
  irqPending = 1; // request IRQ safely; CPU delivers it between instructions
}

void kbdInit(void) {
  kbdDataRegAddr = (uint16_t)read6502(EMU_KBD_DATA_REG) |
                   ((uint16_t)read6502(EMU_KBD_DATA_REG + 1) << 8);
  enableRawMode();
  pthread_create(&workerThread, NULL, kbdWorker, NULL);
  pthread_detach(workerThread);
}

void *kbdWorker(void *args) {
  (void)args;

  while (running) {
    char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n > 0) {
      if ((uint8_t)c == 27) { // ESC or escape sequence
        char seq[2];
        ssize_t n1 = read(STDIN_FILENO, &seq[0], 1);
        ssize_t n2 = read(STDIN_FILENO, &seq[1], 1);
        if (n1 == 0) { // bare ESC → shutdown
          running = 0;
          break;
        }
        kbdDataWrite(27);
        if (n1 > 0)
          kbdDataWrite((uint8_t)seq[0]);
        if (n2 > 0)
          kbdDataWrite((uint8_t)seq[1]);
        continue;
      }
      kbdDataWrite((uint8_t)c);
      // irq6502();
    }
  }

  disableRawMode();
  return NULL;
}
