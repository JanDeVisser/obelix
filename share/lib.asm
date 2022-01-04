; Runtime library

        mov sp,#$C000
        mov bp,sp
        mov a,#$42
        mov b,#$15
        call #%multiply8
        mov di,cd
        hlt

; Multiplication adapted from https://llx.com/Neil/a2/mult.html
multiply8:
        clr c
        clr d
        mov di,#$0008
multiply_1:
        shr b
        jnc #%multiply_2
        add d,a

multiply_2:
        shr d
        shr c
        dec di
        jnz #%multiply_1
        ret

;         101  (5)
;      x  011  (3)

;
;  x    num1 num2 a    res
; ----  0101 0011 ---- ----
; 0100  0101 0011 0000 0000
; 0011  0101 0001 0010 1000
; 0010  0101 0000 0011 1100
; 0001  0101 0000 0001 1110
; 0000  0101 0000 0000 1111

;         LDA #0       ;Initialize RESULT to 0
;         LDX #8       ;There are 8 bits in NUM2
; L1      LSR NUM2     ;Get low bit of NUM2
;         BCC L2       ;0 or 1?
;         CLC          ;If 1, add NUM1
;         ADC NUM1
; L2      ROR A        ;"Stairstep" shift (catching carry from add)
;         ROR RESULT
;         DEX
;         BNE L1
;         STA RESULT+1
;
; The method is easily extendable to wider numbers. Here's a routine for multiplying two-byte numbers, giving a four-byte result:
;
;         LDA #0       ;Initialize RESULT to 0
;         STA RESULT+2
;         LDX #16      ;There are 16 bits in NUM2
; L1      LSR NUM2+1   ;Get low bit of NUM2
;         ROR NUM2
;         BCC L2       ;0 or 1?
;         TAY          ;If 1, add NUM1 (hi byte of RESULT is in A)
;         CLC
;         LDA NUM1
;         ADC RESULT+2
;         STA RESULT+2
;         TYA
;         ADC NUM1+1
; L2      ROR A        ;"Stairstep" shift
;         ROR RESULT+2
;         ROR RESULT+1
;         ROR RESULT
;         DEX
;         BNE L1
;         STA RESULT+3
