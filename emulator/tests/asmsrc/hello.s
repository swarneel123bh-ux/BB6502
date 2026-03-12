.include "vars.s"

.segment "CODE"

_main:
  cli
  lda #<msg
  sta STRPTR
  lda #>msg
  sta STRPTR+1
  jsr puts
  jmp exit

.segment "RODATA"
msg:
  .byte "Hello from prog", $0d, $0a, $00

.segment "BIOS"
.include "bios.s"
