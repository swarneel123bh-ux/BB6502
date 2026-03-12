.include "vars.s"
.include "wozvars.s"

.segment "CODE"

_main:
  lda #<wozmsg
  sta STRPTR
  lda #>wozmsg
  sta STRPTR+1
  jsr puts
  jmp _wozmon
  jmp exit

.segment "RODATA"
wozmsg: .byte "Jumping to wozmon", $0d, $0a, $00

.segment "CODE"
_wozmon:
.include "mywozmon.s"

.segment "BIOS"
.include "bios.s"
