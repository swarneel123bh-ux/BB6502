.segment "BIOS"   ; or separate segment if you prefer

putc_impl:
    sta $0205
    rts
