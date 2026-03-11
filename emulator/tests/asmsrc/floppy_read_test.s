
  .include vars.s
  .dsb $2000, 0
  .org $2000

_main:
  ; Print prompt
  lda #<msg1
  sta STRPTR
  lda #>msg1
  sta STRPTR+1
  jsr puts

  ; Set floppy stuff
  ; Set LBA
  lda $00 ; LBA Addr 0
  tay

  ; Set DMA
  lda #$00 ; Write data to $3000
  sta FLPDMAREG
  lda #$30
  sta FLPDMAREG+1

  ; Set number of sectors to read
  lda 1 ; Read 1 sector
  jsr floppy_read


  ; Print prompt 2
  lda #<msg2
  sta STRPTR
  lda #>msg2
  sta STRPTR+1
  jsr puts

  ; Return control so we can inspect what was read into memory
  jsr retctrl

  ; Exit
  jmp exit
  
msg1: .asciiz "Reading floppy"
msg2: .asciiz "Read done"
buf:  .dsb $ff, 0
_main_end:

  .org $8000
  .include bios.s
