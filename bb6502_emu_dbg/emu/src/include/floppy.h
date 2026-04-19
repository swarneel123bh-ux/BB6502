#pragma once

#include <stdint.h>
#include <stdio.h>

// ─── Floppy physical parameters ───────────────────────────────────────────────
#define FLOPPY_RPM                  360
#define FLOPPY_TRACK_TO_TRACK_SEEK_TIME 5  // milliseconds
#define FLOPPY_HEADS                2
#define FLOPPY_CYLINDERS            80
#define FLOPPY_TOTAL_SECTORS        2880
#define FLOPPY_SECTORS_PER_TRACK    18
#define FLOPPY_BYTES_PER_SECTOR     512
#define FLOPPY_TRACKS_PER_SIDE      80
#define FLOPPY_SIDES                2
#define FLOPPY_TOTAL_CAPACITY       1474560  // bytes

// ─── Actual register addresses (loaded from device table at init) ─────────────
extern uint16_t floppyCmdRegAddr;
extern uint16_t floppyStatusRegAddr;
extern uint16_t floppyDataRegAddr;

// Floppy image file name (set before floppyInit is called)
extern char flpFileName[FILENAME_MAX];

// ─── Floppy commands ──────────────────────────────────────────────────────────
enum floppy_commands_t {
    FLOPPY_CMD_NO_CMD       = 0x00,
    FLOPPY_CMD_RESET        = 0x01,
    FLOPPY_CMD_SET_DMA_ADDR = 0x02,
    FLOPPY_CMD_STORE_LBA    = 0x03,
    FLOPPY_CMD_READ_SECTOR  = 0x04,
    FLOPPY_CMD_WRITE_SECTOR = 0x05
};

// ─── Floppy status register bitmasks ─────────────────────────────────────────
#define FLOPPY_STATUS_IDLE  0x01
#define FLOPPY_STATUS_BUSY  0x02
#define FLOPPY_STATUS_ERROR 0x04
#define FLOPPY_STATUS_IRQ   0x08

// ─── Floppy internal state ────────────────────────────────────────────────────
typedef struct floppy_t {
    // 6502-invisible internal registers
    uint8_t  cylinder;
    uint8_t  head;
    uint8_t  sector;
    uint16_t lba;
    uint16_t dmaAddr;
    // 6502-visible registers (mirrored in mem6502)
    uint8_t  status;
    uint8_t  cmd;
    uint8_t  data;
} floppy_t;

extern floppy_t floppy;

extern void  floppyInit(void);
extern void *floppyWorker(void *args);
