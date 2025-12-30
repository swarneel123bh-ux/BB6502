ANSWER = $6001

loadstore:
  lda #$01
  adc #$02
  sta ANSWER

loop:
  jmp loop


  .org $fffc
resetv:
  .word loadstore

  .org $ffff
exitv:
  .byte $00
  
