	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 12, 0	sdk_version 12, 3
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	str	wzr, [sp, #12]
	str	w0, [sp, #8]
	str	x1, [sp]
	adrp	x8, _x@PAGE
	ldr	w0, [x8, _x@PAGEOFF]
	add	sp, sp, #16
	ret
	.cfi_endproc
                                        ; -- End function
	.section	__DATA,__data
	.globl	_x                              ; @x
	.p2align	2
_x:
	.long	12                              ; 0xc

.subsections_via_symbols
