; TESTING GET_C FUNCTIONALITY

  .include vars.s
  .dsb $2000, 0
  .org $2000

_main:

  ; Step from _main
  ; jsr retctrl
  ; Get character input
  jsr get_c
  ; Print what was received
  jsr put_c
  ; Exit
  jmp exit
_main_end:

  .org $8000
  .include bios.s
