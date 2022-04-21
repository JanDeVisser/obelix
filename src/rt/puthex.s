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

    ; Print the string we generated using to_string:
    mov     x2,x0
    mov     x0,#1
    mov     x16,#0x04
    svc     #0x00
    ldp     fp,lr,[sp],#48
    ret
