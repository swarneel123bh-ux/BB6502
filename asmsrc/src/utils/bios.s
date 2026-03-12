; ------
; BIOS CODE
; Assembled into the ROM segment by the linker config
; ------

.ifndef BIOS_INCLUDED
BIOS_INCLUDED = 1

.include "vars.s"

; ----------------------------------------
; EXPORTS — visible to bootloader + kernel via .import
; ----------------------------------------
.export reset
.export puts, gets, putc, getc
.export hang, exit, retctrl
.export floppy_read, floppy_write
.export nmi, irq

; ----------------------------------------
; CODE SEGMENT
; ----------------------------------------
.segment "BIOS"

; ----------------------------------------------------------------
; ENTRY POINT AT RESET
; ----------------------------------------------------------------
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

  jsr _bootloader       ; call bootloader — if it returns, no kernel found

  lda #<msg_nokernel
  sta STRPTR
  lda #>msg_nokernel
  sta STRPTR+1
  jsr puts
  jmp hang


; ------
; exit — returns control to debugger
; ------
exit:
  lda IXFLAGREG
  ora #%10000000
  sta IXFLAGREG
@exit_wait:
  jmp @exit_wait


; ------
; hang — halt forever
; ------
hang:
  lda #<msg_hanging
  sta STRPTR
  lda #>msg_hanging
  sta STRPTR+1
  jsr puts
@loop:
  jmp @loop


; ------
; retctrl — pause and return control to debugger
; ------
retctrl:
  lda IXFLAGREG
  ora #%01000000
  sta IXFLAGREG
  rts


; ------
; puts — print null-terminated string
; In:  STRPTR / STRPTR+1 = address of string
; ------
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


; ------
; gets — read a line into buffer
; In:  STRPTR / STRPTR+1 = address of destination buffer
; Out: A = length of string read
; ------
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


; ------
; putc — send byte to UART
; In:  A = byte to send
;      X = output mode (00=text, 01=VGA TBD)
; ------
putc:
  pha
@putc_wait:
  lda IXFLAGREG
  and #%00000001
  bne @putc_wait
  pla
  sta UARTOUTREG
  pha
  lda IXFLAGREG
  ora #%00000001
  sta IXFLAGREG
  pla
  rts


; ------
; getc — wait for a keypress, return byte in A
; ------
getc:
  lda IXFLAGREG
  ora #%00000100
  sta IXFLAGREG
@getc_wait:
  lda IXFLAGREG
  and #%00000010
  beq @getc_wait
  lda UARTSHADOW
  pha
  lda IXFLAGREG
  and #%11111001
  sta IXFLAGREG
  pla
  cli
  rts


; ----------------------------------------------------------------
; FLOPPY SUBROUTINES
; ----------------------------------------------------------------

; ------
; floppy_write
; In:  A = number of sectors to write
;      Y = LBA address on floppy
;      FLPDMAREG = 16-bit DMA source address
; ------
floppy_write:
  pha
  sta FLPSECREG
  tya
  sta FLPLBAREG
  lda IXFLAGREG
  ora #%00010000
  sta IXFLAGREG
@floppy_write_wait:
  lda IXFLAGREG
  and #%00001000
  bne @floppy_write_wait
  pla
  rts


; ------
; floppy_read
; In:  A = number of sectors to read
;      Y = LBA address on floppy
;      FLPDMAREG = 16-bit DMA destination address
; ------
floppy_read:
  pha
  sta FLPSECREG
  tya
  sta FLPLBAREG
  lda IXFLAGREG
  ora #%00100000
  sta IXFLAGREG
@floppy_read_wait:
  lda IXFLAGREG
  and #%00000100
  bne @floppy_read_wait
  pla
  rts


; ----------------------------------------------------------------
; INTERRUPT HANDLERS
; ----------------------------------------------------------------
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


; ----------------------------------------------------------------
; BIOS MESSAGES
; ----------------------------------------------------------------
.segment "BIOSRODATA"
msg_bios:
  .byte "Hello from BIOS", $0d, $0a, $00
msg_nokernel:
  .byte "No kernel found, exiting", $0d, $0a, $00
msg_hanging:
  .byte "Hanging...", $0d, $0a, $00


; ----------------------------------------------------------------
; DEVICE TABLES  (placed at exact addresses by linker config)
; ----------------------------------------------------------------
.segment "DEVTABLE"
floppy_lba_reg_addr:        .word FLPLBAREG   ; $ffe4-$ffe5
floppy_dma_reg_addr:        .word FLPDMAREG   ; $ffe6-$ffe7
floppy_sector_count_reg_addr: .word FLPSECREG ; $ffe8-$ffe9
uart_inreg_addr:            .word UARTINREG   ; $ffea-$ffeb
uart_outreg_addr:           .word UARTOUTREG  ; $ffec-$ffed
ixflagreg_addr:             .word IXFLAGREG   ; $ffee-$ffef


; ----------------------------------------------------------------
; CPU VECTORS  (placed at $fffa by linker config)
; ----------------------------------------------------------------
.segment "VECTORS"
.word nmi         ; $fffa-$fffb
.word reset       ; $fffc-$fffd
.word irq         ; $fffe-$ffff

.endif
