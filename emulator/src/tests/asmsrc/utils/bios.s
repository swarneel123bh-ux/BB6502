; ------
; BIOS CODE
; TO BE INCLUDED IN ROM AT ADDR $8000
; ------

; ------
; Reset code
; ------
reset:
  sei
  cld       ; Clear decimal mode
  ldx #$FF
  txs       ; Init stack at 0xff

  ; Show message
  lda #<msg_bios
  sta STRPTR
  lda #>msg_bios
  sta STRPTR+1
  jsr puts

  ; jsr retctrl

  ; Jump to _main (MUST HAVE)
  jmp _main

  ; Main jump failed, display message and exit
  lda #<msg_nomain
  sta STRPTR
  lda #>msg_nomain
  sta STRPTR+1
  jsr puts

  jmp hang

  ; If failed for any reason
  jmp exit


; ------
; label exit:
; params: None
; desc: Returns control to the debugger, only useful when using one else just sets a flag
; ------
exit:
  ; To return control to debugger we set b7 if IXFLAGREG
  lda IXFLAGREG
  ora #%10000000  ; Set b7 to indicate exit request
  sta IXFLAGREG
.wait:
  jmp .wait

; label hang:
; params: None
; desc: Hang the program till exit to debugger
; ------
hang:
  lda #<msg_exit_fail
  sta STRPTR
  lda #>msg_exit_fail
  sta STRPTR+1
  jsr puts
hang_:
  jmp hang_        ; If the debugger cannot notice the exit request then we hang till exit

; ------
; subroutine retctrl:
; params: None
; desc: Pauses exec and returns control to debugger
; ------
retctrl:
  lda IXFLAGREG
  ora #%01000000
  sta IXFLAGREG
  rts


; ------
; subroutine puts:
; params:
;   STRPTR: string addr low byte
;   STRPTR+1: string addr high byte
; desc: prints null terminated string by sending character data to uart
; ------
puts:
  ldy #$00

.loop:
  lda (STRPTR),y
  beq .done
  ldx #00
  jsr putc
  iny
  jmp .loop

.done:
  rts

; ------
; subroutine gets:
; params:
;   STRPTR: (2 bytes) address to the buffer to store the string to in little endian format
;           For now the subroutine is limited to max $ff bytes (256) but we will modify it later
; modifies:
;   A register: stores the length of string taken
; desc: the irq handler writes (n) bytes to IPBUF, we wait till (n) bytes have been written, not
; including carriage return and line feed. The subroutine WILL add a 0 to the buffer end though
; ------
gets:
  ;; Loop while 0x0a is received or we loop back to y=0
  ldy #$00        ; Load buffer write index as 0

.loop:
  jsr getc        ; Wait to get a character
  ldx #00
  jsr putc        ; Echo the character received
  cmp #$0a        ; Check if its newline
  beq .done       ; If it is, we are done

  sta (STRPTR),y  ; If not, store it in the buffer at the curerent write index
  iny             ; Increment write index
  beq .loopback   ; If we loopback, then handle it
  jmp .loop       ; Otherwise get next character

.loopback:
  dey             ; On loopback, go to the end of the buffer

.done:
  lda #$00        ; Add a 0 to make it printable
  sta (STRPTR),y  ; Store the 0 in the buffer at the last index
  tya             ; Transfer the last index (=length) to A
  rts             ; Return

; ------
; subroutine putc:
; params:
;   A: byte to send to uart
;   X: output mode:-
;       #00: Text mode
;       #01: VGA mode (terminal buffer shall be populated) (TO IMPLEMENT)
; Communicates with the uart and the emulator via registers
; ------
putc:
  pha

  ; Wait while b0 is set
.wait:
  lda IXFLAGREG
  and #%00000001
  bne .wait

  ; Store and set b0
  pla
  sta UARTOUTREG
  pha
  lda IXFLAGREG
  ora #$00000001        ; Set b0 => Emulator will output the character
  sta IXFLAGREG
  pla

  rts

; ------
; subroutine getc:
; params: None
; desc: Waits for input till keypress is detected
; and returns with the byte in A register
; ------
getc:
  lda IXFLAGREG ; Send getc request
  ora #%00000100
  sta IXFLAGREG

.wait:
  lda IXFLAGREG
  and #%00000010  ; Wait till b1 is not set
  beq .wait

  ; Echo the character received
  lda UARTSHADOW
  pha

  ; Reset the request and uart_wait flag
  lda IXFLAGREG
  and #%11111001
  sta IXFLAGREG

  ; Restore the character read, renable interrupts and exit
  pla
  cli
  rts


; ------
; Bios messages
; ------
msg_bios:       .asciiz "Hello from BIOS", $0d, $0a
msg_nomain:     .asciiz "No _main found, exiting", $0d, $0a
msg_exit_fail:  .asciiz "Failed to exit, hanging...", $0d, $0a


; ------
; Non-Maskable Interrupt hander
; ------
nmi:
  rti

; ------
; irq handler:
; params: None
; desc: Loads into A register the byte received in UARTINREG and sets ixflagreg's b1 to
; indicate that the byte has been read
; ------
irq:
  pha
  txa
  pha
  tya
  pha

  lda UARTINREG
  sta UARTSHADOW
  lda IXFLAGREG
  ora #%00000010  ; Indicate that UARTSHADOW has new bit
  sta IXFLAGREG

  pla
  tay
  pla
  tax
  pla

  rti             ; Return from interrupt


; ------
; Emulator required metadata
; ------
  .org $ffea
uart_inreg_addr:
  .word UARTINREG  ; $ffea - $ffeb
uart_outreg_addr:
  .word UARTOUTREG ; $ffec - $ffed
ixflagreg_addr:
  .word IXFLAGREG   ; $ffee - $ffef

; ------
; Vector table
; ------
  .org $fffa
nmiv:
  .word nmi         ; $fffa - $fffb
resetv:
  .word reset       ; $fffc - $fffd
irqv:
  .word irq         ; $fffe - $ffff
