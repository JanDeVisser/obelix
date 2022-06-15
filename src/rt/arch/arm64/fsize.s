.include "rt/arch/arm64/syscalls.inc"
.align 4

; In:
fd        .req x0

; Out
; x0: >=0: size of file, <0: -errno

; Work
statbuf .req x1

fsize:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    sub     sp,sp,#144      // sizeof(struct stat)
    mov     statbuf,sp
    mov     x16,syscall_stat
    svc     #0x80
    b.lo    __fsize_ok
    neg     x0,x0
    b       __fsize_done
__fsize_ok:
    ldr     x0,[sp,#96]  // offsetof(struct stat, st_size)
__fsize_done:
    add     sp,sp,#144
    ldp     fp,lr,[sp],#16
    ret
