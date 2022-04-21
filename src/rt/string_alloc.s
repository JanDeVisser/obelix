.include "rt/string.inc"

; string_alloc - Allocate a new stringbuffer and copy the passed in
;                string into it.

; In
len     .req w0
src     .req x1

; Out
; w0 - len, or -1 on error.
; x1 - Newly allocated buffer, or nullptr on error.

; Work
len_stash .req w9

string_alloc:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    cmp     len,#0                       ; Check that requested length is greater than zero
    b.lt    string_alloc_error

    add     w2,len,#1                   ; Add one to requested length for zero terminator and store in w2
    mov     len_stash,len               ; Store len
    add     w2,w2,string_pool_pointer   ; Add requested length (+1) to allocated string pool size and keep in w2
    cmp     w2,string_pool_size         ; Compare to total pool size
    b.gt    string_alloc_error

    add     x0,string_pool,string_pool_pointer,uxtw    ; Find start of new buffer in x0
                                                       ; Note that x1 already contains the src pointer
    bl      strncpy                                    ; Copy src string to new buffer
    add     string_pool_pointer,string_pool_pointer,w2 ; Update pointer to end of string pool
    mov     w2,wzr                                     ; Terminate new string with \0
    strb    w2,[x1]                                    ; strncpy returns the pointer one-past the end of the string in x1
    mov     x0,x1                                      ; Copy pointer to new buffer to x1 for return
    mov     len,len_stash                              ; Restore w1

string_alloc_return:
    ldp     fp,lr,[sp],#16
    ret

string_alloc_error:
    mov     x1,xzr                      ; Return null pointer on error
    mov     w0,#-1                      ; And set w0 to -1
    b       string_alloc_return
