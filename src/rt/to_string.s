.align 4
.global to_string

//
// to_string - Convert integer to character string
//
// In:
//   x0 - Pointer to buffer.
//   x1 - Size of buffer
//   x2 - Number to convert
//   w3 - Radix
//
// Out:
//   x0 - Pointer to start of string. Can be different from in.
//   x1 - Number of bytes
//
// Registers used: x0-x8
//

to_string:
    stp     fp,lr,[sp,#-16]! ; Save return address
    mov     x6,x0       ; x6 keeps the pointer passed in safe
    add     x0,x0,x1    ; Make x0 point one past end of the buffer
    mov     x4,x0       ; x4 keeps the end+1 safe so we can calculate the number of bytes used.

    cmp     w3,#16      ; Check radix. If 16, go to the special hex section
    b.eq    to_hex
    cmp     w3,#2       ; If 2, go to special binary section.
    b.eq    to_binary
                        ; Note that we could have a base 4 and octal (base 8) section as well but meh.

to_string_generic:      ; Use a generic algo with remainders and divisors instead of bit masks and shifts
    sub     x0,x0,#1
    sdiv    x7,x2,x3    ; x7 holds X / radix (rounded down)
    mul     x8,x7,x3    ; Multiply x7 with radix so we can get X mod radix with the next step
    sub     x5,x2,x8    ; w5 holds X mod radix
    add     w5,w5,#48   ; Add '0' to w5
    cmp     w5,#58      ; did that exceed ASCII '9'?
    b.lt    generic_less_than_10
    add     w5,w5,#7    ; add 'A' - ('0'+10) if needed

generic_less_than_10:
    strb    w5,[x0]     ; Poke character in x0 which points to the next position
    cmp     x0,x6       ; If x0 equals x6 (the pointer passed in) we're done.
                        ; FIXME: flag error by returning -1 or something
    b.le    done
    mov     x2,x7       ; Move x7 into x2, so we're set up for the next round
    cmp     x2,#0       ; If x2 is 0, we're done else we go again
    b.ne    to_string_generic
    b       done

; to_hex and to_binary are very similar to the generic case except that we
; can use bitmasks to find the modulo and bit shifts to get the next digit.
to_hex:
    sub     x0,x0,#1
    and     x5,x2,#0x0F
    add     w5, w5, #48
    cmp     w5, #58              ; did that exceed ASCII '9'?
    b.lt    to_hex_less_than_10
    add     w5, w5, #7           ; add 'A' - ('0'+10) if needed

to_hex_less_than_10:
    strb    w5,[x0]
    cmp     x0,x6
    b.le    done
    lsr     x2,x2,#4
    cmp     x2,#0
    b.ne    to_hex
    b       done

to_binary:
    sub     x0,x0,#1
    and     x5,x2,#0x01
    add     w5, w5, #48
    strb    w5,[x0]
    cmp     x0,x6
    b.le    done
    lsr     x2,x2,#1
    cmp     x2,#0
    b.ne    to_binary

done:
    sub     x1,x4,x0         ; Load the difference between x4 and x0 in x1. This is the
                             ; number of characters generated.

bail:
    ldp     fp,lr,[sp],#16   ; Restore return address
    ret
