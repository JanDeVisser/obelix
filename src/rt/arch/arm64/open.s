.include "rt/arch/arm64/syscalls.inc"
.align 4

; In:
path_len  .req w0
path_cstr .req x1
flags     .req x2

; Out
; x0: >0: file handle, <0: -errno

open:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    mov     x0,path_cstr
    mov     x1,flags
    mov     x16,syscall_open
    svc     #0x80
    b.lo    __open_ok
    neg     x0,x0

__open_ok:
    ldp     fp,lr,[sp],#16
    ret
