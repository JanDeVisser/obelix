	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 12, 0	sdk_version 12, 1
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #192                    ; =192
	stp	x29, x30, [sp, #176]            ; 16-byte Folded Spill
	add	x29, sp, #176                   ; =176
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	stur	w0, [x29, #-8]
	stur	x1, [x29, #-16]
	adrp	x0, l_.str@PAGE
	add	x0, x0, l_.str@PAGEOFF
	mov	x9, sp
	mov	x8, #144
	str	x8, [x9]
	bl	_printf
	add	x9, sp, #16                     ; =16
	add	x8, x9, #96                     ; =96
	subs	x8, x8, x9
	adrp	x0, l_.str.1@PAGE
	add	x0, x0, l_.str.1@PAGEOFF
	mov	x9, sp
	str	x8, [x9]
	bl	_printf
	adrp	x0, l_.str.2@PAGE
	add	x0, x0, l_.str.2@PAGEOFF
	mov	w1, #2
	bl	_open
	str	w0, [sp, #12]
	ldr	w8, [sp, #12]
	subs	w8, w8, #0                      ; =0
	b.ge	LBB0_2
; %bb.1:
	bl	___error
	ldr	w8, [x0]
	stur	w8, [x29, #-4]
	b	LBB0_5
LBB0_2:
	ldr	w0, [sp, #12]
	add	x1, sp, #16                     ; =16
	bl	_fstat
	str	w0, [sp, #8]
	ldr	w8, [sp, #8]
	tbnz	w8, #31, LBB0_4
; %bb.3:
	ldr	x8, [sp, #112]
                                        ; kill: def $w8 killed $w8 killed $x8
	str	w8, [sp, #8]
	ldr	w9, [sp, #8]
                                        ; implicit-def: $x8
	mov	x8, x9
	adrp	x0, l_.str.3@PAGE
	add	x0, x0, l_.str.3@PAGEOFF
	mov	x9, sp
	str	x8, [x9]
	bl	_printf
LBB0_4:
	ldr	w8, [sp, #8]
	stur	w8, [x29, #-4]
LBB0_5:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #176]            ; 16-byte Folded Reload
	add	sp, sp, #192                    ; =192
	ret
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"sizeof: %ld\n"

l_.str.1:                               ; @.str.1
	.asciz	"offsetof: %ld\n"

l_.str.2:                               ; @.str.2
	.asciz	"stat.c"

l_.str.3:                               ; @.str.3
	.asciz	"size: %d\n"

.subsections_via_symbols
