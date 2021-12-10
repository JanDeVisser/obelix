.define stack             0x8000
.define stack_size        0x0400
.define _keybuffer        0x8400
.define _keybuffer_hi     0x84
.define _keybuffer_lo     0x00
.define _keybuffer_in     0x8410
.define _keybuffer_out    0x8411
.define _cmdbuffer_ptr    0x8412

    jmp  start

banner:
    asciz "JV-80 (c) 2020\n\n"

prompt:
    asciz "$ "

ok:
    asciz "OK\n"

start:
	mov   sp, stack
	clr   a
	clr   c
	clr   d
	mov   *_keybuffer_in, a
	mov   *_keybuffer_out, a
	mov   *_cmdbuffer_ptr, cd

	nmi   nmi

	mov   si, #banner
	call  print
	mov   si, #prompt
	call  print


_main_loop:
    mov   d, _keybuffer_hi
    mov   c, *_keybuffer_out

_key_process_loop:
    mov   b, *_keybuffer_in
    cmp   b, c

    jz    _key_process_loop
    mov   a, *cd

    cmp   a, #0x0A
    jnz   _key_process_loop_out
	mov   si, #ok
	call  print
    jmp   _key_process_loop_inc

_key_process_loop_out:
    out   #0x01, a

_key_process_loop_inc:
    inc   c
    and   c, #0x0F
    mov   *_keybuffer_out, c
    jmp   _key_process_loop

	hlt

.include print.asm
.include nmi.asm

