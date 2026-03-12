; ------
; All the GLOBAL wide variables, to be included at the top of every file
; ------

.ifndef VARS_INCLUDED
VARS_INCLUDED = 1

; ----------------------------------------
; MEMORY LAYOUT
; ----------------------------------------
KERNEL_LOAD_ADDR = $2000  ; RAM address where bootloader loads the kernel
; (declared here as constants pointing into ZP segment)
; ----------------------------------------
STRPTR      = $00     ; Zero page String Pointer (2 bytes: $00-$01)

; ----------------------------------------
; DEVICE REGISTERS
; ----------------------------------------
UARTINREG   = $1000   ; UART Input register
UARTOUTREG  = $1001   ; UART Output register
UARTSHADOW  = $1002   ; UART byte temp storage
IXFLAGREG   = $1003   ; Interfacing Flag Register (emulator)
FLPLBAREG   = $1004   ; Floppy LBA address register
FLPSECREG   = $1005   ; Floppy sector count register
FLPDMAREG   = $1006   ; Floppy DMA register (16-bit)

; ----------------------------------------
; RAM VARIABLES
; ----------------------------------------
IPRPTR      = $1004   ; Keyboard input buf read ptr  (2 bytes)
IPWPTR      = $1005   ; Keyboard input buf write ptr (2 bytes)
IPBUF       = $1100   ; Keyboard input buffer (256 bytes: $1100-$11FF)

; ixFlagReg bit description:
; b7: set   => Exit request
; b6: set   => Return control to debugger (no exit)
; b5: set   => Floppy read request
; b4: set   => Floppy write request
; b3:          Unused
; b2: set   => getc request
; b1: reset => UART INPUT REGISTER ready
; b0: set   => UART OUTPUT REGISTER waiting to be read

.endif
