; ============================================================
; vars.s — Global constants, included at the top of every file
; ============================================================
; NOTE: We have a 32K ram, so we can only use this memspace layout for now
; Later, if we get more ram chips, we can move BIOS further down, but we will need
; extra decoding hardware to decode ROM addresses (might invest in a decoder, but too expensive and will be wasteful)
;
; Memory map (post-boot, kernel running)
;
;   $0000–$000F   BIOS zero page  (STRPTR, CMPPTR, JMPPTR, KBD_LAST, FLOPPY_DONE,
;                                   DISPGFX shadow regs, cursor row/col)
;   $0010–$00FF   Free zero page  (240 B — kernel / app ZP variables)
;   $0100–$01FF   Hardware stack  (256 B — fixed by 6502 architecture)
;   $0200–$0209   Device registers (MMIO — 10 bytes, fixed by emulator DEVTABLE)
;   $020A–$06B9   MONITOR VRAM   (1200 B, 40×30 chars)
;   $06BA–$0B69   MONITOR CRAM   (1200 B, 40×30 colour attributes)
;   $0B6A–$0F69   Kernel         (2 sectors × 512 B = 1 KB loaded area)
;   $0F6A–$1069   KERNELBSS      (256 B — kernel_ipbuf)
;   $106A–$7FFF   FREE RAM       (~28.5 KB — one contiguous block)
;   $8000–$FEFF   BIOS ROM
;   $FF00–$FF0F   Device address table (16 B — 8 two-byte LE pointers)
;   $FFFA–$FFFF   CPU vectors
;
; ============================================================

.ifndef VARS_INCLUDED
VARS_INCLUDED = 1

; ----------------------------------------
; MEMORY LAYOUT
; ----------------------------------------
; Kernel is DMA'd from floppy LBA 0 directly into this address.
; Placed immediately after CRAM so everything above is free RAM.
KERNEL_LOAD_ADDR    = $0B6A

; ----------------------------------------
; ZERO PAGE — BIOS-owned ($00–$0F)
; ----------------------------------------
STRPTR              = $00   ; 2-byte string pointer     (lo=$00, hi=$01)
CMPPTR              = $02   ; 2-byte compare pointer    (lo=$02, hi=$03)
JMPPTR              = $04   ; 2-byte jump pointer       (lo=$04, hi=$05)
KBD_LAST            = $06   ; last keystroke from IRQ handler
FLOPPY_DONE         = $07   ; set to 1 by IRQ on floppy completion
DISPGFX_VRAM_SHADOW = $08   ; 1 byte — temp copy of char being written
DISPGFX_VRAM_WPTR   = $09   ; 2 bytes ($09-$0A) — running ptr into VRAM
DISPGFX_CRAM_WPTR   = $0B   ; 2 bytes ($0B-$0C) — running ptr into CRAM
DISPGFX_CURS_ROW    = $0D   ; 1 byte — current cursor row    (0-29)
DISPGFX_CURS_COL    = $0E   ; 1 byte — current cursor column (0-39)
; $0F spare

; Zero page $10–$FF is free for kernel / app use.

; ----------------------------------------
; MEMORY-MAPPED DEVICE REGISTERS ($0200–$0209)
; The C emulator loads these addresses from the DEVTABLE at $FF00
; at startup, so they are not hardcoded in C — only here and in
; the DEVTABLE segment of bios.s.
; ----------------------------------------
FLOPPY_STATUS_REG       = $0200
FLOPPY_CMD_REG          = $0201
FLOPPY_DATA_REG         = $0202
; $0203 = FLOPPY_DATA_REG+1 (DMA address hi byte) — do not reuse
KBD_DATA_REG            = $0204
DISPTEXT_DATA_REG       = $0205
DISPGFX_CMD_REG         = $0206
DISPGFX_DATA_REG        = $0207 ; 2 bytes ($0207-$0208)
DISPGFX_STATUS_REG      = $0209

; ----------------------------------------
; FLOPPY COMMANDS
; ----------------------------------------
FLOPPY_CMD_NO_CMD       = $00
FLOPPY_CMD_RESET        = $01
FLOPPY_CMD_SET_DMA_ADDR = $02
FLOPPY_CMD_STORE_LBA    = $03
FLOPPY_CMD_READ_SECTOR  = $04
FLOPPY_CMD_WRITE_SECTOR = $05

; ----------------------------------------
; FLOPPY STATUS BITS
; ----------------------------------------
FLOPPY_STATUS_IDLE  = $01
FLOPPY_STATUS_BUSY  = $02
FLOPPY_STATUS_ERROR = $04
FLOPPY_STATUS_IRQ   = $08

; ============================================================
; MONITOR GEOMETRY INFORMATION
; ROWS = 30,  COLS = 40
; Char width = 8,  Char height = 8
; Resolution = 320 × 240 pixels, scale factor = 3
; VRAM size = 1200 bytes
; ============================================================
DISPGFX_ROWS            = 30
DISPGFX_COLS            = 40

; ============================================================
; MONITOR COLOR PALETTE
; Only need the values; the emulator handles actual rendering.
; Colour-RAM attribute byte:
;   low nibble  → foreground colour
;   high nibble → background colour
; ============================================================
DISPGFX_COLOR_BLACK          = $00
DISPGFX_COLOR_DARK_BLUE      = $01
DISPGFX_COLOR_DARK_GREEN     = $02
DISPGFX_COLOR_DARK_CYAN      = $03
DISPGFX_COLOR_DARK_RED       = $04
DISPGFX_COLOR_DARK_MAGENTA   = $05
DISPGFX_COLOR_BROWN          = $06
DISPGFX_COLOR_LIGHT_GREY     = $07
DISPGFX_COLOR_DARK_GREY      = $08
DISPGFX_COLOR_BLUE           = $09
DISPGFX_COLOR_GREEN          = $0A
DISPGFX_COLOR_CYAN           = $0B
DISPGFX_COLOR_LIGHT_RED      = $0C
DISPGFX_COLOR_MAGENTA        = $0D
DISPGFX_COLOR_YELLOW         = $0E
DISPGFX_COLOR_WHITE          = $0F

; Default colour attribute: light grey on black
DISPGFX_DEFAULT_ATTR         = $07

; ===========================================================
; Monitor commands
; ===========================================================
DISPGFX_CMD_NOP         = $00
DISPGFX_CMD_SET_VRAM    = $01 ; DATA = 16-bit LE baseaddr of char VRAM
DISPGFX_CMD_SET_CRAM    = $02 ; DATA = 16-bit LE baseaddr of color RAM
DISPGFX_CMD_CLEAR       = $03 ; fill VRAM with spaces, CRAM with dflt
DISPGFX_CMD_SET_CURSOR  = $04 ; DATA low = col, DATA high = row
DISPGFX_CMD_CURSOR_ON   = $05 ; enable blinking cursor
DISPGFX_CMD_CURSOR_OFF  = $06 ; disable cursor
DISPGFX_CMD_SET_BORDER  = $07 ; DATA low = border colour index (0-15)

; ===========================================================
; Monitor Status Register Bits
; ===========================================================
DISPGFX_STATUS_IDLE     = $01
DISPGFX_STATUS_BUSY     = $02
DISPGFX_STATUS_VBLANK   = $04 ; set once per frame

; ===========================================================
; Monitor Buffers — packed right after device registers
; ===========================================================
DISPGFX_VRAM_BASE       = $020A ; 1200 bytes ($020A–$06B9)
DISPGFX_CRAM_BASE       = $06BA ; 1200 bytes ($06BA–$0B69)

.endif
