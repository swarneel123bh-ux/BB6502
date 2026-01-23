; TESTING GET_C FUNCTIONALITY

  .include vars.s
  .dsb $2000, 0
  .org $2000

_main:
  ; Get character input
  jsr gets
  ; Print what was received
  jsr putc
  ; Exit
  jmp exit
_main_end:

  .org $8000
  .include bios.s
