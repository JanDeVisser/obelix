.align 4
.global cputs

;
; puts - Print string
;
; In:
;   x0: Pointer to string buffer

; Out:
;   x0: Number of characters printed.

; Work:
;   x16: Syscall

cputs:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    cmp     x0,#0
    b.eq    __cputs_print_nullptr
    mov     x1,x0
    bl      cstrlen
    mov     w2,w0

__cputs_print:
    ; Print the string:
    mov     x0,#1
    mov     x16,#0x04
    svc     #0x00
    ldp     fp,lr,[sp],#16
    ret

__cputs_print_nullptr:
    ; Print '[[null]]' if the buffer is the null pointer:
    adr     x1,__str_nullptr
    adr     x9,__str_nullptr_len
    ldr     x2,[x9]
    b       __cputs_print

__str_nullptr:
    .string "[[null]]"

.align 4
__str_nullptr_len:
    .int    9
