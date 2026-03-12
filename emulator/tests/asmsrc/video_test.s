.include "vars.s"

.segment "CODE"

_main:
  lda #$01
  tax
  lda #'c'
  jsr putc
  jmp exit

.segment "RODATA"
msg: .byte "hello vga", $0d, $0a, $00

.segment "BIOS"
.include "bios.s"
