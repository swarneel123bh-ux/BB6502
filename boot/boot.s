
; -----------------------------------------
; Memory map : -
;   $0000 - $00FF : Zero Page
;   $0100 - $01FF : Stack
;   $0200 - $7FFF : RAM (See Ram break-up)
;   $8000 - $FFFF : ROM (Has the vectors)

; RAM composition : -
;   $0200 - $03FF :       Unused
;   $0400 - $1FFF : Kernel copied to RAM
;   $2000 - $5FFF :       Unused
;   $6000 - $600C : Buffer metadata and Hardware
;   $600D - $60FF :       Unused
;   $6100 - $61FF : Standard Out Buffer
;   $6200 - $62FF : Standard In Buffer
; 
; See addrs.s for more details
; -----------------------------------------



; --------------------------
; Address data (see addrs.s)
; --------------------------
  .include addrs.s



; -------------------------
; Other important addresses
; -------------------------
; Pointers for multiple page copying 
SRC_PTR = $02   ; $02 - $03
DST_PTR = $04   ; $04 - $05



; --------------------------
; ROM STARTS
; --------------------------
  .org $8000



; --------------------------------------------------
; BOOTLOADER CODE
; --------------------------------------------------
reset:
  ; Disable interrupts and BCD
  sei 
  cld
  ldx #$FF
  txs

  ; Loading message
  lda #<loading
  sta STRPTR
  lda #>loading
  sta STRPTR+1
  jsr puts

  lda #$00
  tay
  jsr load_kernel

  ; Loaded Message
  lda #<loaded
  sta STRPTR
  lda #>loaded
  sta STRPTR+1
  jsr puts

  ; Jump to kernel in RAM at address $0400
  ldy #$00
  jmp $0400

  ; Failed message
  lda #<failed
  sta STRPTR
  lda #>failed
  sta STRPTR+1
  jsr puts

bootloader_hang:
  jmp bootloader_hang


; Find out how many pages required to copy the kernel 
kernel_size = kernel_end_ - kernel_start_
kernel_pages = (kernel_size + 256) / 256


; Copy kernel from ROM to RAM
load_kernel:
  lda #<kernel_main
  sta SRC_PTR
  lda #>kernel_main
  sta SRC_PTR+1

  lda #<$0400
  sta DST_PTR
  lda #>$0400
  sta DST_PTR+1

  ldx #kernel_pages
  ldy #$00

.copy_page:
  ldy #$00          ; ensure Y reset each page

.copy_byte:
  lda (SRC_PTR),y
  sta (DST_PTR),y
  iny
  bne .copy_byte

  inc SRC_PTR+1
  inc DST_PTR+1
  dex
  bne .copy_page

  rts


; ---------------------------
; Boot data
; ---------------------------
loading:  .asciiz "Loading kernel!", $0d, $0a
loaded:   .asciiz "Kernel Loaded!", $0d, $0a
failed:   .asciiz "Failed to load kernel", $0d, $0a
 


; -------------------------------------------------------------
; KERNEL CODE
; -------------------------------------------------------------
kernel_start_:
  .include kernel.s
kernel_end_:



; -------------------------------------------------------------
; BIOS CODE
; -------------------------------------------------------------
  .include bios.s



; -------------------------------------------------------------
; VECTORS CODE
; -------------------------------------------------------------
; ------------------------------
; Non Maskable Interrupt Handler
; ------------------------------
nmi:
  rti


; ------------------------------
; Interrupt request handler code
; ------------------------------
irq:
  rti ; For wozmon functionality we dont really need irq at all
  pha
  txa 
  pha

  ; Detect keypresses by reading UARTIN
  ; Put the same byte into UARTOUT
  lda UARTIN
  sta UARTOUT

  ; Set the uart_out_flag so that the emulator can reflect it
  lda uart_out_flag
  ora #$01
  sta uart_out_flag

  pla
  tax
  pla

  rti




; ---------------------------
; Emulator dependant metadata
; ---------------------------
  ; We start here because metadata is growing backwards
  ; From address $fff9
  .org $FFEA  
  .include metadata.s



; ---------------------------
; Vectors
; The meta-data ends at $fff9
; So we can directly write the vector table
; ---------------------------
nmiv:
  .word nmi     ; $fffa - $fffb

resetv:
  .word reset   ; $fffc - $fffd

irqv:
  .word irq     ; $fffe - $ffff












