.define iterations $0017
    jmp  start
	db  0x42
	dw  $AA55
	data   0x00 0x01 0x02
	str "jan de visser"
	asciz  "jan de visser"
start:
	clr a
	clr b
	mov c,      #0x01
	clr d
	mov si,     #iterations

loop:
	add   ab,   cd
	swp   a,    c
	swp   b,    d
	dec   si
	jnz   loop

	mov   di,   cd
	hlt

; .include fib2.asm

