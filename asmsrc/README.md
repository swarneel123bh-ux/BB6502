# asmsrc/ (legacy)

The first version of the BB6502 BIOS, bootloader and kernel shell, written for the legacy debugger emulator in [`../emulator`](../emulator). It uses the old memory map (kernel at `$2000`, device registers at `$1000`) and the old IXFLAGREG device protocol.

The current OS lives in [`../bb6502_emu_dbg/software`](../bb6502_emu_dbg/software). See [`../docs/legacy.md`](../docs/legacy.md) for details.
