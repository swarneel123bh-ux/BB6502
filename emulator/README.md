# emulator/ (legacy)

The original single-threaded BB6502 emulator, built around an **ncurses debugger**. It has been superseded by the threaded emulator in [`../bb6502_emu_dbg/emu`](../bb6502_emu_dbg/emu), but it is kept because it still has things the new emulator lacks:

- breakpoints (by address or symbol), single-stepping, `continue`
- a disassembler (`optable` in `src/include/debugger.h`)
- memory dumps and register display
- a loader for ld65 `--dbgfile` debug symbols

It uses the **old device protocol** (one IXFLAGREG flag register, UART-style text I/O, 256-byte floppy sectors), so it only runs the old OS in [`../asmsrc`](../asmsrc) and the tests in `tests/asmsrc/`. It cannot run the new BIOS.

See [`../docs/legacy.md`](../docs/legacy.md) for build steps, the old memory map and known bugs.
