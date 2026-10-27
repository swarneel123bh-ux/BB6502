# Hardware interface (emulator ↔ 6502)

This page defines the contract between the emulated hardware and the software running on it. If you build real hardware later, this is the spec it must meet.

## Device table (`$FF00-$FF0F`)

The ROM contains eight little-endian 16-bit pointers. At startup, the emulator reads them to learn where each register lives. The table is the `DEVTABLE` segment at the end of [`bios.s`](../bb6502_emu_dbg/software/src/utils/bios.s).

| Table entry | Points to | Current address |
|---|---|---|
| `$FF00` | floppy STATUS | `$0200` |
| `$FF02` | floppy CMD | `$0201` |
| `$FF04` | floppy DATA (2 bytes) | `$0202-$0203` |
| `$FF06` | keyboard DATA | `$0204` |
| `$FF08` | text console DATA | `$0205` |
| `$FF0A` | display CMD | `$0206` |
| `$FF0C` | display DATA (2 bytes) | `$0207-$0208` |
| `$FF0E` | display STATUS | `$0209` |

To move a device, change its constant in `vars.s` and rebuild the ROM. The emulator needs no changes.

## Common command protocol (floppy and display)

Both "smart" devices use the same handshake:

1. Wait until the device is ready: `CMD == 0` and the `STATUS.IDLE` bit is set.
2. Write any arguments to `DATA`.
3. Write the command code to `CMD`. This wakes the device's worker thread.
4. The device sets `BUSY`, does the work, writes `CMD = 0`, and goes back to `IDLE`.
5. The CPU spins until `CMD == 0` (and `IDLE` is set).

Always wait for `CMD` to read back 0, not just for `IDLE`. Otherwise the CPU can see a stale `IDLE` before the worker thread has even woken up. The BIOS helpers `_floppy_wait_cmd` and `dispgfx_wait_idle` do this.

## Floppy controller

Source: [`emu/src/include/floppy.c`](../bb6502_emu_dbg/emu/src/include/floppy.c). The disk is an emulated 3.5" 1.44 MB floppy: 2880 sectors of 512 bytes, 80 cylinders, 2 heads, 18 sectors per track.

**STATUS bits**

| Bit | Name | Meaning |
|---|---|---|
| `$01` | IDLE | Ready for a command |
| `$02` | BUSY | Executing a command |
| `$04` | ERROR | The last READ/WRITE had a bad LBA (≥ 2880) |
| `$08` | IRQ | A READ/WRITE finished; the IRQ handler clears this bit |

**Commands**

| Code | Command | Argument | Effect |
|---|---|---|---|
| `$01` | RESET | none | Handshake only |
| `$02` | SET_DMA_ADDR | DATA = 16-bit RAM address | Sets the DMA address |
| `$03` | STORE_LBA | DATA low byte = sector | Sets the LBA (8-bit, so only sectors 0-255) |
| `$04` | READ_SECTOR | none | Copies sector LBA → RAM at the DMA address (512 B), then raises an IRQ |
| `$05` | WRITE_SECTOR | none | Copies RAM at the DMA address → sector LBA (512 B), then raises an IRQ |

READ and WRITE sleep for a realistic seek plus rotation time (360 RPM, 5 ms per track), so a multi-sector load takes visible time.

**Limitations:**

- The image file must be exactly 1,474,560 bytes.
- Writes change only the in-memory copy and are lost when the emulator exits.
- Only the first 256 sectors (128 KB) are addressable until STORE_LBA takes 16 bits.

## Keyboard

Source: [`kbd.c`](../bb6502_emu_dbg/emu/src/include/kbd.c), plus SDL key forwarding in [`dispgfx.c`](../bb6502_emu_dbg/emu/src/include/dispgfx.c).

The keyboard has a single register, DATA:

- The device writes a byte and raises an IRQ.
- The CPU reads the byte and writes 0 back to acknowledge it.
- Input comes from both the SDL window and the terminal that launched the emulator. ESC in either one quits.
- Enter sends `$0D`, Backspace `$08`, Tab `$09`, Delete `$7F`, Ctrl-A..Z `$01-$1A`. The arrow keys send `ESC [ A/B/C/D`.

**Limitation:** there is no FIFO. A second key that arrives before the first is acknowledged overwrites it. This means multi-byte sequences such as arrow keys usually arrive with bytes missing.

## Text console (disptext)

Source: [`disptext.c`](../bb6502_emu_dbg/emu/src/include/disptext.c).

The text console has a single register, DATA. The CPU writes a nonzero character, the device prints it to the host terminal's stdout, and the device writes 0 back. The CPU must wait for 0 before sending the next character. Writing `$FF` stops the worker. This is the "serial console", used by the BIOS `putc`/`puts`/`gets`.

## Display (dispgfx)

Source: [`dispgfx.c`](../bb6502_emu_dbg/emu/src/include/dispgfx.c).

A 40×30 text mode with 8×8 glyphs (320×240 pixels, shown scaled 3× in an SDL window). It is redrawn at about 60 fps directly from 6502 RAM:

- **VRAM**: 1200 bytes, one ASCII code per cell, index = `row*40 + col`. Codes ≥ 128 draw as blank.
- **CRAM**: 1200 bytes, one attribute per cell: high nibble = background, low nibble = foreground.

The 16 CGA colours are: 0 black, 1 dark blue, 2 dark green, 3 dark cyan, 4 dark red, 5 dark magenta, 6 brown, 7 light grey, 8 dark grey, 9 blue, 10 green, 11 cyan, 12 light red, 13 magenta, 14 yellow, 15 white. The default attribute is `$07` (light grey on black).

**Commands**

| Code | Command | DATA |
|---|---|---|
| `$01` | SET_VRAM | 16-bit VRAM base address |
| `$02` | SET_CRAM | 16-bit CRAM base address |
| `$03` | CLEAR | Fills VRAM with spaces and CRAM with `$07` |
| `$04` | SET_CURSOR | low byte = column, high byte = row |
| `$05` / `$06` | CURSOR_ON / OFF | Blinking block cursor (about 0.5 s phase) |
| `$07` | SET_BORDER | Low nibble = colour index |

**STATUS bits:** `$01` IDLE, `$02` BUSY, `$04` VBLANK. VBLANK is set at the end of each rendered frame and cleared after the frame is presented.
