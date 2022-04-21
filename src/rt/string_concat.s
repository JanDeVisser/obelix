.include "rt/string.inc"

; In:
len1    .req w0
buffer1 .req x1
len2    .req w2
buffer2 .req x3

; Out:
; w0 - Total length of concatenated string
; x1 - Buffer for the concatenated string. This is the same as buffer1

; Work:
total_len .req w5

string_concat:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    str     x0,[sp,#-16]!

    cmp     buffer1,#0                  ; Check that first string is unequal to the null pointer
    b.eq    strcat_error                ; FIXME: Check that x0 is between string_pool and string_pool_size
    cmp     buffer2,#0                  ; Check that the second string is unequal to the null pointer
    b.eq    strcat_error
    cmp     len2,#0                     ; Check that length of second string is not 0
    b.eq    strcat_return               ; This is not an error but a NOP

    add     total_len,len1,len2
    add     buffer1,buffer1,len1,uxtw   ; Point buffer1 to the current end of the target string
    mov     x1,x2                       ; Copy src string pointer to x1
    mov     w2,w3
    bl      strncpy                     ; Copy src string to dst string buffer
    mov     w2,wzr                      ; Load terminating 0 in w2
    strb    w2,[buffer1]                ; store terminating zero
    mov     len1,total_len              ; Store new length of string in w0
    ldr     buffer1,[sp],#16            ; Reset buffer1

strcat_return:
    ldp     fp,lr,[sp],#16
    ret

strcat_error:
    mov     x1,xzr                      ; Return null pointer on error
    mov     w0,#-1                      ; And set w1 to -1
    b       strcat_return
