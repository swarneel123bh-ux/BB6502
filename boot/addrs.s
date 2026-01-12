; Buffer metadatas
STDOUTPTR     = $6000     ; write index
STDOUTBUFSIZ  = $6002     ; number of bytes written

STDINREADPTR  = $6004     ; Read address for stdin
STDINWRITEPTR = $6006     ; Write address for stdin
STDINBUFSIZ   = $6008     ; Size of the stdin buffer

UARTIN        = $600A     ; Input register for the uart (read from here)
UARTOUT       = $600C     ; Output register for the uart (write to this for output)


; Buffers
STDOUTBUF     = $6100     ; stdout buffer
STDINBUF      = $6200     ; stdin buffer

; Zero Page String Pointers
STRPTR        = $00      ; zero-page pointer (2 bytes)

