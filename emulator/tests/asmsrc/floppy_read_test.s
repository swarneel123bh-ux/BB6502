.include "vars.s"

.segment "CODE"

_main:
  lda #<msg1
  sta STRPTR
  lda #>msg1
  sta STRPTR+1
  jsr puts

  ; Set LBA address (sector 3)
  lda #$03
  tay

  ; Set DMA destination to $3000
  lda #$00
  sta FLPDMAREG
  lda #$30
  sta FLPDMAREG+1

  ; Read 1 sector
  lda #1
  jsr floppy_read

  lda #<msg2
  sta STRPTR
  lda #>msg2
  sta STRPTR+1
  jsr puts

  jsr retctrl
  jmp exit

.segment "RODATA"
msg1: .byte "Reading floppy", $00
msg2: .byte "Read done", $00
buf:  .res $ff, 0

.segment "BIOS"
.include "bios.s"
