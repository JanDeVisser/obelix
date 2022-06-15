.include "rt/arch/arm64/string.inc"

; In:
len1    .req w0
buffer1 .req x1
len2    .req w2
buffer2 .req x3

; Out:
; w0 - Total length of concatenated string
; x1 - Buffer for the concatenated string. This is the same as buffer1

; Work:
total_len       .req w20
len1_stash       .req w21
buffer1_stash    .req x22
len2_stash       .req w23
buffer2_stash    .req x24

string_concat:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    stp     x20,x21,[sp,#-16]!
    stp     x22,x23,[sp,#-16]!
    stp     x24,x25,[sp,#-16]!
    mov     buffer1_stash,buffer1
    mov     len1_stash,len1
    mov     buffer2_stash,buffer2
    mov     len2_stash,len2

    cmp     buffer1,#0                  ; Check that first string is unequal to the null pointer
    b.eq    strcat_error                ; FIXME: Check that x0 is between string_pool and string_pool_size
    cmp     buffer2,#0                  ; Check that the second string is unequal to the null pointer
    b.eq    strcat_error
    cmp     len2,#0                     ; Check that length of second string is not 0
    b.eq    strcat_return               ; This is not an error but a NOP

    add     total_len,len1_stash,len2_stash  ; Calculate the length of the concatted string
    add     x0,buffer1_stash,len1_stash,uxtw ; Point x0 to the current end of the target string
    mov     x1,buffer2_stash                 ; Copy src string pointer to x1
                                             ; The length of the string to be concatted is already in w2
    bl      strncpy                          ; Copy src string to dst string buffer
    mov     w2,wzr                           ; Load terminating 0 in w2
    strb    w2,[x1]                          ; store terminating zero
    mov     len1,total_len                   ; Store new length of string in w0
    mov     buffer1,buffer1_stash            ; Reset buffer1

strcat_return:
    ldp     x24,x25,[sp],#16
    ldp     x22,x23,[sp],#16
    ldp     x20,x21,[sp],#16
    ldp     fp,lr,[sp],#16
    ret

strcat_error:
    add     sp,sp,#16                   ; Pop buffer1 from the stack and discard it
    mov     buffer1,xzr                 ; Return null pointer on error
    mov     len1,#-1                    ; And set w0 to -1
    b       strcat_return
