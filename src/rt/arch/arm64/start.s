.global _start

.include "rt/arch/arm64/string.inc"

.align 2
_start:
    mov     fp,sp
    sub     sp,sp,string_pool_size

    adrp    x8,_string_pool@PAGE
    mov     x9,sp
    str     x9,[x8, _string_pool@PAGEOFF]

    mov     w9,wzr
    adrp    x8,_string_pool_pointer@PAGE
    str     w9,[x8, _string_pool_pointer@PAGEOFF]

    bl      main
    mov     x16,#0x01
    svc     0
