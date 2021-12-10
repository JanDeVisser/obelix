print:
    push  a
    push  b
    clr   b

_print_loop:
	mov   a, *si
	cmp   a, b
	jz    _print_done
_print_out:
    out   #0x01, a
	jmp   _print_loop

_print_done:
    pop   b
    pop   a
    ret

println:
    call  print
    push  a
    mov   a, #0x0A
	out   #0x01, a
	pop   a
	ret
