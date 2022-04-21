.include "rt/syscalls.inc"
.align 4

; In:
fd  .req x0    ; fd to read from (1=stdout, 2=stderr)
buffer .req x1 ; Buffer to read into
len .req w2    ; Number of characters to read

; Out
; x0: >0: bytes read, <0: -errno

read:
        mov     x16, syscall_read
        svc     #0x80
        b.lo    __read_ok
        neg     x0,x0
__read_ok:
        ret
