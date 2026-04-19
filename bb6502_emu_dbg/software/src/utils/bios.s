; ============================================================
; bios.s — BIOS code, assembled into the ROM segment
; ============================================================

.ifndef BIOS_INCLUDED
BIOS_INCLUDED = 1

.include "vars.s"

; ----------------------------------------
; EXPORTS
; ----------------------------------------
.export reset
.export puts, gets, putc, getc
.export putsg, getsg, putcg, getcg
.export hang, exit
.export floppy_read, floppy_write
.export nmi, irq

; ----------------------------------------
; CODE SEGMENT
; ----------------------------------------
.segment "BIOS"

; ============================================================
; RESET — cold-start entry point
; ============================================================
reset:
    sei
    cld
    ldx #$FF
    txs

    ; Initialize keyboard / floppy state
    lda #$00
    sta KBD_LAST
    sta FLOPPY_DONE

    ; Initialise cursor position to (0,0)
    sta DISPGFX_CURS_ROW
    sta DISPGFX_CURS_COL

    ; Initialise VRAM write pointer → start of VRAM
    lda #<DISPGFX_VRAM_BASE
    sta DISPGFX_VRAM_WPTR
    lda #>DISPGFX_VRAM_BASE
    sta DISPGFX_VRAM_WPTR+1

    ; Initialise CRAM write pointer → start of CRAM
    lda #<DISPGFX_CRAM_BASE
    sta DISPGFX_CRAM_WPTR
    lda #>DISPGFX_CRAM_BASE
    sta DISPGFX_CRAM_WPTR+1

    ; ── Tell emulator where VRAM lives ────────────────────────
    lda #<DISPGFX_VRAM_BASE
    sta DISPGFX_DATA_REG
    lda #>DISPGFX_VRAM_BASE
    sta DISPGFX_DATA_REG+1
    lda #DISPGFX_CMD_SET_VRAM       ; NOTE: #immediate
    sta DISPGFX_CMD_REG
    jsr dispgfx_wait_idle

    ; ── Tell emulator where CRAM lives ────────────────────────
    lda #<DISPGFX_CRAM_BASE
    sta DISPGFX_DATA_REG
    lda #>DISPGFX_CRAM_BASE
    sta DISPGFX_DATA_REG+1
    lda #DISPGFX_CMD_SET_CRAM
    sta DISPGFX_CMD_REG
    jsr dispgfx_wait_idle

    ; ── Clear screen ──────────────────────────────────────────
    lda #DISPGFX_CMD_CLEAR
    sta DISPGFX_CMD_REG
    jsr dispgfx_wait_idle

    ; ── Set hardware cursor at (0,0) ──────────────────────────
    lda #$00
    sta DISPGFX_DATA_REG            ; col = 0
    sta DISPGFX_DATA_REG+1          ; row = 0
    lda #DISPGFX_CMD_SET_CURSOR
    sta DISPGFX_CMD_REG
    jsr dispgfx_wait_idle

    ; ── Enable blinking cursor ────────────────────────────────
    lda #DISPGFX_CMD_CURSOR_ON
    sta DISPGFX_CMD_REG
    jsr dispgfx_wait_idle

    ; ── Print BIOS banner on graphical display ────────────────
    lda #<msg_bios
    sta STRPTR
    lda #>msg_bios
    sta STRPTR+1
    jsr putsg

    ; ── Boot the kernel from floppy ───────────────────────────
    jsr _bootloader         ; load + jump to kernel; only returns on error

    lda #<msg_nokernel
    sta STRPTR
    lda #>msg_nokernel
    sta STRPTR+1
    jsr putsg
    jmp hang


; ============================================================
; hang — halt forever
; ============================================================
hang:
    lda #<msg_hanging
    sta STRPTR
    lda #>msg_hanging
    sta STRPTR+1
    jsr putsg
@loop:
    jmp @loop


; ============================================================
; exit — shutdown (alias to hang in current protocol)
; ============================================================
exit:
    jmp hang


; ============================================================
; dispgfx_wait_idle — spin until worker has consumed the command
;   Waits until CMD register reads NOP ($00) AND STATUS has IDLE.
;   Same pattern as _floppy_wait_cmd — prevents the race where
;   the BIOS sees stale IDLE before the worker has woken up.
; ============================================================
dispgfx_wait_idle:
    lda DISPGFX_CMD_REG
    bne dispgfx_wait_idle       ; spin until CMD = NOP ($00)
    lda DISPGFX_STATUS_REG
    and #DISPGFX_STATUS_IDLE
    beq dispgfx_wait_idle       ; also ensure IDLE is set
    rts


; ============================================================
; dispgfx_update_hw_cursor — send SET_CURSOR command to emulator
;   Uses DISPGFX_CURS_COL / DISPGFX_CURS_ROW shadow registers.
; ============================================================
dispgfx_update_hw_cursor:
    jsr dispgfx_wait_idle
    lda DISPGFX_CURS_COL
    sta DISPGFX_DATA_REG        ; DATA low = col
    lda DISPGFX_CURS_ROW
    sta DISPGFX_DATA_REG+1      ; DATA high = row
    lda #DISPGFX_CMD_SET_CURSOR
    sta DISPGFX_CMD_REG
    jsr dispgfx_wait_idle
    rts


; ============================================================
; putcg — write one character to the graphical display
;
; In:  A = character (ASCII)
; Out: A clobbered; Y preserved
;
; Handles printable chars, CR ($0D), LF ($0A), backspace ($08).
; Wraps at column 40, scrolls at row 30.
; Does NOT update the hardware cursor — putsg / getsg do that
; once after a full string/line for efficiency.
; ============================================================
putcg:
    sta DISPGFX_VRAM_SHADOW     ; save character

    ; ── Preserve Y ─────────────────────────────────────────────
    tya
    pha

    lda DISPGFX_VRAM_SHADOW

    ; ── Check special characters ───────────────────────────────
    cmp #$0D
    beq @newline
    cmp #$0A
    beq @newline
    cmp #$08
    beq @backspace

    ; ── Regular printable character ────────────────────────────
    ldy #$00
    sta (DISPGFX_VRAM_WPTR),y  ; write char to VRAM at write-pointer

    ; Advance VRAM write pointer by 1
    inc DISPGFX_VRAM_WPTR
    bne :+
    inc DISPGFX_VRAM_WPTR+1
:

    ; Advance cursor column
    inc DISPGFX_CURS_COL
    lda DISPGFX_CURS_COL
    cmp #DISPGFX_COLS
    bcc @done                   ; col < 40 → no wrap needed

    ; ── Wrap to next line ──────────────────────────────────────
    lda #$00
    sta DISPGFX_CURS_COL
    jmp @advance_row

@newline:
    ; Advance WPTR by (COLS - current_col) to reach the next row boundary
    sec
    lda #DISPGFX_COLS
    sbc DISPGFX_CURS_COL        ; A = bytes to skip
    clc
    adc DISPGFX_VRAM_WPTR
    sta DISPGFX_VRAM_WPTR
    bcc :+
    inc DISPGFX_VRAM_WPTR+1
:
    ; Also advance CRAM pointer by the same amount
    sec
    lda #DISPGFX_COLS
    sbc DISPGFX_CURS_COL
    clc
    adc DISPGFX_CRAM_WPTR
    sta DISPGFX_CRAM_WPTR
    bcc :+
    inc DISPGFX_CRAM_WPTR+1
:
    lda #$00
    sta DISPGFX_CURS_COL

@advance_row:
    inc DISPGFX_CURS_ROW
    lda DISPGFX_CURS_ROW
    cmp #DISPGFX_ROWS
    bcc @done                   ; row < 30 → ok
    jsr _scroll_up
    jmp @done

@backspace:
    ; Ignore if already at column 0
    lda DISPGFX_CURS_COL
    beq @done

    dec DISPGFX_CURS_COL

    ; Decrement VRAM write pointer
    lda DISPGFX_VRAM_WPTR
    bne :+
    dec DISPGFX_VRAM_WPTR+1
:   dec DISPGFX_VRAM_WPTR

    ; Write space at the (now current) position
    lda #$20
    ldy #$00
    sta (DISPGFX_VRAM_WPTR),y
    jmp @done

@done:
    ; ── Restore Y ──────────────────────────────────────────────
    pla
    tay
    rts


; ============================================================
; _scroll_up — shift the entire screen up by one row
;
; Copies VRAM[row 1..29] → VRAM[row 0..28]  (1160 bytes)
; Clears VRAM row 29 (40 bytes → spaces)
; Does the same for CRAM (colour attributes).
; Sets cursor row = 29, recomputes VRAM/CRAM write pointers.
;
; Clobbers: A, X, Y.  Saves/restores STRPTR and CMPPTR.
; ============================================================
_scroll_up:
    ; ── Save STRPTR and CMPPTR (caller may be using them) ─────
    lda STRPTR
    pha
    lda STRPTR+1
    pha
    lda CMPPTR
    pha
    lda CMPPTR+1
    pha

    ; ============ Scroll VRAM ============
    ; Source = VRAM_BASE + 40 (row 1)
    lda #<(DISPGFX_VRAM_BASE + DISPGFX_COLS)
    sta STRPTR
    lda #>(DISPGFX_VRAM_BASE + DISPGFX_COLS)
    sta STRPTR+1
    ; Dest = VRAM_BASE (row 0)
    lda #<DISPGFX_VRAM_BASE
    sta CMPPTR
    lda #>DISPGFX_VRAM_BASE
    sta CMPPTR+1

    ; Copy 1160 bytes = 4 full pages (1024) + 136 remaining
    ldx #$04
@vram_page:
    ldy #$00
@vram_byte:
    lda (STRPTR),y
    sta (CMPPTR),y
    iny
    bne @vram_byte
    inc STRPTR+1
    inc CMPPTR+1
    dex
    bne @vram_page

    ; Copy remaining 136 bytes
    ldy #$00
@vram_tail:
    cpy #136
    beq @vram_clear_last
    lda (STRPTR),y
    sta (CMPPTR),y
    iny
    bne @vram_tail              ; always taken (Y < 136 < 256)

@vram_clear_last:
    ; CMPPTR now points 1024 bytes past VRAM_BASE.
    ; Last row starts at VRAM_BASE + 1160 = VRAM_BASE + 1024 + 136.
    ; Advance CMPPTR by 136 to reach it.
    clc
    lda CMPPTR
    adc #136
    sta CMPPTR
    lda CMPPTR+1
    adc #$00
    sta CMPPTR+1

    lda #$20                    ; space
    ldy #$00
@vram_clr:
    sta (CMPPTR),y
    iny
    cpy #DISPGFX_COLS
    bne @vram_clr

    ; ============ Scroll CRAM ============
    lda #<(DISPGFX_CRAM_BASE + DISPGFX_COLS)
    sta STRPTR
    lda #>(DISPGFX_CRAM_BASE + DISPGFX_COLS)
    sta STRPTR+1
    lda #<DISPGFX_CRAM_BASE
    sta CMPPTR
    lda #>DISPGFX_CRAM_BASE
    sta CMPPTR+1

    ldx #$04
@cram_page:
    ldy #$00
@cram_byte:
    lda (STRPTR),y
    sta (CMPPTR),y
    iny
    bne @cram_byte
    inc STRPTR+1
    inc CMPPTR+1
    dex
    bne @cram_page

    ldy #$00
@cram_tail:
    cpy #136
    beq @cram_clear_last
    lda (STRPTR),y
    sta (CMPPTR),y
    iny
    bne @cram_tail

@cram_clear_last:
    clc
    lda CMPPTR
    adc #136
    sta CMPPTR
    lda CMPPTR+1
    adc #$00
    sta CMPPTR+1

    lda #DISPGFX_DEFAULT_ATTR   ; default colour
    ldy #$00
@cram_clr:
    sta (CMPPTR),y
    iny
    cpy #DISPGFX_COLS
    bne @cram_clr

    ; ── Restore STRPTR / CMPPTR ───────────────────────────────
    pla
    sta CMPPTR+1
    pla
    sta CMPPTR
    pla
    sta STRPTR+1
    pla
    sta STRPTR

    ; ── Row = 29, col unchanged.  Recompute write pointers. ──
    lda #(DISPGFX_ROWS - 1)
    sta DISPGFX_CURS_ROW

    ; VRAM_WPTR = VRAM_BASE + 29*40 + col
    ;   29*40 = 1160 = $0488
    clc
    lda #<(DISPGFX_VRAM_BASE + 1160)
    adc DISPGFX_CURS_COL
    sta DISPGFX_VRAM_WPTR
    lda #>(DISPGFX_VRAM_BASE + 1160)
    adc #$00
    sta DISPGFX_VRAM_WPTR+1

    ; CRAM_WPTR = CRAM_BASE + 29*40 + col
    clc
    lda #<(DISPGFX_CRAM_BASE + 1160)
    adc DISPGFX_CURS_COL
    sta DISPGFX_CRAM_WPTR
    lda #>(DISPGFX_CRAM_BASE + 1160)
    adc #$00
    sta DISPGFX_CRAM_WPTR+1

    rts


; ============================================================
; putsg — print a null-terminated string on the graphical display
; In:  STRPTR/STRPTR+1 = address of string
; Out: A, Y clobbered; updates hardware cursor at end
; ============================================================
putsg:
    ldy #$00
@loop:
    lda (STRPTR),y
    beq @done
    jsr putcg                   ; preserves Y
    iny
    bne @loop
@done:
    jsr dispgfx_update_hw_cursor
    rts


; ============================================================
; putcg already serves as 'putc' for the graphical display.
; For the serial/text console, the original putc is still here.
; ============================================================

; ============================================================
; putc — send one character to the text display and wait for ACK
; In:  A = character
; Out: A = 0 (clobbered)
; ============================================================
putc:
    sta DISPTEXT_DATA_REG
@wait:
    lda DISPTEXT_DATA_REG
    bne @wait               ; worker writes 0 back when consumed
    rts


; ============================================================
; puts — print a null-terminated string to the text display
; In:  STRPTR/STRPTR+1 = address of string
; Out: A, Y clobbered
; ============================================================
puts:
    ldy #$00
@loop:
    lda (STRPTR),y
    beq @done
    jsr putc
    iny
    bne @loop
@done:
    rts


; ============================================================
; getcg — block until a key is available, return it in A (no echo)
; In:  (none)
; Out: A = ASCII key; KBD_LAST cleared; X clobbered
; ============================================================
getcg:
    cli                     ; enable IRQs — keyboard is IRQ-driven
@wait:
    lda KBD_LAST
    beq @wait               ; spin until IRQ handler posts a byte
    ldx #$00
    stx KBD_LAST            ; consume
    rts


; ============================================================
; getc — block until a key is available, return it in A
; In:  (none)
; Out: A = ASCII key; KBD_LAST cleared; X clobbered
; ============================================================
getc:
    cli
@wait:
    lda KBD_LAST
    beq @wait
    ldx #$00
    stx KBD_LAST
    rts


; ============================================================
; getsg — read a line into a buffer and echo on graphical display
; In:  STRPTR/STRPTR+1 = destination buffer address
; Out: A = length (not counting null); buffer null-terminated
; ============================================================
getsg:
    ldy #$00
@loop:
    jsr getcg                ; A = typed character
    pha                     ; save character — putcg clobbers A
    jsr putcg                ; echo it (Y preserved by putcg)
    pla                     ; restore character
    cmp #$0A                ; newline?
    beq @done
    cmp #$0D                ; carriage return?
    beq @done
    sta (STRPTR),y          ; store character in buffer
    iny
    beq @overflow           ; Y wrapped: buffer full (256 bytes)
    jmp @loop
@overflow:
    dey                     ; back off so null lands at [255]
@done:
    lda #$00
    sta (STRPTR),y          ; null-terminate
    tya                     ; A = length
    pha                     ; save length
    jsr dispgfx_update_hw_cursor   ; update blinking cursor after input
    pla                     ; restore length into A
    rts


; ============================================================
; gets — read a line into a buffer (text display)
; In:  STRPTR/STRPTR+1 = destination buffer address
; Out: A = length (not counting null); buffer null-terminated
; ============================================================
gets:
    ldy #$00
@loop:
    jsr getc
    pha
    jsr putc                ; echo
    pla
    cmp #$0A
    beq @done
    cmp #$0D
    beq @done
    sta (STRPTR),y
    iny
    beq @overflow
    jmp @loop
@overflow:
    dey
@done:
    lda #$00
    sta (STRPTR),y
    tya
    rts


; ============================================================
; _floppy_wait_idle
;   Spins until the floppy controller is not BUSY.
; ============================================================
_floppy_wait_idle:
    lda FLOPPY_STATUS_REG
    and #FLOPPY_STATUS_BUSY
    bne _floppy_wait_idle
    rts


; ============================================================
; _floppy_wait_cmd
;   Spins until CMD = NO_CMD AND BUSY is clear.
; ============================================================
_floppy_wait_cmd:
    lda FLOPPY_CMD_REG
    bne _floppy_wait_cmd
    lda FLOPPY_STATUS_REG
    and #FLOPPY_STATUS_BUSY
    bne _floppy_wait_cmd
    rts


; ============================================================
; floppy_read — load N sectors from floppy into RAM
;
; In:  A       = number of sectors to read (1–255)
;      Y       = starting LBA sector number (0–255)
;      STRPTR  = 16-bit DMA destination address
;
; Out: Carry clear on success, Carry set on error.
;      A, X, Y clobbered.  STRPTR advanced past last byte loaded.
; ============================================================
floppy_read:
    sta CMPPTR
    sty CMPPTR+1

    jsr _floppy_wait_idle
    lda #FLOPPY_CMD_RESET
    sta FLOPPY_CMD_REG
    jsr _floppy_wait_cmd

@read_loop:
    lda STRPTR
    sta FLOPPY_DATA_REG
    lda STRPTR+1
    sta FLOPPY_DATA_REG+1
    lda #FLOPPY_CMD_SET_DMA_ADDR
    sta FLOPPY_CMD_REG
    jsr _floppy_wait_cmd

    lda CMPPTR+1
    sta FLOPPY_DATA_REG
    lda #FLOPPY_CMD_STORE_LBA
    sta FLOPPY_CMD_REG
    jsr _floppy_wait_cmd

    lda #$00
    sta FLOPPY_DONE
    cli
    lda #FLOPPY_CMD_READ_SECTOR
    sta FLOPPY_CMD_REG
@read_wait_irq:
    lda FLOPPY_DONE
    beq @read_wait_irq
    sei

    lda FLOPPY_STATUS_REG
    and #FLOPPY_STATUS_ERROR
    bne @read_error

    clc
    lda STRPTR
    adc #$00
    sta STRPTR
    lda STRPTR+1
    adc #$02
    sta STRPTR+1

    inc CMPPTR+1
    dec CMPPTR
    bne @read_loop

    clc
    rts

@read_error:
    sec
    rts


; ============================================================
; floppy_write — write N sectors from RAM to floppy
;
; In:  A       = number of sectors to write (1–255)
;      Y       = starting LBA sector number (0–255)
;      STRPTR  = 16-bit DMA source address
;
; Out: Carry clear on success, Carry set on error.
; ============================================================
floppy_write:
    sta CMPPTR
    sty CMPPTR+1

    jsr _floppy_wait_idle
    lda #FLOPPY_CMD_RESET
    sta FLOPPY_CMD_REG
    jsr _floppy_wait_cmd

@write_loop:
    lda STRPTR
    sta FLOPPY_DATA_REG
    lda STRPTR+1
    sta FLOPPY_DATA_REG+1
    lda #FLOPPY_CMD_SET_DMA_ADDR
    sta FLOPPY_CMD_REG
    jsr _floppy_wait_cmd

    lda CMPPTR+1
    sta FLOPPY_DATA_REG
    lda #FLOPPY_CMD_STORE_LBA
    sta FLOPPY_CMD_REG
    jsr _floppy_wait_cmd

    lda #$00
    sta FLOPPY_DONE
    cli
    lda #FLOPPY_CMD_WRITE_SECTOR
    sta FLOPPY_CMD_REG
@write_wait_irq:
    lda FLOPPY_DONE
    beq @write_wait_irq
    sei

    lda FLOPPY_STATUS_REG
    and #FLOPPY_STATUS_ERROR
    bne @write_error

    clc
    lda STRPTR
    adc #$00
    sta STRPTR
    lda STRPTR+1
    adc #$02
    sta STRPTR+1

    inc CMPPTR+1
    dec CMPPTR
    bne @write_loop

    clc
    rts

@write_error:
    sec
    rts


; ============================================================
; IRQ handler
; ============================================================
irq:
    pha
    txa
    pha
    tya
    pha

    ; ── Keyboard ──────────────────────────────────────────────
    lda KBD_DATA_REG
    beq @check_floppy
    sta KBD_LAST
    lda #$00
    sta KBD_DATA_REG        ; ACK

@check_floppy:
    ; ── Floppy ────────────────────────────────────────────────
    lda FLOPPY_STATUS_REG
    and #FLOPPY_STATUS_IRQ
    beq @irq_done
    lda FLOPPY_STATUS_REG
    and #($FF - FLOPPY_STATUS_IRQ)
    sta FLOPPY_STATUS_REG
    lda #$01
    sta FLOPPY_DONE

@irq_done:
    pla
    tay
    pla
    tax
    pla
    rti


; ============================================================
; NMI handler
; ============================================================
nmi:
    rti


; ============================================================
; BIOS strings
; ============================================================
.segment "BIOSRODATA"
msg_bios:
    .byte "BB6502 BIOS v1.0", $0D, $0A, $00
msg_nokernel:
    .byte "No kernel found", $0D, $0A, $00
msg_hanging:
    .byte "Halted", $0D, $0A, $00


; ============================================================
; DEVICE TABLE at $FF00
;
; 16 bytes — eight 16-bit LE pointers to actual device registers.
; The C emulator reads these at startup to learn where devices are:
;
;   $FF00–$FF01  floppy STATUS reg    → $0200
;   $FF02–$FF03  floppy CMD reg       → $0201
;   $FF04–$FF05  floppy DATA reg      → $0202
;   $FF06–$FF07  kbd DATA reg         → $0204
;   $FF08–$FF09  disptext DATA reg    → $0205
;   $FF0A–$FF0B  dispgfx CMD reg      → $0206
;   $FF0C–$FF0D  dispgfx DATA reg     → $0207
;   $FF0E–$FF0F  dispgfx STATUS reg   → $0209
; ============================================================
.segment "DEVTABLE"
    .word FLOPPY_STATUS_REG     ; $FF00
    .word FLOPPY_CMD_REG        ; $FF02
    .word FLOPPY_DATA_REG       ; $FF04
    .word KBD_DATA_REG          ; $FF06
    .word DISPTEXT_DATA_REG     ; $FF08
    .word DISPGFX_CMD_REG       ; $FF0A
    .word DISPGFX_DATA_REG      ; $FF0C
    .word DISPGFX_STATUS_REG    ; $FF0E


; ============================================================
; 6502 VECTORS at $FFFA
; ============================================================
.segment "VECTORS"
    .word nmi               ; $FFFA  NMI
    .word reset             ; $FFFC  RESET
    .word irq               ; $FFFE  IRQ/BRK


.endif
