.global strncpy
.align 2

dst    .req x0
src    .req x1
len    .req w2

char    .req w22
char64  .req x22
iter    .req x23
target  .req x24
count   .req w25
count64 .req x25
step    .req x26

strncpy:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    str     char64,[sp,#-16]!
    stp     iter,target,[sp,#-16]!
    stp     count64,step,[sp,#-16]!

    add     iter,src,len,uxtw               ; Find end of src buffer
    cmp     iter,dst                        ; Compare to dst
    b.gt    __strncpy_reverse               ; If src + len overlaps with dst + len, we need to iterate backwards
    mov     step,#1                         ; If it isn't we iterate forward
    mov     iter,src                        ; The first source address is src
    mov     target,dst                      ; And the first target address is dst
    b       __strncpy_init_loop             ; Initialize the loop

__strncpy_reverse:
    mov     step,#-1                        ; We're iterating backwards so the step is -1
    add     target,dst,len,uxtw             ; Set target to the end of the dst buffer. iter is already at the
                                            ; end of the src buffer

__strncpy_init_loop:
    mov     count,wzr                       ; Set the counter to 0

__strncpy_loop:
    cmp     count,len                       ; If we have copied len or more bytes, we're done
    b.ge    __strncpy_done
    ldrb    char,[iter]                     ; Load the byte from the src buffer
    strb    char,[target]                   ; Store the byte we just loaded in the dst buffer
    cmp     char,#0                         ; If the byte is \0 we're done
    b.eq    __strncpy_done
    add     count,count,#1                  ; Increment count
    add     iter,iter,step                  ; Increment/decrement src iterator
    add     target,target,step              ; Increment/decrement dst iterator
    b       __strncpy_loop                  ; Back to loop start

__strncpy_done:
    add     src,dst,count,uxtw              ; Copy pointer to last byte +1 of dst in src (x1)
    ldp     count64,step,[sp],#16          ; Restore registers
    ldp     iter,target,[sp],#16
    ldr     char64,[sp],#16
    ldp     fp,lr,[sp],#16
    ret                                     ; Return
