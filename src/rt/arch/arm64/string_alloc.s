.include "rt/string.inc"

; string_alloc - Allocate a new stringbuffer and copy the passed in
;                string into it.

; In
len     .req w0
buffer  .req x1

; Out
; w0 - len, or -1 on error.
; x1 - Newly allocated buffer, or nullptr on error.

; Work
len_work        .req w2
len_stash       .req w20
pool_ptr        .req w21
pool            .req x22
buffer_stash    .req x23

string_alloc:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp
    stp     x20,x21,[sp,#-16]!
    stp     x22,x23,[sp,#-16]!

    cmp     len,#0                                           ; Check that requested length is greater than zero
    b.lt    string_alloc_error

    add     len_work,len,#1                                  ; Add one to requested length for zero terminator and store in w2
    mov     len_stash,len                                    ; Store len

    adrp    x8,_string_pool_pointer@PAGE                     ; Load string pool pointer data page in x8
    ldr     pool_ptr,[x8, _string_pool_pointer@PAGEOFF]      ; Get pointer to end of string pool

    add     len_work,len_work,pool_ptr                       ; Add requested length (+1) to allocated string pool size and keep in len_work
    cmp     len_work,string_pool_size                        ; Compare to total pool size
    b.gt    string_alloc_error

    adrp    x8,_string_pool@PAGE                             ; Load string pool data page in x8
    ldr     pool,[x8, _string_pool@PAGEOFF]                  ; Get start of string pool
    add     x0,pool,pool_ptr,uxtw                            ; Find start of new buffer
                                                             ; Note that x1 already contains the src pointer
    add     len_work,len_stash,#1                            ; Add one to requested length for zero terminator and store in w2
    bl      strncpy                                          ; Copy src string to new buffer
    adrp    x8,_string_pool_pointer@PAGE                     ; Load string pool pointer data page in x8
    str     pool_ptr,[x8, _string_pool_pointer@PAGEOFF]      ; Store pointer

    mov     len_work,wzr                                     ; Terminate new string with \0
    strb    len_work,[x1]                                    ; strncpy returns the pointer one-past the end of the string in x1
    mov     x1,x0                                            ; Copy pointer to new buffer to x1 for return
    mov     len,len_stash                                    ; Restore w0
    mov     buffer_stash,buffer

string_alloc_return:
    ldp     x22,x23,[sp],#16
    ldp     x20,x21,[sp],#16
    ldp     fp,lr,[sp],#16
    ret

string_alloc_error:
    mov     x1,xzr                                           ; Return null pointer on error
    mov     w0,#-1                                           ; And set w0 to -1
    b       string_alloc_return


.section __DATA,__data
.global  _string_pool        
.global  _string_pool_pointer
.align          8

_string_pool:
    .long       0
    .long       0

_string_pool_pointer:
    .long       0
