// ============================================================================
// floppy.h — emulated 3.5" 1.44 MB floppy controller with DMA
//
// 6502-visible registers (addresses come from the device table at $FF00):
//   STATUS  IDLE / BUSY / ERROR / IRQ bits (below)
//   CMD     write a command; the worker resets it to NO_CMD when finished
//   DATA    2 bytes: DMA address (lo, hi) or LBA (lo byte only, today)
//
// Typical sector read, as done by the BIOS floppy_read:
//   DATA = dest addr, CMD = SET_DMA_ADDR, wait CMD == 0
//   DATA = lba,       CMD = STORE_LBA,    wait CMD == 0
//   CMD  = READ_SECTOR  → 512 bytes are DMA'd into RAM, STATUS.IRQ is set
//                          and an IRQ is raised.
//
// The whole disk image is held in RAM (flpBuffer). Writes only change that
// buffer: they are NOT written back to the image file.
// ============================================================================
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
//   RESET          no-op handshake (returns to IDLE)
//   SET_DMA_ADDR   DMA address = DATA (16-bit LE)
//   STORE_LBA      LBA = DATA low byte (so only sectors 0–255 are reachable)
//   READ_SECTOR    disk[LBA] → RAM[DMA address], 512 bytes, then IRQ
//   WRITE_SECTOR   RAM[DMA address] → disk[LBA], 512 bytes, then IRQ
// READ/WRITE with LBA >= 2880 set STATUS.ERROR (still raise the IRQ).
enum floppy_commands_t {
    FLOPPY_CMD_NO_CMD       = 0x00,
    FLOPPY_CMD_RESET        = 0x01,
    FLOPPY_CMD_SET_DMA_ADDR = 0x02,
    FLOPPY_CMD_STORE_LBA    = 0x03,
    FLOPPY_CMD_READ_SECTOR  = 0x04,
    FLOPPY_CMD_WRITE_SECTOR = 0x05
};

// ─── Floppy status register bitmasks ─────────────────────────────────────────
// IRQ is set when a READ/WRITE finishes; the BIOS IRQ handler clears it.
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
    // 6502-visible registers. Currently unused: the real values live
    // directly in mem6502 at the addresses above. `head` is unused too.
    uint8_t  status;
    uint8_t  cmd;
    uint8_t  data;
} floppy_t;

extern floppy_t floppy;

// Read register addresses from the device table, load the image (or start
// blank if none / missing), mark STATUS = IDLE and start the worker thread.
extern void  floppyInit(void);
// Worker thread: sleeps on floppyCond until CMD != NO_CMD, then executes it.
extern void *floppyWorker(void *args);
