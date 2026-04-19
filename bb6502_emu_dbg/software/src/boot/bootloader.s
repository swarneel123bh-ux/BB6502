; ============================================================
; bootloader.s — loads the kernel from floppy into RAM and jumps to it
; ============================================================

.ifndef BOOTLOADER_INCLUDED
BOOTLOADER_INCLUDED = 1

.include "vars.s"

.export _bootloader

; ============================================================
; How many sectors to load.
;
; Each sector is 512 bytes.  Kernel runs at KERNEL_LOAD_ADDR ($0B6A).
;
; Current kernel size: ~908 bytes → 2 sectors (1024 B) is sufficient.
;
; Hard ceiling before BIOS ROM at $8000:
;   max sectors = ($8000 - KERNEL_LOAD_ADDR) / $200 = 58
;
; To grow: raise this AND raise FLOPPY size in floppy.cfg by the same
; number of sectors, AND update KERNELRAM start in floppy.cfg to:
;   KERNELRAM start = KERNEL_LOAD_ADDR + (BOOT_SECTOR_COUNT × $200)
; ============================================================
BOOT_SECTOR_COUNT = 2

; ============================================================
; CODE SEGMENT
; ============================================================
.segment "BOOTLOADER"

_bootloader:
    ; ── Point STRPTR at the DMA destination ───────────────────
    lda #<KERNEL_LOAD_ADDR
    sta STRPTR
    lda #>KERNEL_LOAD_ADDR
    sta STRPTR+1

    ; ── floppy_read calling convention ────────────────────────
    ; A = number of sectors to load
    ; Y = starting LBA (0 = first sector of the floppy image)
    ; STRPTR = DMA destination address (set above)
    lda #BOOT_SECTOR_COUNT
    ldy #0
    jsr floppy_read

    ; ── Check for error (floppy_read sets Carry on error) ─────
    bcs @error

    ; ── Kernel is loaded — jump to it ─────────────────────────
    jmp KERNEL_LOAD_ADDR

@error:
    lda #<msg_boot_error
    sta STRPTR
    lda #>msg_boot_error
    sta STRPTR+1
    jsr puts
    rts                     ; return to BIOS reset → hang


; ============================================================
; RODATA
; ============================================================
.segment "BOOTRODATA"
msg_boot_error:
    .byte "Boot error: floppy read failed", $0D, $0A, $00

.endif
