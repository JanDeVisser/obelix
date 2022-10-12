.align 4
.global puts
.global _puts

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
_puts:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    cmp     x1,#0
    b.eq    __puts_print_null
    mov     x2,x0

    ; Print the string we generated using to_string:
__puts_print:
    mov     x0,#1
    mov     x16,#0x04
    svc     #0x00
    ldp     fp,lr,[sp],#16
    ret

    ; Print '[[null]]' if the buffer is the null pointer:
__puts_print_null:
    adr     x1,__str_nullptr
    mov     x2,str_nullptr_len
    b       __puts_print

__str_nullptr:
    .string "[[null]]"

.equ str_nullptr_len,8
