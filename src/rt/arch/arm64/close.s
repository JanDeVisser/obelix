.include "rt/arch/arm64/syscalls.inc"

; In
fd .req w0 ; fd of the file to close

; Out
; x0: >0: file handle, <0: -errno

close:
        mov     x16, syscall_close
        svc     #0x80
        b.lo    __close_ok
        neg     x0,x0
__close_ok:
        ret
