  .include vars.s
  .include wozvars.s
  .dsb $2000, 0
  .org $2000

_main:

	lda #<wozmsg
	sta STRPTR
	lda #>wozmsg
	sta STRPTR+1
	jsr puts

	jmp _wozmon

  jmp exit

_main_end:

; messages
wozmsg: .asciiz "Jumping to wozmon", $0D, $0A, 0

_wozmon:
	.include mywozmon.s
_wozmon_end:

; bios
  .org $8000
  .include bios.s
