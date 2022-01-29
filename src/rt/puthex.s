.align 4
.global puthex

;
; puthex - Print integer in hexadecimal.
;
; In:
;   x0: Number to print.
; Out:
;   x0: Number of characters printed.
; Registers used: x0-x8
;

puthex:
    stp     fp,lr,[sp,#-48]!
    mov     fp,sp
    mov     x2,x0
    mov     x0,sp
    mov     x1,#32
    mov     w3,#16
    bl      to_string

    mov     x2,x1
    mov     x1,x0
    mov     x0,#1
    mov     x16,#0x04
    svc     #0x00
    ldp     fp,lr,[sp],#48
    ret
