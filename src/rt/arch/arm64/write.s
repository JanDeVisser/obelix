.include "rt/arch/arm64/syscalls.inc"
.align 4

; In:
fd  .req x0    ; fd to write to (0=stdin)
buffer .req x1 ; Buffer to write from
len .req w2    ; Number of characters to write

; Out
; x0: >0: bytes written, <0: -errno

write:
        mov     x16,syscall_write // MacOS write system call
        svc     #0x80                  // Call MacOS to output the string
        b.lo    __write_ok
        neg     x0,x0
__write_ok:
        ret
