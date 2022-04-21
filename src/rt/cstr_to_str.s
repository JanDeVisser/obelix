.global cstr_to_str
.align 2

; cstr_to_str - Allocates a new string buffer and copies a the passed-in null
;               terminated string into it.

; In
cstr       .req x0

; Out
len        .req w0
data       .req x1

; Work
char       .req w9
ptr        .req x10
cstr_stash .req x11

cstr_to_str:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    mov     cstr_stash,cstr
    bl      cstrlen             ; Returns len in w0 which we pass straight into string_alloc

    mov     x1,cstr_stash
    bl      string_alloc

__cstr_to_str_done:
    ldp     fp,lr,[sp],#16
    ret                         ; Return
