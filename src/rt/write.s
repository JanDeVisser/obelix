.include "rt/syscalls.inc"
.align 4

// x0 - fd to print to (1=stdout, 2=stderr)
// x1 points to start of string
// x2 holds number of characters to print
write:
        mov     x16,syscall_write // MacOS write system call
        svc     #0x80                  // Call MacOS to output the string
        b.lo    __write_ok
        neg     x0,x0
__write_ok:
        ret
