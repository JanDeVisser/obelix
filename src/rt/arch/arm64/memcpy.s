.global memcpy
.align 2

dst     .req x0
src     .req x1
len     .req w2

char    .req w22
char64  .req x22
iter    .req x23
target  .req x24
count   .req w25
count64 .req x25
step    .req x26

memcpy:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    str     char64,[sp,#-16]!
    stp     iter,target,[sp,#-16]!
    stp     count64,step,[sp,#-16]!

    add     iter,x1,x2
    cmp     iter,x0
    b.gt    __memcpy_reverse
    mov     step,#1
    mov     target,x0
    b       __memcpy_init_loop

__memcpy_reverse:
    mov     step,#-1
    add     target,x0,x2

__memcpy_init_loop:
    mov     count,wzr

__memcpy_loop:
    cmp     count,len
    b.ge    __memcpy_done
    ldrb    char,[iter]
    strb    char,[target]
    add     count,count,#1
    add     iter,iter,step
    add     target,target,step
    b       __memcpy_loop

__memcpy_done:
    ldp     count64,step,[sp],#-16
    ldp     iter,target,[sp],#-16
    ldr     char64,[sp],#-16
    ldp     fp,lr,[sp],#16
    ret
