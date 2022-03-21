	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 12, 0	sdk_version 12, 1
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	        sp, sp, #192                    ; =192
	stp	        fr, lp, [sp, #176]            ; 16-byte Folded Spill
	add	        fr, sp, #176                   ; =176
	.cfi_def_cfa fr, 16
	.cfi_offset lp, -8
	.cfi_offset fr, -16
	stur    	wzr, [fr, #-4]
	stur    	w0, [fr, #-8]
	stur	    x1, [fr, #-16]
	adrp	    x0, l_.str@PAGE
	add	        x0, x0, l_.str@PAGEOFF
	add	        x1, sp, #16                     ; =16
	bl	        _stat
	str	        w0, [sp, #12]
	ldr	        w8, [sp, #12]
	tbnz	    w8, #31, LBB0_2
; %bb.1:
	ldr	        x8, [sp, #112]
                                        ; kill: def $w8 killed $w8 killed $x8
	str	        w8, [sp, #12]
LBB0_2:
	ldr	        w0, [sp, #12]
	ldp	        fr, lp, [sp, #176]            ; 16-byte Folded Reload
	add	        sp, sp, #192                    ; =192
	ret
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"stat.c"

.subsections_via_symbols

_stat:
_stat64:
     mov     x16, #338
     svc     #0x80
     b.lo    __stat_return
     stp     fr, lp, [sp, #-16]!
     mov     x29, sp
     bl      _cerror_nocancel
     mov     sp, x29
     ldp     fr, lp, [sp], #16
     ret
__stat_return:
     ret

 _cerror_nocancel:
     adrp    x8, 59 ; 0x3c000
     str     w0, [x8, #96]
     mrs     x8, TPIDRRO_EL0
     ldr     x8, [x8, #8]
     cbz     x8, ___cerror_nocancel_return
     str     w0, [x8]

 ___cerror_nocancel_return:
     mov     x0, #-1
     mov     x1, #-1
     ret
