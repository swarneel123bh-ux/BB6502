# BIOS and kernel

## Calling conventions

- Arguments go in registers (A, X, Y) or in the BIOS zero-page pointers below. Results come back in A and the carry flag.
- **STRPTR, CMPPTR and JMPPTR are scratch.** Assume any BIOS call clobbers them, and reload STRPTR before every call that uses it.
- Calls are plain `jsr`. The kernel resolves BIOS addresses **by symbol at link time** (`.import puts` …). This means the kernel must be relinked whenever the BIOS changes. Separately built programs have no stable entry points yet.

### BIOS zero page (`$00-$0F`)

| Addr | Name | Use |
|---|---|---|
| `$00-$01` | STRPTR | Main pointer argument (string / buffer / DMA address) |
| `$02-$03` | CMPPTR | Second pointer; floppy routines use it internally as counter + LBA |
| `$04-$05` | JMPPTR | Pointer scratch (the kernel's command-table walker) |
| `$06` | KBD_LAST | Last key from the IRQ handler; 0 = none |
| `$07` | FLOPPY_DONE | Set to 1 by the IRQ handler when a sector transfer finishes |
| `$08` | DISPGFX_VRAM_SHADOW | `putcg` temp |
| `$09-$0A` | DISPGFX_VRAM_WPTR | Next VRAM cell to write |
| `$0B-$0C` | DISPGFX_CRAM_WPTR | Matching CRAM cell (see note under `putcg`) |
| `$0D` / `$0E` | DISPGFX_CURS_ROW / COL | Text cursor position |

## BIOS routines

All of these are in [`bios.s`](../bb6502_emu_dbg/software/src/utils/bios.s).

### Graphical display

| Routine | In | Out / clobbers | Notes |
|---|---|---|---|
| `putcg` | A = char | A (and X when the screen scrolls); Y preserved | Handles CR, LF, BS; wraps at column 40; scrolls at row 30. Does not move the hardware cursor. |
| `putsg` | STRPTR = string | A, Y | Prints a null-terminated string (max 255 chars), then updates the hardware cursor |
| `getcg` | none | A = key, X | Blocks until a key arrives. Executes `CLI` and leaves IRQs enabled. No echo. |
| `getsg` | STRPTR = 256-byte buffer | A = length | Reads a line with echo, ending on CR/LF; the result is null-terminated |

### Text console (host terminal)

`putc`, `puts`, `getc` and `gets` behave like the graphical versions, but write to the text console device.

### Floppy

| Routine | In | Out |
|---|---|---|
| `floppy_read` | A = sector count, Y = start LBA (0-255), STRPTR = destination | C = 1 on error. Clobbers A, X, Y, STRPTR (advanced past the data), CMPPTR. Returns with `I` set. |
| `floppy_write` | Same, with STRPTR = source | Same |

Sectors are 512 bytes. Each sector is one DMA transfer completed by an IRQ. The routine enables IRQs while it waits.

### Control

| Routine | Effect |
|---|---|
| `reset` | Cold start (the RESET vector) |
| `hang` | Prints "Halted" and spins forever |
| `exit` | Same as `hang` (there is no power-off device yet) |
| `irq` | Shared IRQ handler: keyboard, then floppy |
| `nmi` | `rti` |

### Display quirks to know about

- A `"$0D,$0A"` line ending prints as **two** newlines, because CR and LF each start a new line.
- `putcg` advances the CRAM pointer only on newlines, so per-character colour writes through `DISPGFX_CRAM_WPTR` would land in the wrong cells. Compute CRAM addresses yourself (`CRAM_BASE + row*40 + col`) for now.
- `getsg`/`gets` currently store backspace characters in the buffer, so correcting a typo in a command still gives "Invalid command".

## Bootloader

[`bootloader.s`](../bb6502_emu_dbg/software/src/boot/bootloader.s) loads `BOOT_SECTOR_COUNT` sectors (currently 2, 1 KB) from LBA 0 to `KERNEL_LOAD_ADDR` (`$0B6A`) and jumps there. To grow the kernel, see [build-system.md](build-system.md#growing-the-kernel).

## Kernel shell

[`kernel.s`](../bb6502_emu_dbg/software/src/boot/kernel.s):

```
_kernel:  print "Hello from kernel"
shell:    print ">>> "
          getsg → kernel_ipbuf
          kernel_dispatch_cmd
          repeat
```

`kernel_dispatch_cmd` walks a table of `(name pointer, handler pointer)` pairs that ends with `$0000,$0000`. It compares the **whole input line** to each name with `kernel_cmp_str`, an exact, case-sensitive compare. On a match it calls the handler with an indirect jump (it pushes a return address, then does `jmp (CMPPTR)`). A and Y are preserved across the call.

Commands today: `help`, `exit`.

### Adding a command

```asm
; 1. handler (KERNEL segment)
kernel_hello:
    lda #<msg_hi
    sta STRPTR
    lda #>msg_hi
    sta STRPTR+1
    jsr putsg
    rts

; 2. strings (KERNELRODATA)
cmd_hello: .byte "hello", $00
msg_hi:    .byte "hi!", $0D, $00

; 3. table entry, before the sentinel
cmd_table_base:
    .word cmd_help,  kernel_show_help
    .word cmd_exit,  kernel_exit
    .word cmd_hello, kernel_hello
    .word $0000,     $0000
```

Commands cannot take arguments yet, because the whole line must match a name. Watch the size limit: the kernel image must fit in `BOOT_SECTOR_COUNT × 512` bytes.

> **Known bug:** the table walker advances its pointer with `adc #4` followed by `bne`, instead of checking the carry. If the table ever straddles a page boundary, dispatch breaks. The fix is to branch on carry (`bcc`) instead.
