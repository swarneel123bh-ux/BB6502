  .include vars.s

  ; ------ Padding with zeros ------
  .dsb $2000, 0

  .org $2000  ; THIS REMAINS CONSTANT ALWAYS
  _main:
    cli       ; renable interrupts
    lda #<msg 
    sta STRPTR
    lda #>msg
    sta STRPTR+1
    jsr puts
    jmp exit

msg: .asciiz "Hello from prog", $0d, $0a
_main_end:

    .org $8000
    .include bios.s
