.global _start

.include "rt/string.inc"

.align 2
_start:
    mov     fp,sp
    sub     sp,sp,string_pool_size
    mov     string_pool,sp
    mov     string_pool_pointer,#0
    bl      main
    mov     x16,#0x01
    svc     0
