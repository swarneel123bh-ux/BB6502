; ======================================================================
; kernel.s — BB6502 kernel
; Loaded from floppy into RAM at KERNEL_LOAD_ADDR ($0B6A) by bootloader
; ======================================================================

.include "vars.s"

; ----------------------------------------
; IMPORTS from BIOS — resolved at link time via rom.o
; ----------------------------------------
.import puts, putsg
.import hang
.import exit
.import gets, getsg

; ----------------------------------------
; EXPORTS
; ----------------------------------------
.export _kernel

; ============================================================
; CODE SEGMENT  (starts at KERNEL_LOAD_ADDR = $0B6A)
; ============================================================
.segment "KERNEL"

_kernel:
    lda #<msg_hello_kernel
    sta STRPTR
    lda #>msg_hello_kernel
    sta STRPTR+1
    jsr putsg

@shell:
    jsr kernel_getcmd
    jsr kernel_dispatch_cmd
    jmp @shell


; ----------------------------------------
; kernel_getcmd
; Out: STRPTR → null-terminated input string in kernel_ipbuf
; ----------------------------------------
kernel_getcmd:
    lda #<msg_shell_prompt
    sta STRPTR
    lda #>msg_shell_prompt
    sta STRPTR+1
    jsr putsg

    lda #<kernel_ipbuf
    sta STRPTR
    lda #>kernel_ipbuf
    sta STRPTR+1
    jsr getsg
    rts


; ----------------------------------------
; kernel_cmp_str
; In:  STRPTR → string A,  CMPPTR → string B
; Out: A=0 same, A=1 different; X=first diff index
; ----------------------------------------
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


; ----------------------------------------
; kernel_dispatch_cmd
; In: STRPTR → command string (from kernel_ipbuf)
; ----------------------------------------
kernel_dispatch_cmd:
    pha
    tya
    pha

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
    beq @match

    ; No match — advance JMPPTR by 4 bytes (2 str ptr + 2 fn ptr)
    lda JMPPTR
    clc
    adc #$04
    sta JMPPTR
    bne @not_new_page
    lda JMPPTR+1
    clc
    adc #$01
    sta JMPPTR+1
@not_new_page:
    ; End-of-table sentinel: both bytes of str ptr are $00
    ldy #$00
    lda (JMPPTR),y
    beq @at_end_low
    jmp @next_cmd
@at_end_low:
    iny
    lda (JMPPTR),y
    beq @at_end_high
    jmp @next_cmd
@at_end_high:
    lda #<msg_invalid_cmd
    sta STRPTR
    lda #>msg_invalid_cmd
    sta STRPTR+1
    jsr putsg
    pla
    tay
    pla
    rts

@match:
    ldy #$02
    lda (JMPPTR),y
    sta CMPPTR
    iny
    lda (JMPPTR),y
    sta CMPPTR+1

    lda #>(@return - 1)
    pha
    lda #<(@return - 1)
    pha
    jmp (CMPPTR)

@return:
    pla
    tay
    pla
    rts


; ----------------------------------------
; Built-in commands
; ----------------------------------------
kernel_show_help:
    lda #<msg_help
    sta STRPTR
    lda #>msg_help
    sta STRPTR+1
    jsr putsg
    rts

kernel_exit:
    jsr exit
    rts


; ============================================================
; BSS — uninitialized kernel RAM
; ============================================================
.segment "KERNELBSS"
kernel_ipbuf:   .res 256    ; input line buffer for the shell


; ============================================================
; RODATA
; ============================================================
.segment "KERNELRODATA"

; Command strings
cmd_help:   .byte "help", $00
cmd_exit:   .byte "exit", $00

; Messages
msg_hello_kernel:
    .byte "Hello from kernel", $0D, $0A, $00
msg_shell_prompt:
    .byte ">>> ", $00
msg_invalid_cmd:
    .byte "Invalid command", $0D, $0A, $00
msg_help:
    .byte "Commands: help, exit", $0D, $0A, $00

; Command table: { .word str_ptr, .word fn_ptr } ... sentinel $0000,$0000
cmd_table_base:
    .word cmd_help, kernel_show_help
    .word cmd_exit, kernel_exit
    .word $0000,    $0000
