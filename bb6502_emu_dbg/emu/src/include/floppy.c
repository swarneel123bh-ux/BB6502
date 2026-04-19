#include "floppy.h"
#include "fake6502.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static void floppyReadSector(void);
static void floppyWriteSector(void);
static void floppyDelayMs(int milliseconds);
static void floppySimulateDelayAndUpdateCHS(uint8_t targetCylinder,
                                            uint8_t targetSector);

uint16_t floppyCmdRegAddr = 0;
uint16_t floppyStatusRegAddr = 0;
uint16_t floppyDataRegAddr = 0;
char flpFileName[FILENAME_MAX] = {0};

static FILE *flpFile;
uint8_t *flpBuffer;
floppy_t floppy;
static pthread_t workerThread;

void floppyInit(void) {
  // Read the 3 device-table entries (each is a 2-byte LE pointer)
  floppyStatusRegAddr = (uint16_t)read6502(EMU_FLOPPY_STATUS_REG) |
                        ((uint16_t)read6502(EMU_FLOPPY_STATUS_REG + 1) << 8);
  floppyCmdRegAddr = (uint16_t)read6502(EMU_FLOPPY_CMD_REG) |
                     ((uint16_t)read6502(EMU_FLOPPY_CMD_REG + 1) << 8);
  floppyDataRegAddr = (uint16_t)read6502(EMU_FLOPPY_DATA_REG) |
                      ((uint16_t)read6502(EMU_FLOPPY_DATA_REG + 1) << 8);

  flpBuffer = (uint8_t *)malloc(FLOPPY_TOTAL_CAPACITY);
  if (!flpBuffer) {
    fprintf(stderr, "[FATAL] Couldn't allocate floppy buffer: %s\n",
            strerror(errno));
    exit(1);
  }

  flpFile = fopen(flpFileName, "rb");
  if (!flpFile) {
    fprintf(stderr, "[WARN] No floppy image found, starting blank\n");
    memset(flpBuffer, 0, FLOPPY_TOTAL_CAPACITY);
  } else {
    size_t r = fread(flpBuffer, 1, FLOPPY_TOTAL_CAPACITY, flpFile);
    fclose(flpFile);
    if (r != FLOPPY_TOTAL_CAPACITY) {
      fprintf(stderr,
              "[FATAL] Invalid floppy image (expected %u bytes, got %zu)\n",
              FLOPPY_TOTAL_CAPACITY, r);
      free(flpBuffer);
      exit(1);
    }
  }

  pthread_create(&workerThread, NULL, &floppyWorker, NULL);
  pthread_detach(workerThread);

  // The 6502 must see IDLE before it can issue any command.
  // calloc zeroed the register; set it explicitly so floppy_wait_idle
  // doesn't spin forever on the very first call.
  mem6502[floppyStatusRegAddr] = FLOPPY_STATUS_IDLE;
}

void *floppyWorker(void *args) {
  (void)args;

  while (running) {
    // Sleep until the CPU writes a non-zero command
    pthread_mutex_lock(&floppyLock);
    while ((mem6502[floppyCmdRegAddr] == FLOPPY_CMD_NO_CMD) && running) {
      pthread_cond_wait(&floppyCond, &floppyLock);
    }
    pthread_mutex_unlock(&floppyLock);

    if (!running)
      break;

    // Only service commands when idle or in error state
    uint8_t st = read6502(floppyStatusRegAddr);
    if (!(st & FLOPPY_STATUS_IDLE) && !(st & FLOPPY_STATUS_ERROR)) {
      continue;
    }

    uint8_t cmd = read6502(floppyCmdRegAddr);
    switch (cmd) {

    case FLOPPY_CMD_RESET: {
      st = read6502(floppyStatusRegAddr);
      st &= ~(FLOPPY_STATUS_IDLE);
      st |= FLOPPY_STATUS_BUSY;
      write6502(floppyStatusRegAddr, st);
      write6502(floppyCmdRegAddr, FLOPPY_CMD_NO_CMD);
      st &= ~FLOPPY_STATUS_BUSY;
      st |= (FLOPPY_STATUS_IDLE);
      write6502(floppyStatusRegAddr, st);
      break;
    }

    case FLOPPY_CMD_SET_DMA_ADDR: {
      st = read6502(floppyStatusRegAddr);
      st &= ~(FLOPPY_STATUS_IDLE);
      st |= FLOPPY_STATUS_BUSY;
      write6502(floppyStatusRegAddr, st);
      floppy.dmaAddr = (uint16_t)read6502(floppyDataRegAddr) |
                       ((uint16_t)read6502(floppyDataRegAddr + 1) << 8);
      write6502(floppyCmdRegAddr, FLOPPY_CMD_NO_CMD);
      st &= ~FLOPPY_STATUS_BUSY;
      st |= (FLOPPY_STATUS_IDLE);
      write6502(floppyStatusRegAddr, st);
      break;
    }

    case FLOPPY_CMD_STORE_LBA: {
      st = read6502(floppyStatusRegAddr);
      st &= ~(FLOPPY_STATUS_IDLE);
      st |= FLOPPY_STATUS_BUSY;
      write6502(floppyStatusRegAddr, st);
      floppy.lba = read6502(floppyDataRegAddr);
      write6502(floppyCmdRegAddr, FLOPPY_CMD_NO_CMD);
      st &= ~FLOPPY_STATUS_BUSY;
      st |= (FLOPPY_STATUS_IDLE);
      write6502(floppyStatusRegAddr, st);
      break;
    }

    case FLOPPY_CMD_READ_SECTOR: {
      st = read6502(floppyStatusRegAddr);
      st &= ~(FLOPPY_STATUS_ERROR | FLOPPY_STATUS_IDLE | FLOPPY_STATUS_IRQ);
      st |= FLOPPY_STATUS_BUSY;
      write6502(floppyStatusRegAddr, st);
      if (floppy.lba >= FLOPPY_TOTAL_SECTORS) {
        st &= ~FLOPPY_STATUS_BUSY;
        st |= FLOPPY_STATUS_IDLE;
        st |= FLOPPY_STATUS_ERROR;
        st |= FLOPPY_STATUS_IRQ;
        write6502(floppyStatusRegAddr, st);
        write6502(floppyCmdRegAddr, FLOPPY_CMD_NO_CMD);
        // irq6502();
      } else {
        floppyReadSector();
        st &= ~FLOPPY_STATUS_BUSY;
        st &= ~FLOPPY_STATUS_ERROR;
        st |= FLOPPY_STATUS_IDLE;
        st |= FLOPPY_STATUS_IRQ;
        write6502(floppyStatusRegAddr, st);
        write6502(floppyCmdRegAddr, FLOPPY_CMD_NO_CMD);
        // irq6502();
      }
      irqPending = 1; // request IRQ safely (no data race)
      break;
    }

    case FLOPPY_CMD_WRITE_SECTOR: {
      st = read6502(floppyStatusRegAddr);
      st &= ~(FLOPPY_STATUS_ERROR | FLOPPY_STATUS_IDLE | FLOPPY_STATUS_IRQ);
      st |= FLOPPY_STATUS_BUSY;
      write6502(floppyStatusRegAddr, st);
      if (floppy.lba >= FLOPPY_TOTAL_SECTORS) {
        st &= ~FLOPPY_STATUS_BUSY;
        st |= FLOPPY_STATUS_IDLE;
        st |= FLOPPY_STATUS_ERROR;
        st |= FLOPPY_STATUS_IRQ;
        write6502(floppyStatusRegAddr, st);
        write6502(floppyCmdRegAddr, FLOPPY_CMD_NO_CMD);
        // irq6502();
      } else {
        floppyWriteSector();
        st &= ~FLOPPY_STATUS_BUSY;
        st &= ~FLOPPY_STATUS_ERROR;
        st |= FLOPPY_STATUS_IDLE;
        st |= FLOPPY_STATUS_IRQ;
        write6502(floppyStatusRegAddr, st);
        write6502(floppyCmdRegAddr, FLOPPY_CMD_NO_CMD);
        // irq6502();
      }
      irqPending = 1; // request IRQ safely (no data race)
      break;
    }

    case FLOPPY_CMD_NO_CMD:
    default:
      break;
    }
  }

  free(flpBuffer);
  return NULL;
}

static void floppySimulateDelayAndUpdateCHS(uint8_t targetCylinder,
                                            uint8_t targetSector) {
  float rotation_time = 60000.0f / FLOPPY_RPM;
  float sector_time = rotation_time / FLOPPY_SECTORS_PER_TRACK;

  uint32_t cyl_diff = (uint32_t)abs((int)targetCylinder - (int)floppy.cylinder);
  uint32_t seek_time = cyl_diff * FLOPPY_TRACK_TO_TRACK_SEEK_TIME;

  float sector_diff =
      (float)((targetSector - floppy.sector + FLOPPY_SECTORS_PER_TRACK) %
              FLOPPY_SECTORS_PER_TRACK);
  float rotation_latency = sector_diff * sector_time;
  float transfer_time =
      (sector_diff == 0.0f && cyl_diff > 0) ? 0.0f : sector_time;

  float total_wait = (float)seek_time + rotation_latency + transfer_time;
  floppy.cylinder = targetCylinder;
  floppy.sector = (targetSector + 1) % FLOPPY_SECTORS_PER_TRACK;
  floppyDelayMs((int)total_wait);
}

static void floppyReadSector(void) {
  uint8_t sector = (uint8_t)((floppy.lba % FLOPPY_SECTORS_PER_TRACK) + 1);
  uint8_t cylinder =
      (uint8_t)((floppy.lba / FLOPPY_SECTORS_PER_TRACK) / FLOPPY_HEADS);
  floppySimulateDelayAndUpdateCHS(cylinder, sector);

  uint32_t offset = (uint32_t)floppy.lba * FLOPPY_BYTES_PER_SECTOR;
  for (int i = 0; i < FLOPPY_BYTES_PER_SECTOR; i++) {
    if ((uint32_t)(floppy.dmaAddr + i) > 0xFFFF)
      break;
    write6502((uint16_t)(floppy.dmaAddr + i), flpBuffer[offset + i]);
  }
}

static void floppyWriteSector(void) {
  uint8_t sector = (uint8_t)((floppy.lba % FLOPPY_SECTORS_PER_TRACK) + 1);
  uint8_t cylinder =
      (uint8_t)((floppy.lba / FLOPPY_SECTORS_PER_TRACK) / FLOPPY_HEADS);
  floppySimulateDelayAndUpdateCHS(cylinder, sector);

  uint32_t offset = (uint32_t)floppy.lba * FLOPPY_BYTES_PER_SECTOR;
  for (int i = 0; i < FLOPPY_BYTES_PER_SECTOR; i++) {
    if ((uint32_t)(floppy.dmaAddr + i) > 0xFFFF)
      break;
    flpBuffer[offset + i] = read6502((uint16_t)(floppy.dmaAddr + i));
  }
}

static void floppyDelayMs(int milliseconds) {
#ifdef _WIN32
  Sleep(milliseconds);
#else
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000L;
  int res;
  do {
    res = nanosleep(&ts, &ts);
  } while (res && errno == EINTR);
#endif
}
