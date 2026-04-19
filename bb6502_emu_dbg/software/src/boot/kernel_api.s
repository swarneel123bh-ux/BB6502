.segment "KERNEL_API"

.export k_putc

k_putc:
    jmp putc_impl
