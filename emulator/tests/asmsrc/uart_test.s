  .include vars.s
  .dsb $2000, 0
  .org $2000

_main:
  ; Print prompt
  lda #<msg1
  sta STRPTR
  lda #>msg1
  sta STRPTR+1
  jsr puts

  ; Get input
  lda #<buf
  sta STRPTR
  lda #>buf
  sta STRPTR+1
  jsr gets

  ; Print the next prompt
  lda #<msg2
  sta STRPTR
  lda #>msg2
  sta STRPTR+1
  jsr puts

  ; Print what was received
  lda #<buf
  sta STRPTR
  lda #>buf
  sta STRPTR+1
  jsr puts

  ; Exit
  jmp exit
  
msg1: .asciiz "Give a string: "
msg2: .asciiz "You gave: "
buf:  .dsb $ff, 0
_main_end:

  .org $8000
  .include bios.s
