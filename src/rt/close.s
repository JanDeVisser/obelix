.include "rt/syscalls.inc"

close:
        mov     x16, syscall_close
        svc     #0x80
        b.lo    __close_ok
        neg     x0,x0
__close_ok:
        ret
