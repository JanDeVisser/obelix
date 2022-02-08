.include "rt/string.inc"

string_concat:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    str     x0,[sp,#-16]!

    cmp     x0,#0                       ; Check that first string is unequal to the null pointer
    b.eq    strcat_error                ; FIXME: Check that x0 is between string_pool and string_pool_size
    cmp     w1,#0                       ; Check that length of first string is not 0
    b.eq    strcat_error
    cmp     x2,#0                       ; Check that the second string is unequal to the null pointer
    b.eq    strcat_error
    cmp     w3,#0                       ; Check that length of second string is not 0
    b.eq    strcat_return               ; This is not an error but a NOP

    add     x0,x0,w1,uxtw               ; Point x0 to the current end of the target string
    mov     x1,x2                       ; Copy src string pointer to x1
    mov     w2,w3
    bl      strncpy                     ; Copy src string to dst string buffer
    mov     w2,wzr                      ; Load terminating 0 in w2
    strb    w2,[x1]                     ; store terminating zero
    ldr     x0,[sp],#16                 ; Reset x0

strcat_return:
    ldp     fp,lr,[sp],#16
    ret

strcat_error:
    mov     x0,xzr                      ; Return null pointer on error
    mov     w1,#-1                      ; And set w1 to -1
    b       strcat_return
