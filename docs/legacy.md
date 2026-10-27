# Legacy tree: `emulator/` and `asmsrc/`

This is the first generation of the project. It is kept for its **debugger**, which the threaded emulator does not have yet. It cannot run the current BIOS, because it uses a different device protocol and memory map.

## `emulator/`: ncurses debugger emulator

Single-threaded. The CPU only runs while you step or `continue`, and the debugger services device requests between instructions.

Its screen has three panes: terminal (6502 I/O), source (not implemented) and the debugger console.

| Command | Aliases | Action |
|---|---|---|
| `b [addr\|symbol]` | `bp`, `break` | Set a breakpoint (at the current PC if no argument; addresses as `0x…`) |
| `rb <addr\|symbol>` | `rbp`, `remove` … | Remove a breakpoint (**broken**, see below) |
| `l` | `ls`, `listbp` | List breakpoints |
| `c` | `cont` | Run until a breakpoint, exit request or Ctrl-C |
| `s [n]` | `step` | Step n instructions |
| `d [n]` | `dis` | Disassemble n instructions from the PC |
| `m [start] [end]` | `mem` | Hex dump |
| `r` | `reg` | Registers |
| `h` | `help` | Help |
| `q` | `quit`, `exit` | Quit |

Build and run:

```bash
make -C emulator release
```

```bash
make -C asmsrc
```

```bash
emulator/build/bin/debug6502 asmsrc/build/rom.bin -f asmsrc/build/floppy.bin -d asmsrc/build/debug/rom.dbg -d asmsrc/build/debug/kernel.dbg
```

`make -C emulator run_test` assembles the test programs in `emulator/tests/asmsrc/` (each is a flat 64 KB image with its own copy of the BIOS) and runs `uart_test`. `wozmain.s` + `utils/mywozmon.s` is a port of Wozmon.

### Old device protocol

Device registers are found through a table at `$FFE4-$FFEF`. Everything is coordinated through one flag register, **IXFLAGREG**:

| Bit | Meaning |
|---|---|
| 7 | Exit request (stop the program) |
| 6 | Return control to the debugger |
| 5 | Floppy read request |
| 4 | Floppy write request |
| 3 | Floppy done (set by the debugger, cleared by the IRQ handler) |
| 2 | getc request |
| 1 | UART input ready |
| 0 | UART output byte waiting |

Floppy sectors are **256 bytes**. LBA, sector count and DMA address are in registers at `$1004-$1006`.

### Old memory map (`asmsrc/`)

| Range | Contents |
|---|---|
| `$0000-$0005` | STRPTR, CMPPTR, JMPPTR |
| `$1000-$1007` | Device registers |
| `$2000-$5FFF` | Kernel, loaded as 64 × 256 B sectors |
| `$8000-$FFFF` | ROM |
| `$9000` | Kernel BSS. **This is inside ROM space**; it only works because emulator ROM is writable. |

### Known bugs (if you revive it)

- `dbgRmvBp` binary search never recomputes `mid`, so it loops forever. `rb` hangs.
- `CMD_REMOVEBP` has no `break` before `default`.
- The breakpoint-list growth check uses integer division (`dbgNofBps / dbgBpListSize`), so the list only grows when it is completely full instead of at 70% capacity. This is harmless, but not what the code intends.
- The old BIOS `floppy_read` waits on bit 2 (the getc flag) instead of the floppy-done bit. It only works because the debugger services requests synchronously.

## Reuse ideas for the threaded emulator

The disassembler table (`optable` in `debugger.h`), the `.dbg` symbol loader (`dbgReadDbgSyms`) and the breakpoint logic can be lifted almost as-is. Pausing would be a flag the CPU thread checks in `cpuLoop()`.
