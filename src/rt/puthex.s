.align 4
.global puthex

;
; puthex - Print integer in hexadecimal.
;
; In:
num .req x0  ; Number to print.

; Out:
;   x0: Number of characters printed.

; Work:
;   ---

puthex:
    stp     fp,lr,[sp,#-48]!
    mov     fp,sp

    ; Set up call to to_string.
    ; We have allocated 32 bytes on the stack
    mov     x2,num
    mov     x1,sp
    mov     w0,#32
    mov     w3,#16
    bl      to_string
    bl      puts
    ldp     fp,lr,[sp],#48
    ret
