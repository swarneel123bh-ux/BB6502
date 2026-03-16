; ------
; KERNEL
; Loaded from floppy into RAM at $2000 by bootloader
; ------

.include "vars.s"

; ----------------------------------------
; IMPORTS from bios — resolved at link time, no hardcoded addresses
; ----------------------------------------
.import puts
.import hang
.import exit
.import gets

; ----------------------------------------
; EXPORTS
; ----------------------------------------
.export _kernel

; ----------------------------------------
; CODE SEGMENT
; ----------------------------------------
.segment "KERNEL"

_kernel:
  lda #<msg_hello_kernel
  sta STRPTR
  lda #>msg_hello_kernel
  sta STRPTR+1
  jsr puts

@shell:
  jsr kernel_getcmd
  jsr kernel_dispatch_cmd
  jmp @shell


; ------------------------------
; Subroutine kernel_getcmd:
; In: None
; Out: [STRPTR+1/STRPTR] -> String input as cmd
; ------------------------------
kernel_getcmd:
  ; Print prompt
  lda #<msg_shell_prompt
  sta STRPTR
  lda #>msg_shell_prompt
  sta STRPTR+1
  jsr puts
  ; Get string
  lda #<kernel_ipbuf
  sta STRPTR
  lda #>kernel_ipbuf
  sta STRPTR+1
  jsr gets
  ; Input is in [STRPTR+1/STRPTR]
  rts


; ------------------------------
; Subroutine kernel_cmp_str:
; In: [STRPTR+1/STRPTR] -> Comparision Src,
;     [CMPPTR+1/CMPPTR] -> Comparision Dest
; Out: A register -> 0 if same else 
;      A register -> 1; X register -> index where first difference occurs
; ------------------------------
kernel_cmp_str:
  tya
  pha
  ldy #$00
@loop:
  lda (STRPTR),y
  beq @str1_end
  cmp (CMPPTR),y
  bne @not_same
  iny
  jmp @loop
@str1_end:
  cmp (CMPPTR),y
  bne @not_same
  pla
  tay
  lda #0
  rts
@not_same:
  tya
  tax
  pla 
  tay
  lda #1
  rts
  

; -----------------------------
; Subroutine kernel_dispatch_cmd:
; In: [STRPTR+1/STRPTR] -> cmd string
; Out: None
; -----------------------------
kernel_dispatch_cmd:
  pha                   ; Store this value because we modify it over and over again
  tya
  pha

  ; Load the base addr of the cmds table
  lda #<cmd_table_base
  sta JMPPTR
  lda #>cmd_table_base
  sta JMPPTR+1

@next_cmd:
  ldy #$00
  lda (JMPPTR),y
  sta CMPPTR
  iny
  lda (JMPPTR),y
  sta CMPPTR+1
  jsr kernel_cmp_str
  cmp #$00
  beq @match  ; Match found, jump to match routine

  ; No match, advance JMPPTR
  lda JMPPTR      ; Increment the [JMPPTR+1/JMPPTR] pointer by 4
  clc
  adc #$04        ; Increment by 4 because string vector is 2 bytes wide 
                  ; followed by subroutine vector which is also 2 bytes wide
  sta JMPPTR
  bne @not_new_page ; If we dint load a 00, that means we are in the same page
  lda JMPPTR+1
  clc
  adc #$01        ; Only add 1 to high byte because we only increment the page once
  sta JMPPTR+1
@not_new_page:
  ; Check if we are at the end of the table
  ; Note, strings are never defined at $0000 so we
  ; Can exit when we load that addr as a string vector
  ldy #$00
  lda (JMPPTR),y
  beq @at_end_low   ; If 0, check high byte too
  jmp @next_cmd     ; Not 0, go for next cmd
@at_end_low:
  iny
  lda (JMPPTR),y
  beq @at_end_high  ; If 0, we are at the end, exit
  jmp @next_cmd     ; Not 0, go for next cmd
@at_end_high:
  ; Display message before exiting
  lda #<msg_invalid_cmd
  sta STRPTR
  lda #>msg_invalid_cmd
  sta STRPTR+1
  jsr puts
  pla               ; Restore Y register
  tay
  pla               ; Restore A register
  rts
  
; A string matched with what was in [STRPTR+1/STRPTR]
; jmp to the subroutine at address loaded from [JMPPTR+1/JMPPTR]+#$02
@match:
  ldy #$02
  lda (JMPPTR),y    ; read lo byte of subroutine addr from table[+2]
  sta CMPPTR
  iny
  lda (JMPPTR),y    ; read hi byte of subroutine addr from table[+3]
  sta CMPPTR+1

  lda #>(@return - 1)
  pha
  lda #<(@return - 1)
  pha
  jmp (CMPPTR)      ; NOW correctly jumps to kernel_show_help / kernel_exit
                    
@return:
  pla               ; Restore old Y register value
  tay
  pla               ; Restore old A register value
  rts  
  
; ----------------------------------------
; Print the help string
; ----------------------------------------
kernel_show_help:
  lda #<msg_help
  sta STRPTR
  lda #>msg_help
  sta STRPTR+1
  jsr puts
  rts


; ----------------------------------------
; Exit the emulator (or shut down)
; ----------------------------------------
kernel_exit:
  jsr exit
  rts

; -------------------------------------------------------------------------------------
; KERNELBSS
; -------------------------------------------------------------------------------------
.segment "KERNELBSS"
kernel_ipbuf:
  .res 256


; -------------------------------------------------------------------------------------
; RODATA
; -------------------------------------------------------------------------------------
.segment "KERNELRODATA"

; ------
; Command strings
; ------
cmd_help:
  .byte "help", $00
cmd_exit:
  .byte "exit", $00

; ----------
; Messages
; ----------
msg_shell_prompt:
  .byte ">>> ", $00
msg_hello_kernel:
  .byte "Hello from kernel", $0d, $0a, $00
msg_invalid_cmd:
  .byte "Invalid command", $0d, $0a, $00
msg_help:
  .byte "Help:", $0d, $0a, "1. help: displays this help", $0d, $0a, $00


; ------
; Commands table
; Structure: 
;   String vector(2 bytes), Subroutine Vector(2 bytes)
; ------
cmd_table_base:
  ; String vec    Subroutine vec
  .word cmd_help, kernel_show_help
  .word cmd_exit, kernel_exit
  .word $0000,    $0000             ; end-of-table sentinel

