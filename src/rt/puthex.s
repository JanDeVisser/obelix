.align 4
.global _puthex

;
; puthex - Print integer in hexadecimal.
;
; In:
;   x0: Number to print.
; Out:
;   x0: Number of characters printed.
; Registers used: x0-x8
;

_puthex:
    str     lr,[sp,-16]!
    mov     x2,x0
    sub     sp,sp,20
    add     x0,sp,16
    mov     x1,#20
    mov     w3,#16
    bl      to_string

    mov     x2,x1
    mov     x1,x0
    mov     x0,#1
    mov     x16,#0x04
    svc     #0x00
    add     sp,sp,20
    ldr     lr,[sp],16
    ret
