nmi:
    push  a
    push  c
    push  d
    mov   d, _keybuffer_hi
    mov   c, *_keybuffer_in


key_loop:
	in    a, #0x00
	cmp   a, #0xFF
	jz    nmi_ret
    mov   *cd, a
    inc   c
    and   c, #0x0F
    mov   *_keybuffer_in, c
	jmp   key_loop

nmi_ret:
    pop   d
    pop   c
    pop   a
	rti


