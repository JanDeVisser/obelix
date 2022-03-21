.global _start
.align 2

_start:
    mov     x0,#34
    bl      hex_to_long
    mov     x16,#1
    svc     0

l2h:
    mov     x2,x1

l2h_loop:
    and     x5,x0,#0x0F
    add     w5, w5, #48
    cmp     w5, #58              ; did that exceed ASCII '9'?
    b.lt    less_than_10
    add     w5, w5, #7           ; add 'A' - ('0'+10) if needed
l2h_less_than_10:
    strb    w5,[x2]
    lsr     x0,x0,#4
    add     x2,x2,#1
    cmp     x0,#0
    b.gt    loop

    sub     x2,x2,x1
    mov     x0,#1
    mov     x16, #4
    svc     #0x80
    ret
