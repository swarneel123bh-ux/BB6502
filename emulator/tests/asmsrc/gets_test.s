.include "vars.s"

.segment "CODE"

_main:
  jsr gets
  jsr putc
  jmp exit

.segment "BIOS"
.include "bios.s"
