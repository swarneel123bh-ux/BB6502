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
  jsr hang


; ----------------------------------------
; RODATA
; ----------------------------------------
.segment "KERNELRODATA"
msg_hello_kernel:
  .byte "Hello from kernel", $0d, $0a, $00

