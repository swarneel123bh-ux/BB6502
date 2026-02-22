; WOZMON specific variables, include in main

; Page 0 Variables

XAML            = $24           ;  Last "opened" location Low
XAMH            = $25           ;  Last "opened" location High
STL             = $26           ;  Store address Low
STH             = $27           ;  Store address High
L               = $28           ;  Hex value parsing Low
H               = $29           ;  Hex value parsing High
YSAV            = $2A           ;  Used to see if hex value is given
MODE            = $2B           ;  $00=XAM, $7F=STOR, $AE=BLOCK XAM


; Other Variables
IN              = $0200         ;  Input buffer to $027F
KBD             = $1010         ;  PIA.A keyboard input
KBDCR           = $1011         ;  PIA.A keyboard control register
DSP             = $1012         ;  PIA.B display output register
DSPCR           = $1013         ;  PIA.B display control register