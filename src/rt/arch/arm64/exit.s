.include "rt/arch/arm64/syscalls.inc"

exit:
        mov     x16, syscall_exit
        svc     #0x80
        mov     x0,#-1                  ; Still here? We obviously didn't exit
        ret
