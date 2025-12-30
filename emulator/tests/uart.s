  .include addrs.s



; -------------------
; Program specific addresses
; ---------------------
INTCNT = $4000  ; $4000 - $4001 -> 2 bytes



; ---------------
; Prog entry point at $8000 -> Fully in ROM but in emulator it will be in RAM
; ---------------
; .org $8000


; -------------------
; BIOS code
; -------------------
  .include bios.s


; --------------
; Reset code
; --------------
reset:

  cli             ; Enable interrupts

  lda #<testmsg
  sta STRPTR
  lda #>testmsg
  sta STRPTR+1
  jsr puts

  lda #<testmsg2
  sta STRPTR
  lda #>testmsg2
  sta STRPTR+1
  jsr puts
  

hang:
  jmp hang        ; Will hang, but since interrputs are enabled
                  ; Keypresses will be reflected

; --------------
; Non maskable interrupt code
; --------------
nmi:
  rti

; --------------
; Interrupt request handling code
; --------------
irq:
  pha
  txa 
  pha

  ; Detect keypresses by reading UARTIN
  ; Put the same byte into UARTOUT
  lda UARTIN
  sta UARTOUT

  ; Set the uart_out_flag so that the emulator can reflect it
  lda uart_out_flag
  ora #$01
  sta uart_out_flag

  ; Also increment the variable INTCNT
  lda INTCNT
  txa
  inx
  tax
  sta INTCNT
  ; If no loopover, then exit
  ; else increment the next byte
  bne irq_exit

  lda INTCNT+1
  txa
  inx
  tax
  sta INTCNT+1

  lda #<loopovermsg
  sta STRPTR
  lda #>loopovermsg
  sta STRPTR+1
  jsr puts

  pla
  tax
  pla

irq_exit:
  rti



; ----------------------
; Text data
; ---------------------
testmsg: .asciiz "Hi", $0D, $0A
testmsg2: .asciiz "Hangin...", $0D, $0A
loopovermsg: .asciiz "LOOPOVER!", $0D, $0A






;---------------
; Vectors and metadata
;---------------
  .include metadata.s
nmiv:
  .word nmi
resetv:
  .word reset
irqv:
  .word irq


