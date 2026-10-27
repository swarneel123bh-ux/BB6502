; ============================================================
; kernel_api.s — EXPERIMENTAL STUB, NOT BUILT
;
; Early sketch of a fixed-address jump table ("syscall" entry
; points) so separately built programs can call the OS without
; linking against rom.o. Not referenced by the Makefile.
;
; Pairs with kernel_impl.s and unified.cfg. Note that unified.cfg
; puts KERNEL_API at $FF00, which is where the DEVTABLE lives in
; the real build, so it cannot be used as-is.
; ============================================================
.segment "KERNEL_API"

.export k_putc

k_putc:
    jmp putc_impl
