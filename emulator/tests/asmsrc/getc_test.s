.include "vars.s"

.segment "CODE"

_main:
  jsr getc
  jsr putc
  jmp exit

.segment "BIOS"
.include "bios.s"
