# Build system

## Toolchain

| Tool | Used for |
|---|---|
| `ca65` / `ld65` (cc65) | Assembling and linking the 6502 software |
| `gcc` or `clang` | The emulator |
| SDL2 (`sdl2-config` must be on PATH) | The emulator's display window |
| `truncate` or `dd` | Padding the floppy image |

## Software (`bb6502_emu_dbg/software/Makefile`)

```bash
make -C bb6502_emu_dbg/software
```

```bash
make -C bb6502_emu_dbg/software clean
```

The build works as follows:

```
src/rom.s ──(.include vars.s, bootloader.s, bios.s)──► ca65 ► build/obj/rom.o
src/boot/kernel.s ──(.include vars.s)──────────────────► ca65 ► build/obj/kernel.o

rom.o     + rom.cfg    ──► ld65 ► build/rom.bin      (32 KB, loaded at $8000)
kernel.o + rom.o + floppy.cfg ──► ld65 ► build/floppy.bin ──► padded to 1,474,560 B
```

- `rom.s` is a one-file build: it textually includes the bootloader and the BIOS, so they share symbols without imports.
- Each build writes debug info to `build/debug/*.dbg` (ld65 `--dbgfile`). The new emulator ignores it; the legacy debugger can load it with `-d`.
- `-I src/utils` lets every file `.include "vars.s"`, and the `VARS_INCLUDED` guard makes multiple includes safe.

### `rom.cfg`

One 32 KB region at `$8000`, filled with `$FF`. Segments are placed in this order: BOOTLOADER, BOOTRODATA, BIOS, BIOSRODATA, then **DEVTABLE pinned at `$FF00`** and **VECTORS pinned at `$FFFA`**.

### `floppy.cfg`

- `FLOPPY` (`$0B6A`, 1 KB) is the only region written to the file. It holds the KERNEL and KERNELRODATA segments, so file offset 0 corresponds to address `$0B6A`.
- `KERNELRAM` (`$0F6A`) holds KERNELBSS. It is not written to the file.
- `DUMMY_ROM` maps all ROM segments to nowhere. `rom.o` is passed to the linker only so that `.import puts` and the other BIOS imports resolve to their real ROM addresses.

Because of this, the kernel's calls into the BIOS are hardwired at link time. Change the BIOS, and you must rebuild **both** images (`make` does this).

### Growing the kernel

The kernel currently uses about 280 of its 1024 bytes. When it outgrows two sectors:

1. In `floppy.cfg`, increase `FLOPPY size` by `$200` per extra sector.
2. In `bootloader.s`, increase `BOOT_SECTOR_COUNT` by the same number.
3. In `floppy.cfg`, move `KERNELRAM start` to `$0B6A + new FLOPPY size`.
4. Keep `$0B6A + size` below `$8000` (at most 58 sectors). Also note that BSS then moves into what is currently the start of free RAM.

If the kernel grows past the region, ld65 fails with a "memory area overflow" error.

### Floppy image layout

| LBA | Contents |
|---|---|
| 0-1 | Kernel image (loaded to `$0B6A`) |
| 2-2879 | Zero padding; no filesystem yet |

## Emulator (`bb6502_emu_dbg/emu/Makefile`)

```bash
make -C bb6502_emu_dbg/emu
```

- Builds `build/release/bb6502_emu` (`-O2 -DNDEBUG`) and `build/debug/bb6502_emu_dbg` (`-O0 -g3 -DDEBUG`). Use `make release` or `make debug` to build only one.
- Sources are found automatically (`src/*.c`, `src/include/*.c`), and header dependencies are tracked with `-MMD -MP`.
- `compile_flags.txt` is for clangd (editor code completion); it is not used by the build.

## Run

```bash
bb6502_emu_dbg/emu/build/release/bb6502_emu bb6502_emu_dbg/software/build/rom.bin -f bb6502_emu_dbg/software/build/floppy.bin
```

Device startup messages and anything printed with `puts`/`putc` appear in the terminal. The 40×30 screen appears in the SDL window.

To debug the emulator itself, run the debug build under `lldb`.
