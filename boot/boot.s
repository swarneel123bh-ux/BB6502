
; Memory map as follows : -
;   $0000 - $00FF : Zero Page
;   $0100 - $01FF : Stack
;   $0200 - $7FFF : RAM (See Ram break-up)
;   $8000 - $FFFF : ROM (Has the vectors)

; RAM composition : -
;   $0200 - $03FF :       Unused
;   $0400 - $1FFF : Kernel
;   $2000 - $2008 : Buffer Meta Data
;   $2009 - $20FF :       Unused
;   $2100 - $21FF : Standard Out Buffer
;   $2200 - $22FF : Standard In Buffer


  .org $8000

; Reset 
reset:
  sei 
  cld
  ldx #$FF
  txs
  lda #$00
  tay

  lda #<kernel_message
  sta STRPTR
  lda #>kernel_message
  sta STRPTR+1
  jsr puts

  jmp load_kernel_loop


hang:
  jmp hang



; Copy kernel (256 bytes max for now) from ROM to RAM
; Right now, there is no kernel so it simply jumps to $0400
load_kernel_loop:
  jmp load_kernel_done
  lda kernel_rom,y
  sta $0400,y
  iny
  bne load_kernel_loop

load_kernel_done:
; Jump to kernel
  jmp $0400

kernel_message: .asciiz "Loading kernel!"

  .include bios.s


