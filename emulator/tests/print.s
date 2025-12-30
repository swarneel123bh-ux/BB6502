STDOUTPTR     = $6000     ; write index
STDOUTBUFSIZ  = $6002     ; number of bytes written
STDINREADPTR  = $6004     ; Read address for stdin
STDINWRITEPTR = $6006     ; Write address for stdin
STDOUTBUF     = $6100     ; stdout buffer
STDINBUF      = $6200     ; stdin buffer

STRPTR        = $00      ; zero-page pointer (2 bytes)

; -------------------------
; Reset entry
; -------------------------
reset:
  lda #<message
  sta STRPTR
  lda #>message
  sta STRPTR+1

  jsr puts

hang:
  jmp hang

; -------------------------
; puts: copy null-terminated
; string into STDOUTBUF
; -------------------------
puts:
  ldy #0                  ; Y = string index

puts_loop:
  lda (STRPTR),y            ; read char
  beq puts_done           ; stop on NULL

  ldx STDOUTPTR           ; X = buffer index
  sta STDOUTBUF,x         ; write char

  inx
  stx STDOUTPTR           ; advance write pointer
  inc STDOUTBUFSIZ        ; increase size

  iny
  jmp puts_loop

puts_done:
  lda $fff2
  ora #$1
  sta $fff2
  rts

; -------------------------
; Data
; -------------------------
message: .asciiz "Hello World!"

; -------------------------
; Emulator-visible metadata
; -------------------------
  .org $FFEE
stdoutbufaddr:
  .word STDOUTBUF     ; 0xFFEE - 0xFFEF
stdoutbufsizaddr:
  .word STDOUTBUFSIZ  ; 0xFFF0 - 0xFFF1
stdoutbufflush:
  .byte 0             ; 0xFFF2
  .byte 0             ; 0xFFF3 (unused)


; -------------------------
; Vectors
; -------------------------
  .org $FFFA
nmi:
  .word $0000         ; 0xFFFA - 0xFFFB (unused)
resetv:
  .word reset         ; 0xFFFC - 0xFFFC
irq:
  .word $0000         ; 0xFFFD - 0xFFFF (unused)
