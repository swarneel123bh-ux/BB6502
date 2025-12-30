; -------------------------
; Emulator-visible metadata
; -------------------------
  .org $FFEA

uartinaddr:
  .word UARTIN        ; 0xFFEA - 0xFFEB

uartoutaddr:
  .word UARTOUT       ; 0xFFEC - 0xFFED

stdoutbufaddr:
  .word STDOUTBUF     ; 0xFFEE - 0xFFEF

stdoutbufsizaddr:
  .word STDOUTBUFSIZ  ; 0xFFF0 - 0xFFF1

stdinbufaddr:
  .word STDINBUF      ; 0xFFF2 - 0xFFF3

stdinbufsizaddr:
  .word STDINBUFSIZ   ; 0xFFF4 - 0xFFF5


; Flags for emulator, not needed for hardware (i think?)
stdout_flushbuf_flag:             ; Set => Emulator will flush STDOUT
  .byte 0             ; 0xFFF6
stdin_get_data_into_buf_flag:     ; Set => Emulator will write in STDIN
  .byte 0             ; 0xFFF7
uart_out_flag:                    ; Set => Emulator will read UART
  .byte 0             ; 0xFFF8
uart_in_flag:                     ; Set => Emulator will write to UART
  .byte 0             ; 0xFFF9

