kernel_main:
  ; Kernel main
  ; Since we can copy only 256 bytes now
  ; We write a little message and fill the rest with 0s

  cli                 ; Renable maskable interrupts
                      ; We need this later for prompts

  ; Print a hello message
  lda #<kernel_hello
  sta STRPTR
  lda #>kernel_hello
  sta STRPTR+1
  jsr puts

  lda #<loading_wozmon
  sta STRPTR
  lda #>loading_wozmon
  sta STRPTR+1
  jsr puts

  ldy #$00
  jmp wozmon

  lda #<wozfail
  sta STRPTR
  lda #>wozfail
  sta STRPTR+1
  jsr puts


kernel_hang:
  jmp kernel_hang



; ----------------------------
; Load into wozmon for now
; ----------------------------
wozmon_start:
  .include wozmon.s
wozmon_end:

wozmon:
  jmp RESET





; ---------------------------
; Kernel data
; ---------------------------
kernel_hello: .asciiz "Hello from kernel!", $0d, $0a
loading_wozmon: .asciiz "Loading Wozmon!", $0d, $0a
wozfail:      .asciiz "Couldnt load wozmon!", $0d, $0a


kernel_end:
  .dsb 512 - (kernel_end - kernel_main), 0

