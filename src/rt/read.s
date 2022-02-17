.include "rt/syscalls.inc"
.align 4

// x0 - fd to read from (1=stdout, 2=stderr)
// x1 points to start of string
// x2 holds number of characters to read
read:
        mov     x16, syscall_read
        svc     #0x80
        b.lo    __read_ok
        neg     x0,x0
__read_ok:
        ret
