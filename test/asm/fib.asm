.define iterations $0017
    jmp  #%start
start:
	clr a
	clr b
	mov c,      #0x01
	clr d
	mov si,     #%iterations

loop:
	add   ab,   cd
	swp   a,    c
	swp   b,    d
	dec   si
	jnz   #%loop

	mov   di,   cd
	hlt

; .include fib2.asm
