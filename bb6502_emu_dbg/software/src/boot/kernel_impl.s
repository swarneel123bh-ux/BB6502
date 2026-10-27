; ============================================================
; kernel_impl.s — EXPERIMENTAL STUB, NOT BUILT
; Implementation side of the kernel_api.s jump-table sketch.
; $0205 is DISPTEXT_DATA_REG (see vars.s).
; ============================================================
.segment "BIOS"   ; or separate segment if you prefer

putc_impl:
    sta $0205
    rts
