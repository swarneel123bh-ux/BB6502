; ------
; BOOTLOADER
; Loaded by BIOS reset, loads kernel from floppy into RAM and jumps to it
; ------

.ifndef BOOTLOADER_INCLUDED
BOOTLOADER_INCLUDED = 1

.include "vars.s"

; ----------------------------------------
; NOTE: When assembled as part of rom.s, all BIOS symbols are
; directly visible. .import is only needed for standalone assembly.
; ----------------------------------------

; ----------------------------------------
; EXPORTS
; ----------------------------------------
.export _bootloader

; ----------------------------------------
; CODE SEGMENT
; ----------------------------------------
.segment "BOOTLOADER"

_bootloader:
  ldy #0        ; LBA address of kernel sector on floppy

  ; Set DMA destination address to $2000 (low byte first)
  lda #$00
  sta FLPDMAREG
  lda #$20
  sta FLPDMAREG+1

  lda #64        ; Number of sectors to load

  jsr floppy_read

  ; Kernel is now loaded at $2000 in RAM — jump to it
  jmp KERNEL_LOAD_ADDR


; ----------------------------------------
; RODATA
; ----------------------------------------
.segment "BOOTRODATA"
msg_hello_bootloader:
  .byte "Hello from bootloader", $0d, $0a, $00

.endif
