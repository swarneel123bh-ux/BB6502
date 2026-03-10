; ------
; All the program wide variables, to be included at the top of the
; file
; ------
; ---Zero Page----
STRPTR      = $00     ; Zero page String Pointer
; ---Zero Page----

UARTINREG   = $1000   ; UART Input register
UARTOUTREG  = $1001   ; UART Output register
UARTSHADOW  = $1002   ; UART Byte temp storage
IXFLAGREG   = $1003   ; Interfacing Flag Register
FLPLBAREG  	= $1004 	; Floppy LBA address register
FLPSECREG   = $1005   ; Floppy Sector count register for read/write
FLPDMAREG   = $1006   ; Floppy DMA register (16bit register)

IPRPTR  = $1004   ; Keyboard input buf read ptr (2 bytes)
IPWPTR  = $1005   ; Keyboard input buf write ptr (2 bytes)

IPBUF   = $1100   ; Keyboard input buf ($2000 = $20FF (256 bytes))
; OPBUF = $1200   ; Screen output buffer (later)


; ixFlagReg description :-
; b7: set    => Exit request
; b6: set    => Program wants to return control to debugger but not exit yet
; b5: set    => Program requested floppy read operation
; b4: set    => Program requested floppy write operation
; b3         => Unused
; b2: set    => Character byte (get_c) request
; b1: reset  => UART INPUT REGISTER ready to write to.
; b0: set    => UART OUTPUT REGISTER is waiting to be read from.
