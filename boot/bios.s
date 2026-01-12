; -------------------------
; gets: copy null-terminated
; string into STDINBUF
; -------------------------
gets:
  ; Set the stdinbufflag to 1  
  ; Wait till same flag is set to 0 by emulator again
  ; Assume that the buf size and pointer addresses are reset properly by the caller
  ; Once set, return
  lda stdin_get_data_into_buf_flag
  ora #$01
  sta stdin_get_data_into_buf_flag

gets_wait:
  lda stdin_get_data_into_buf_flag
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
  lda stdout_flushbuf_flag
  ora #$1
  sta stdout_flushbuf_flag
  rts
