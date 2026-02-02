  .include vars.s
  .dsb $2000, 0
  .org $2000

_main:

  lda #$01
  tax
  lda #"c"
  jsr putc

  ; Exit
  jmp exit
  
msg:  .asciiz "hello vga", 0xd, 0xa, 0
_main_end:

  .org $8000
  .include bios.s
