.align 4
.global to_string


; to_string - Convert integer to character string

; In:
buffer_len .req x0    ; Length of buffer
buffer     .req x1    ; Pointer to buffer
num        .req x2    ; Number to convert
radix      .req w3    ; Radix
xradix     .req x3    ; Radix

; Out:
; w0 - Number of bytes moved
; x1 - Pointer to start of string. Can be different from in.

; Work:
buffer_stash .req x6
buffer_end   .req x4
digit        .req w5
xdigit       .req x5
num_div      .req x7
num_mult     .req x8

to_string:
    stp     fp,lr,[sp,#-16]!         ; Save return address
    mov     fp,sp

    mov     buffer_stash,buffer      ; Stash buffer pointer
    add     buffer,buffer,buffer_len ; Make buffer point one past end of the buffer
    mov     buffer_end,buffer        ; Keep the end+1 safe so we can calculate the number of bytes used.
    mov     buffer_len,xzr           ; Set buffer_len to zero. This is the return value

    cmp     radix,#16                ; Check radix. If 16, go to the special hex section
    b.eq    to_hex
    cmp     radix,#2                 ; If 2, go to special binary section.
    b.eq    to_binary                ; We could have a base 4 and octal (base 8) section as well but meh.
    cmp     radix,#0                 ; If not 0, go straigh to the generic algo
    b.ne    to_string_generic
    mov     radix,#10                ; Radix 10 is the default

to_string_generic:                   ; Use a generic algo with remainders and divisors instead of bit masks and 
                                     ; shifts
    sub     buffer,buffer,#1
    add     buffer_len,buffer_len,#1 
    sdiv    num_div,num,xradix        ; num_div holds X / radix (rounded down)
    mul     num_mult,num_div,xradix   ; Multiply num_div with radix so we can get X mod radix with the next step
    sub     xdigit,num,num_mult       ; xdigit holds X mod radix
    add     digit,digit,#48           ; Add '0' to digit
    cmp     digit,#58                 ; did that exceed ASCII '9'?
    b.lt    generic_push_digit        ; It didn't. Push the digit into the buffer
    add     digit,digit,#7            ; add 'A' - ('0'+10) if needed

generic_push_digit:
    strb    digit,[buffer]           ; Poke character in x0 which points to the next position
    cmp     buffer,buffer_stash      ; If x0 equals x6 (the pointer passed in) we're done.
                                     ; FIXME: flag error by returning -1 or something
    b.le    done
    mov     num,num_div              ; Move num_div into num, so we're set up for the next round
    cmp     num,#0                   ; If num is 0, we're done else we go again
    b.ne    to_string_generic
    b       done

; to_hex and to_binary are very similar to the generic case except that we
; can use bitmasks to find the modulo and bit shifts to get the next digit.
to_hex:
    sub     buffer,buffer,#1
    add     buffer_len,buffer_len,#1 
    and     xdigit,num,#0x0F
    add     digit,digit, #48
    cmp     digit, #58               ; did that exceed ASCII '9'?
    b.lt    to_hex_push_digit
    add     digit, digit, #7         ; add 'A' - ('0'+10) if needed

to_hex_push_digit:
    strb    digit,[buffer]
    cmp     buffer,buffer_stash
    b.le    done
    lsr     num,num,#4
    cmp     num,#0
    b.ne    to_hex
    b       done

to_binary:
    sub     buffer,buffer,#1
    add     buffer_len,buffer_len,#1 
    and     xdigit,num,#0x01
    add     digit, digit, #48
    strb    digit,[buffer]
    cmp     buffer,buffer_stash
    b.le    done
    lsr     num,num,#1
    cmp     num,#0
    b.ne    to_binary

done:
bail:
    ldp     fp,lr,[sp],#16   ; Restore return address
    ret
