.include "vars.s"

.segment "CODE"

_main:
  lda #<msg1
  sta STRPTR
  lda #>msg1
  sta STRPTR+1
  jsr puts

  lda #<buf
  sta STRPTR
  lda #>buf
  sta STRPTR+1
  jsr gets

  lda #<msg2
  sta STRPTR
  lda #>msg2
  sta STRPTR+1
  jsr puts

  lda #<buf
  sta STRPTR
  lda #>buf
  sta STRPTR+1
  jsr puts

  jmp exit

.segment "RODATA"
msg1: .byte "Give a string: ", $00
msg2: .byte "You gave: ", $00
buf:  .res $ff, 0

.segment "BIOS"
.include "bios.s"
