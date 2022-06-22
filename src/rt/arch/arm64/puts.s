.align 4
.global puts
.global putln

;
; puts - Print string
;
; In:
;   x0: String length
;   x1: Pointer to string buffer

; Out:
;   x0: Number of characters printed.

; Work:
;   x16: Syscall

puts:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    cmp     x1,#0
    b.ne    __puts_print

    ; Print '[[null]]' if the buffer is the null pointer:
    adr     x1,__str_nullptr
    adr     x9,__str_nullptr_len
    ldr     x2,[x9]

    ; Print the string we generated using to_string:
__puts_print:
    mov     x2,x0
    mov     x0,#1
    mov     x16,#0x04
    svc     #0x00
    ldp     fp,lr,[sp],#16
    ret

__str_nullptr:
    .string "[[null]]"

.align 4
__str_nullptr_len:
    .int    9
