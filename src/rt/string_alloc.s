.include "rt/string.inc"

string_alloc:
    stp     fp,lr,[sp,#-16]!
    stp     x22,x23,[sp,#-16]!
    mov     fp,sp

    cmp     w1,#0                       ; Check that requested length is greater than zero
    b.lt    string_alloc_error

    add     w2,w1,#1                    ; Add one to requested length for zero terminator and store in w2
    mov     x22,x1                      ; Store w1
    add     w1,w2,string_pool_pointer   ; Add requested length (+1) to allocated string pool size and keep in w1
    cmp     w1,string_pool_size         ; Compare to total pool size
    b.gt    string_alloc_error

    mov     x1,x0                       ; Copy src string pointer to x1
    add     x0,string_pool,string_pool_pointer,uxtw ; Find start of new buffer in x0
    bl      strncpy                     ; Copy src string to new buffer
    add     string_pool_pointer,string_pool_pointer,w2 ; Update pointer to end of string pool
    mov     w2,wzr                      ; Terminate new string with \0
    strb    w2,[x1]                     ; strncpy returns the pointer one-past the end of the string in x1
    mov     x1,x22                      ; Restore w1

string_alloc_return:
    ldp     x22,x23,[sp],#16
    ldp     fp,lr,[sp],#16
    ret

string_alloc_error:
    mov     x0,xzr                      ; Return null pointer on error
    mov     w1,#-1                      ; And set w1 to -1
    b       string_alloc_return
