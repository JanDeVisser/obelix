.global memset
.align 2

ptr     .req x0
char    .req w1
len     .req w2

iter    .req x23
count   .req w24
count64 .req x24

memset:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    stp     iter,count64,[sp,#-16]!

    mov     iter,x0
    mov     count,wzr

__memset_loop:
    cmp     count,len
    b.ge    __memset_done
    strb    char,[iter],#1
    add     count,count,#1
    b       __memset_loop

__memset_done:
    ldp     iter,count64,[sp],#-16
    ldp     fp,lr,[sp],#16
    ret
