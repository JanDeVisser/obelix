.align 4
.global cputln

;
; putln - Print string followed by a newline character
;
; In:
buffer  .req x0     ; Pointer to the string buffer

; Out:
;   x0: Number of characters printed.

; Work:
;   x16 - syscall

cputln:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    bl      cputs
    mov     w10,w0
    mov     x0,#1
    adr     x1,__str_newline
    mov     x2,#1
    mov     x16,#0x04
    svc     #0x00
    add     w0,w10,#1
    ldp     fp,lr,[sp],#16
    ret

__str_newline:
    .string "\n"
