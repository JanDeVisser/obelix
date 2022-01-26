.global _start
.global allocate_string

.equ  string_pool_size, 64*1024

.align 2
_start:
     mov        fp,sp
     bl         main
     mov        x16,#0x01
     svc        0

.if 0

allocate_string:
    cmp     w1,#0
    b.eq    allocate_string_error
    cmp     x0,#0
    b.eq    allocate_string_error

    adrp    x5,string_pool_ptr@GOTPAGE
    adrp    x7,string_pool_end@GOTPAGE
    ldr     x2,[x5]
    sub     x7,x7,x2
    cmp     x7,string_pool_size
    b.lt    allocate_string_error

    mov     w3,wzr
allocate_string_loop:
    ldrb    w5,[x1,w3,uxtw]
    strb    w5,[x2,w3,uxtw]
    add     w3,w3,#0x01
    cmp     w3,w1
    b.eq    allocate_string_done
    b       allocate_string_loop

allocate_string_done:
    mov     x0,x2
    add     x2,x2,x1
    adrp    x5,string_pool_ptr@GOTPAGE
    str     x2,[x5]
    ret

allocate_string_error:
    mov     x0,xzr
    mov     w1,#-1
    ret

.text
.align 4

string_pool:
    .space string_pool_size
string_pool_end:

.align 4
string_pool_ptr:
    .quad string_pool

.endif
