.global cstrlen
.align 2

; cstrlen - Determine the length of a null-terminated string

; In
cstr    .req x0

; Out
; w0 - Length of cstr.

; Work
ptr     .req x9
char    .req w10

cstrlen:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    mov     ptr,cstr

__cstrlen_loop:
    ldrb    char,[ptr]
    cmp     char,#0
    b.eq    __cstrlen_done
    add     ptr,ptr,#1
    b       __cstrlen_loop

__cstrlen_done:
    sub     x0,ptr,cstr
    ldp     fp,lr,[sp],#16
    ret                                     ; Return
