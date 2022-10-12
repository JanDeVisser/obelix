.align 4

.global putln
.global _putln

;
; putln - Print string followed by a newline character
;
; In:
len     .req w0     ; Length of the string
buffer  .req x1     ; Pointer to the string buffer

; Out:
;   x0: Number of characters printed.

; Work:
;   x16 - syscall

_putln:
putln:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    bl      puts
    mov     x0,#1
    adr     x1,__str_newline
    mov     x2,#1
    mov     x16,#0x04
    svc     #0x00
    ldp     fp,lr,[sp],#16
    ret

__str_newline:
    .string "\n"
