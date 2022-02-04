.global _start
.global allocate_string
.global concatenate_string

.equ                string_pool_size, 64*1024
string_pool         .req x28
string_pool_pointer .req w27

.align 2
_start:
    mov     fp,sp
    sub     sp,sp,string_pool_size
    mov     string_pool,sp
    mov     string_pool_pointer,#0
    bl      main
    mov     x16,#0x01
    svc     0

allocate_string:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    cmp     w1,#0                       ; Check that requested length is greater than zero
    b.lt    allocate_string_error

    add     w1,w1,#1                    ; Add one to requested length for zero terminator
    add     w2,string_pool_pointer,w1   ; Add requested length (+1) to allocated string pool size and keep in w2
    cmp     w2,string_pool_size         ; Compare to total pool size
    b.gt    allocate_string_error
    add     x3,string_pool,string_pool_pointer,uxtw ; Find start of new buffer

    mov     w4,wzr                      ; Set counter to zero. Do this before check below so we can use w4
    cmp     x0,#0                       ; If we passed in a null pointer, only allocate and don't copy anything
    b.eq    copy_string_done

copy_string_loop:
    ldrb    w5,[x0,w4,uxtw]             ; Copy input string byte to allocated buffer
    strb    w5,[x3,w4,uxtw]
    cmp     w5,#0                       ; Check if we reached the end of the input string
    b.eq    copy_string_done
    add     w4,w4,#0x01                 ; Increment counter
    cmp     w4,w1                       ; Check if we reached the requested length
    b.eq    copy_string_done
    b       copy_string_loop            ; Loop back

copy_string_done:
    mov     w5,wzr                      ; Load terminating 0 in w5
    strb    w5,[x3,w4,uxtw]             ; store terminating zero
    mov     string_pool_pointer,w2      ; Update the allocated string pool size register
    mov     x0,x3                       ; Move the allocated string pointer to x0
    b       allocate_string_return

allocate_string_error:
    mov     x0,xzr                      ; Return null pointer on error
    mov     w1,#-1                      ; And set w1 to -1

allocate_string_return:
    ldp     fp,lr,[sp],#16
    ret



concatenate_string:
    stp     fp,lr,[sp,#-16]!
    mov     fp,sp

    cmp     x0,#0                       ; Check that first string is unequal to the null pointer
    b.eq    strcat_error                ; FIXME: Check that x0 is between string_pool and string_pool_size
    cmp     w1,#0                       ; Check that length of first string is not 0
    b.eq    strcat_error
    cmp     x2,#0                       ; Check that the second string is unequal to the null pointer
    b.eq    strcat_error
    cmp     w3,#0                       ; Check that length of second string is not 0
    b.eq    strcat_return               ; This is not an error but a NOP

    mov     w4,wzr

strcat_loop:
    ldrb    w5,[x2,w4,uxtw]             ; Copy input string byte to allocated buffer
    strb    w5,[x0,w1,uxtw]
    cmp     w5,#0                       ; Check if we reached the end of the second string
    b.eq    strcat_done
    add     w4,w4,#0x01                 ; Increment counter
    add     w1,w1,#0x01                 ; Increment counter
    cmp     w5,w1                       ; Check if we reached the provided length of the second string
    b.eq    strcat_done
    b       strcat_loop                 ; Loop back

strcat_done:
    mov     w5,wzr                      ; Load terminating 0 in w5
    strb    w5,[x0,w1,uxtw]             ; store terminating zero
    b       strcat_return

strcat_error:
    mov     x0,xzr                      ; Return null pointer on error
    mov     w1,#-1                      ; And set w1 to -1

strcat_return:
    ldp     fp,lr,[sp],#16
    ret
