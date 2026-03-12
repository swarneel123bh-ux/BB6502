; ------
; BIOS CODE — ca65 version
; Included into test binaries at $8000
; ------

.ifndef BIOS_INCLUDED
BIOS_INCLUDED = 1

; ----------------------------------------
; RESET
; ----------------------------------------
reset:
  sei
  cld
  ldx #$FF
  txs

  lda #<msg_bios
  sta STRPTR
  lda #>msg_bios
  sta STRPTR+1
  jsr puts

  jmp _main

  lda #<msg_nomain
  sta STRPTR
  lda #>msg_nomain
  sta STRPTR+1
  jsr puts
  jmp hang
  jmp exit


; ----------------------------------------
; exit
; ----------------------------------------
exit:
  lda IXFLAGREG
  ora #%10000000
  sta IXFLAGREG
@wait:
  jmp @wait


; ----------------------------------------
; hang
; ----------------------------------------
hang:
  lda #<msg_exit_fail
  sta STRPTR
  lda #>msg_exit_fail
  sta STRPTR+1
  jsr puts
@loop:
  jmp @loop


; ----------------------------------------
; retctrl
; ----------------------------------------
retctrl:
  lda IXFLAGREG
  ora #%01000000
  sta IXFLAGREG
  rts


; ----------------------------------------
; puts
; ----------------------------------------
puts:
  ldy #$00
@loop:
  lda (STRPTR),y
  beq @done
  ldx #00
  jsr putc
  iny
  jmp @loop
@done:
  rts


; ----------------------------------------
; gets
; ----------------------------------------
gets:
  ldy #$00
@loop:
  jsr getc
  ldx #00
  jsr putc
  cmp #$0a
  beq @done
  sta (STRPTR),y
  iny
  beq @loopback
  jmp @loop
@loopback:
  dey
@done:
  lda #$00
  sta (STRPTR),y
  tya
  rts


; ----------------------------------------
; putc
; ----------------------------------------
putc:
  pha
@wait:
  lda IXFLAGREG
  and #%00000001
  bne @wait
  pla
  sta UARTOUTREG
  pha
  lda IXFLAGREG
  ora #%00000001
  sta IXFLAGREG
  pla
  rts


; ----------------------------------------
; getc
; ----------------------------------------
getc:
  lda IXFLAGREG
  ora #%00000100
  sta IXFLAGREG
@wait:
  lda IXFLAGREG
  and #%00000010
  beq @wait
  lda UARTSHADOW
  pha
  lda IXFLAGREG
  and #%11111001
  sta IXFLAGREG
  pla
  cli
  rts


; ----------------------------------------
; floppy_write
; ----------------------------------------
floppy_write:
  pha
  sta FLPSECREG
  tya
  sta FLPLBAREG
  lda IXFLAGREG
  ora #%00010000
  sta IXFLAGREG
@wait:
  lda IXFLAGREG
  and #%00001000
  bne @wait
  pla
  rts


; ----------------------------------------
; floppy_read
; ----------------------------------------
floppy_read:
  pha
  sta FLPSECREG
  tya
  sta FLPLBAREG
  lda IXFLAGREG
  ora #%00100000
  sta IXFLAGREG
@wait:
  lda IXFLAGREG
  and #%00000100
  bne @wait
  pla
  rts


; ----------------------------------------
; BIOS messages
; ----------------------------------------
msg_bios:
  .byte "Hello from BIOS", $0d, $0a, $00
msg_nomain:
  .byte "No _main found, exiting", $0d, $0a, $00
msg_exit_fail:
  .byte "Failed to exit, hanging...", $0d, $0a, $00


; ----------------------------------------
; INTERRUPTS
; ----------------------------------------
nmi:
  rti

irq:
  pha
  txa
  pha
  tya
  pha

  lda IXFLAGREG
  and #%00001000
  beq @notFloppy
  and #%11000111
  sta IXFLAGREG
  pla
  tay
  pla
  tax
  pla
  rti

@notFloppy:
  lda IXFLAGREG
  lda UARTINREG
  sta UARTSHADOW
  pla
  tay
  pla
  tax
  pla
  rti


; ----------------------------------------
; DEVICE TABLE at $FFE4
; ----------------------------------------
.segment "DEVTABLE"
floppy_lba_reg_addr:          .word FLPLBAREG   ; $ffe4-$ffe5
floppy_dma_reg_addr:          .word FLPDMAREG   ; $ffe6-$ffe7
floppy_sector_count_reg_addr: .word FLPSECREG   ; $ffe8-$ffe9
uart_inreg_addr:              .word UARTINREG   ; $ffea-$ffeb
uart_outreg_addr:             .word UARTOUTREG  ; $ffec-$ffed
ixflagreg_addr:               .word IXFLAGREG   ; $ffee-$ffef


; ----------------------------------------
; VECTORS at $FFFA
; ----------------------------------------
.segment "VECTORS"
.word nmi
.word reset
.word irq

.endif
