.align 4
.global sys$write

// x0 - fd to print to (1=stdout, 2=stderr)
// x1 points to start of string
// x2 holds number of characters to print
sys$write:
        mov x16, syscall_write // MacOS write system call
        svc 0                  // Call MacOS to output the string
        ret

.equ syscall_write, 0x04
