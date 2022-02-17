.include "rt/syscalls.inc"
.align 4

open:
    stp     fp,lr,[sp,#-48]!
    mov     fp,sp
    mov     x1,x2
    mov     x16,syscall_open
    svc     #0x80
    b.lo    __open_ok
    neg     x0,x0

__open_ok:
    ldp     fp,lr,[sp],#48
    ret
