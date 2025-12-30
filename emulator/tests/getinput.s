; Buffer metadatas
STDOUTPTR     = $6000     ; write index
STDOUTBUFSIZ  = $6002     ; number of bytes written
STDINREADPTR  = $6004     ; Read address for stdin
STDINWRITEPTR = $6006     ; Write address for stdin
STDINBUFSIZ   = $6008     ; Size of the stdin buffer

; Buffers
STDOUTBUF     = $6100     ; stdout buffer
STDINBUF      = $6200     ; stdin buffer

; Zero Page String Pointers
STRPTR        = $00      ; zero-page pointer (2 bytes)


; -------------------------
; gets: copy null-terminated
; string into STDINBUF
; -------------------------
gets:
  ; Set the stdinbufflag to 1  
  ; Wait till same flag is set to 0 by emulator again
  ; Assume that the buf size and pointer addresses are reset properly by the caller
  ; Once set, return
  lda $fff8
  ora #$01
  sta $fff8

gets_wait:
  lda $fff8
  beq gets_done
  jmp gets_wait

gets_done:
  rts



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

stdinbufaddr:
  .word STDINBUF      ; 0xFFF4 - 0xFFF5
stdinbufsizaddr:
  .word STDINBUFSIZ   ; 0xFFF6 - 0xFFF7
stdinbufflag:
  .byte 0             ; 0xFFF8
  .byte 0             ; 0xFFF9



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
