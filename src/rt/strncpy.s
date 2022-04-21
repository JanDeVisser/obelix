.global strncpy
.align 2

; strncpy - Copy characters from src to dst up to len characters or the first \0
;           character, whichever comes first.

; In
dst    .req x0
src    .req x1
len    .req w2

; Out
; x0 - dst
; x1 - dst + count + 1 - One past the end of the copied string.

; Work
char    .req w9
char64  .req x10
iter    .req x11
target  .req x12
count   .req w13
count64 .req x14
step    .req x15

strncpy:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

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
    ldp     fp,lr,[sp],#16
    ret                                     ; Return
