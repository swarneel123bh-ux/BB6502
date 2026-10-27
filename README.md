# BB6502

A homebrew 6502 computer, built from the software side first. It is loosely inspired by Ben Eater's breadboard 6502 and consists of:

- **A multithreaded C emulator.** The CPU and every peripheral (floppy controller with DMA, keyboard, text console, 40×30 colour text display) run on their own threads, so the 6502 talks to its devices the way it would talk to real asynchronous hardware: it writes registers, polls status bits and takes interrupts.
- **An OS written in 6502 assembly (ca65).** It has a BIOS in ROM, a bootloader that DMAs the kernel off a 1.44 MB floppy image, and a kernel with a table-driven command shell.

```
 ┌──────────── BB6502 emulator (C, pthreads + SDL2) ────────────┐
 │  CPU thread ── read6502()/write6502() ── 64 KB address space  │
 │                       │ MMIO dispatch                         │
 │   ┌─────────┬─────────┼──────────┬────────────┐               │
 │   floppy    kbd       disptext   dispgfx      (one worker      │
 │   (DMA,IRQ) (IRQ)     (stdout)   (SDL window)  thread each)    │
 └───────────────────────────────────────────────────────────────┘
        ROM: BIOS → bootloader → floppy LBA 0 → kernel shell
```

## Quick start

You need [cc65](https://cc65.github.io/) (`ca65`/`ld65`), SDL2 and a C compiler. On macOS: `brew install cc65 sdl2`.

```bash
make -C bb6502_emu_dbg/software
```

```bash
make -C bb6502_emu_dbg/emu
```

```bash
bb6502_emu_dbg/emu/build/release/bb6502_emu bb6502_emu_dbg/software/build/rom.bin -f bb6502_emu_dbg/software/build/floppy.bin
```

An SDL window opens showing the BIOS banner and the kernel's `>>>` prompt. Type `help` and press Enter. Press **ESC** to quit.

## Documentation

| Doc | What's in it |
|---|---|
| [docs/architecture.md](docs/architecture.md) | Big picture: boot sequence, memory map, how the pieces fit |
| [docs/hardware-interface.md](docs/hardware-interface.md) | Every device register and protocol, the device table, interrupts |
| [docs/bios-and-kernel.md](docs/bios-and-kernel.md) | BIOS routines and calling conventions, zero page, the shell, adding commands |
| [docs/emulator-internals.md](docs/emulator-internals.md) | Emulator threading model, MMIO dispatch, adding a new device |
| [docs/build-system.md](docs/build-system.md) | Makefiles, linker configs, floppy image layout, growing the kernel |
| [docs/legacy.md](docs/legacy.md) | The original ncurses debugger emulator and old OS (`emulator/`, `asmsrc/`) |

## Repository layout

```
bb6502_emu_dbg/
  emu/          threaded emulator (C)            ← current
  software/     BIOS, bootloader, kernel (ca65)  ← current
emulator/       legacy ncurses debugger emulator + test programs
asmsrc/         legacy OS for the debugger emulator
docs/           documentation
```

## Status

| Working | Not yet |
|---|---|
| Boot from ROM through the floppy to the kernel | Persistent floppy writes |
| 40×30 colour display with scrolling and a blinking cursor | Running external programs |
| Keyboard input via IRQ | Filesystem |
| Multi-sector floppy DMA reads/writes, IRQ-completed | Text editor |
| Command shell (`help`, `exit`) | Debugger for the threaded emulator |
