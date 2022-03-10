.global cstr_to_str
.align 2

cstr   .req x0
len    .req x1

char    .req w22
char64  .req x22
ptr     .req x23

cstr_to_str:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    stp     char64,ptr,[sp,#-16]!

    mov     len,xzr
    mov     ptr,cstr

__cstr_to_str_loop:
    ldrb    char,[ptr]
    cmp     char,#0
    b.eq    __cstr_to_str_done
    add     len,len,#1
    add     ptr,ptr,#1
    b       __cstr_to_str_loop

__cstr_to_str_done:
    ldr     char64,[sp],#16
    ldp     fp,lr,[sp],#16
    ret                                     ; Return
