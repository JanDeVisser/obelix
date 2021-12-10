/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <oblasm/Assembler.h>

#include "AsmTest.h"

namespace Obelix::Assembler {

CHECK_SIMPLE_INSTR("nop", NOP)
CHECK_8BIT_IMM("mov a, {}", MOV_A_IMM)
CHECK_16BIT_IMM_IND("mov a, {}", MOV_A_IMM_IND)
CHECK_SIMPLE_INSTR("mov a,b", MOV_A_B)
CHECK_SIMPLE_INSTR("mov a,c", MOV_A_C)
CHECK_SIMPLE_INSTR("mov a,d", MOV_A_D)
CHECK_8BIT_IMM("mov b, {}", MOV_B_IMM)
CHECK_16BIT_IMM_IND("mov b, {}", MOV_B_IMM_IND)
CHECK_SIMPLE_INSTR("mov b,a", MOV_B_A)
CHECK_SIMPLE_INSTR("mov b,c", MOV_B_C)
CHECK_SIMPLE_INSTR("mov b,d", MOV_B_D)
CHECK_8BIT_IMM("mov c, {}", MOV_C_IMM)
CHECK_16BIT_IMM_IND("mov c, {}", MOV_C_IMM_IND)
CHECK_SIMPLE_INSTR("mov c,a", MOV_C_A)
CHECK_SIMPLE_INSTR("mov c,b", MOV_C_B)
CHECK_SIMPLE_INSTR("mov c,d", MOV_C_D)
CHECK_8BIT_IMM("mov d, {}", MOV_D_IMM)
CHECK_16BIT_IMM_IND("mov d, {}", MOV_D_IMM_IND)
CHECK_SIMPLE_INSTR("mov d,a", MOV_D_A)
CHECK_SIMPLE_INSTR("mov d,b", MOV_D_B)
CHECK_SIMPLE_INSTR("mov d,c", MOV_D_C)
CHECK_16BIT_IMM("mov sp, {}", MOV_SP_IMM)
CHECK_16BIT_IMM_IND("mov sp, {}", MOV_SP_IMM_IND)
CHECK_SIMPLE_INSTR("mov sp,si", MOV_SP_SI)
CHECK_16BIT_IMM("mov si, {}", MOV_SI_IMM)
CHECK_16BIT_IMM_IND("mov si, {}", MOV_SI_IMM_IND)
CHECK_SIMPLE_INSTR("mov si,cd", MOV_SI_CD)
CHECK_16BIT_IMM("mov di, {}", MOV_DI_IMM)
CHECK_16BIT_IMM_IND("mov di, {}", MOV_DI_IMM_IND)
CHECK_SIMPLE_INSTR("mov di,cd", MOV_DI_CD)
CHECK_SIMPLE_INSTR("mov a, *si", MOV_A_IND_SI)
CHECK_SIMPLE_INSTR("mov b,*si", MOV_B_IND_SI)
CHECK_SIMPLE_INSTR("mov c,*si", MOV_C_IND_SI)
CHECK_SIMPLE_INSTR("mov d,*si", MOV_D_IND_SI)
CHECK_SIMPLE_INSTR("mov a,*di", MOV_A_IND_DI)
CHECK_SIMPLE_INSTR("mov b,*di", MOV_B_IND_DI)
CHECK_SIMPLE_INSTR("mov c,*di", MOV_C_IND_DI)
CHECK_SIMPLE_INSTR("mov d,*di", MOV_D_IND_DI)
CHECK_SIMPLE_INSTR("mov *di,*si", MOV_IND_DI_IND_SI)
CHECK_16BIT_IMM("jmp {}", JMP)
CHECK_16BIT_IMM("jnz {}", JNZ)
CHECK_16BIT_IMM("jc {}", JC)
CHECK_16BIT_IMM("jv {}", JV)
CHECK_16BIT_IMM("call {}", CALL)
CHECK_SIMPLE_INSTR("ret", RET)
CHECK_SIMPLE_INSTR("push a", PUSH_A)
CHECK_SIMPLE_INSTR("push b", PUSH_B)
CHECK_SIMPLE_INSTR("push c", PUSH_C)
CHECK_SIMPLE_INSTR("push d", PUSH_D)
CHECK_SIMPLE_INSTR("push si", PUSH_SI)
CHECK_SIMPLE_INSTR("push di", PUSH_DI)
CHECK_SIMPLE_INSTR("pop a", POP_A)
CHECK_SIMPLE_INSTR("pop b", POP_B)
CHECK_SIMPLE_INSTR("pop c", POP_C)
CHECK_SIMPLE_INSTR("pop d", POP_D)
CHECK_SIMPLE_INSTR("pop si", POP_SI)
CHECK_SIMPLE_INSTR("pop di", POP_DI)
CHECK_16BIT_IMM_IND("mov {}, a", MOV_IMM_IND_A)
CHECK_SIMPLE_INSTR("mov *di, a", MOV_IND_DI_A)
CHECK_16BIT_IMM_IND("mov {}, b", MOV_IMM_IND_B)
CHECK_SIMPLE_INSTR("mov *di, b", MOV_IND_DI_B)
CHECK_16BIT_IMM_IND("mov {}, c", MOV_IMM_IND_C)
CHECK_SIMPLE_INSTR("mov *di, c", MOV_IND_DI_C)
CHECK_16BIT_IMM_IND("mov {}, d", MOV_IMM_IND_D)
CHECK_SIMPLE_INSTR("mov *di, d", MOV_IND_DI_D)
CHECK_16BIT_IMM_IND("mov {}, si", MOV_IMM_IND_SI)
CHECK_16BIT_IMM_IND("mov {}, di", MOV_IMM_IND_DI)
CHECK_16BIT_IMM_IND("mov {}, cd", MOV_IMM_IND_CD)
CHECK_SIMPLE_INSTR("mov *si,cd", MOV_IND_SI_CD)
CHECK_SIMPLE_INSTR("mov *di,cd", MOV_IND_DI_CD)
CHECK_SIMPLE_INSTR("add a,b", ADD_A_B)
CHECK_SIMPLE_INSTR("adc a,b", ADC_A_B)
CHECK_SIMPLE_INSTR("sub a,b", SUB_A_B)
CHECK_SIMPLE_INSTR("sbb a,b", SBB_A_B)
CHECK_SIMPLE_INSTR("and a,b", AND_A_B)
CHECK_SIMPLE_INSTR("or a,b", OR_A_B)
CHECK_SIMPLE_INSTR("xor a,b", XOR_A_B)
CHECK_SIMPLE_INSTR("not a", NOT_A)
CHECK_SIMPLE_INSTR("shl a", SHL_A)
CHECK_SIMPLE_INSTR("shr a", SHR_A)
CHECK_SIMPLE_INSTR("add a,c", ADD_A_C)
CHECK_SIMPLE_INSTR("adc a,c", ADC_A_C)
CHECK_SIMPLE_INSTR("sub a,c", SUB_A_C)
CHECK_SIMPLE_INSTR("sbb a,c", SBB_A_C)
CHECK_SIMPLE_INSTR("and a,c", AND_A_C)
CHECK_SIMPLE_INSTR("or a,c", OR_A_C)
CHECK_SIMPLE_INSTR("xor a,c", XOR_A_C)
CHECK_SIMPLE_INSTR("add a,d", ADD_A_D)
CHECK_SIMPLE_INSTR("adc a,d", ADC_A_D)
CHECK_SIMPLE_INSTR("sub a,d", SUB_A_D)
CHECK_SIMPLE_INSTR("sbb a,d", SBB_A_D)
CHECK_SIMPLE_INSTR("and a,d", AND_A_D)
CHECK_SIMPLE_INSTR("or a,d", OR_A_D)
CHECK_SIMPLE_INSTR("xor a,d", XOR_A_D)
CHECK_SIMPLE_INSTR("add b,c", ADD_B_C)
CHECK_SIMPLE_INSTR("adc b,c", ADC_B_C)
CHECK_SIMPLE_INSTR("sub b,c", SUB_B_C)
CHECK_SIMPLE_INSTR("sbb b,c", SBB_B_C)
CHECK_SIMPLE_INSTR("and b,c", AND_B_C)
CHECK_SIMPLE_INSTR("or b,c", OR_B_C)
CHECK_SIMPLE_INSTR("xor b,c", XOR_B_C)
CHECK_SIMPLE_INSTR("not b", NOT_B)
CHECK_SIMPLE_INSTR("shl b", SHL_B)
CHECK_SIMPLE_INSTR("shr b", SHR_B)
CHECK_SIMPLE_INSTR("add b,d", ADD_B_D)
CHECK_SIMPLE_INSTR("adc b,d", ADC_B_D)
CHECK_SIMPLE_INSTR("sub b,d", SUB_B_D)
CHECK_SIMPLE_INSTR("sbb b,d", SBB_B_D)
CHECK_SIMPLE_INSTR("and b,d", AND_B_D)
CHECK_SIMPLE_INSTR("or b,d", OR_B_D)
CHECK_SIMPLE_INSTR("xor b,d", XOR_B_D)
CHECK_SIMPLE_INSTR("add c,d", ADD_C_D)
CHECK_SIMPLE_INSTR("adc c,d", ADC_C_D)
CHECK_SIMPLE_INSTR("sub c,d", SUB_C_D)
CHECK_SIMPLE_INSTR("sbb c,d", SBB_C_D)
CHECK_SIMPLE_INSTR("and c,d", AND_C_D)
CHECK_SIMPLE_INSTR("or c,d", OR_C_D)
CHECK_SIMPLE_INSTR("xor c,d", XOR_C_D)
CHECK_SIMPLE_INSTR("not c", NOT_C)
CHECK_SIMPLE_INSTR("shl c", SHL_C)
CHECK_SIMPLE_INSTR("shr c", SHR_C)
CHECK_SIMPLE_INSTR("not d", NOT_D)
CHECK_SIMPLE_INSTR("shl d", SHL_D)
CHECK_SIMPLE_INSTR("shr d", SHR_D)
CHECK_SIMPLE_INSTR("clr a", CLR_A)
CHECK_SIMPLE_INSTR("clr b", CLR_B)
CHECK_SIMPLE_INSTR("clr c", CLR_C)
CHECK_SIMPLE_INSTR("clr d", CLR_D)
CHECK_SIMPLE_INSTR("swp a,b", SWP_A_B)
CHECK_SIMPLE_INSTR("swp a,c", SWP_A_C)
CHECK_SIMPLE_INSTR("swp a,d", SWP_A_D)
CHECK_SIMPLE_INSTR("swp b,c", SWP_B_C)
CHECK_SIMPLE_INSTR("swp b,d", SWP_B_D)
CHECK_SIMPLE_INSTR("swp c,d", SWP_C_D)
CHECK_SIMPLE_INSTR("add ab,cd", ADD_AB_CD)
CHECK_SIMPLE_INSTR("adc ab,cd", ADC_AB_CD)
CHECK_SIMPLE_INSTR("sub ab,cd", SUB_AB_CD)
CHECK_SIMPLE_INSTR("sbb ab,cd", SBB_AB_CD)
CHECK_16BIT_IMM_IND("jmp {}", JMP_IND)
CHECK_16BIT_IMM_IND("jnz {}", JNZ_IND)
CHECK_16BIT_IMM_IND("jc {}", JC_IND)
CHECK_16BIT_IMM_IND("jv {}", JV_IND)
CHECK_16BIT_IMM_IND("call {}", CALL_IND)
CHECK_SIMPLE_INSTR("cmp a,b", CMP_A_B)
CHECK_SIMPLE_INSTR("cmp a,c", CMP_A_C)
CHECK_SIMPLE_INSTR("cmp a,d", CMP_A_D)
CHECK_SIMPLE_INSTR("cmp b,c", CMP_B_C)
CHECK_SIMPLE_INSTR("cmp b,d", CMP_B_D)
CHECK_SIMPLE_INSTR("cmp c,d", CMP_C_D)
CHECK_SIMPLE_INSTR("inc a", INC_A)
CHECK_SIMPLE_INSTR("inc b", INC_B)
CHECK_SIMPLE_INSTR("inc c", INC_C)
CHECK_SIMPLE_INSTR("inc d", INC_D)
CHECK_SIMPLE_INSTR("dec a", DEC_A)
CHECK_SIMPLE_INSTR("dec b", DEC_B)
CHECK_SIMPLE_INSTR("dec c", DEC_C)
CHECK_SIMPLE_INSTR("dec d", DEC_D)
CHECK_SIMPLE_INSTR("inc si", INC_SI)
CHECK_SIMPLE_INSTR("inc di", INC_DI)
CHECK_SIMPLE_INSTR("dec si", DEC_SI)
CHECK_SIMPLE_INSTR("dec di", DEC_DI)
CHECK_8BIT_IMM("out {},a", OUT_A_IMM)
CHECK_8BIT_IMM("out {},b", OUT_B_IMM)
CHECK_8BIT_IMM("out {},c", OUT_C_IMM)
CHECK_8BIT_IMM("out {},d", OUT_D_IMM)
CHECK_8BIT_IMM("in a,{}", IN_A_IMM)
CHECK_8BIT_IMM("in b,{}", IN_B_IMM)
CHECK_8BIT_IMM("in c,{}", IN_C_IMM)
CHECK_8BIT_IMM("in d,{}", IN_D_IMM)
CHECK_SIMPLE_INSTR("pushfl", PUSH_FLAGS)
CHECK_SIMPLE_INSTR("popfl", POP_FLAGS)
CHECK_SIMPLE_INSTR("clrfl", CLR_FLAGS)
CHECK_16BIT_IMM("jz {}", JZ)
CHECK_16BIT_IMM_IND("jz {}", JZ_IND)
CHECK_SIMPLE_INSTR("mov *cd,a", MOV_IND_CD_A)
CHECK_SIMPLE_INSTR("mov *cd,b", MOV_IND_CD_B)
CHECK_8BIT_IMM("cmp a,{}", CMP_A_IMM)
CHECK_8BIT_IMM("cmp b,{}", CMP_B_IMM)
CHECK_8BIT_IMM("cmp c,{}", CMP_C_IMM)
CHECK_8BIT_IMM("cmp d,{}", CMP_D_IMM)
CHECK_8BIT_IMM("and a,{}", AND_A_IMM)
CHECK_8BIT_IMM("and b,{}", AND_B_IMM)
CHECK_8BIT_IMM("and c,{}", AND_C_IMM)
CHECK_8BIT_IMM("and d,{}", AND_D_IMM)
CHECK_8BIT_IMM("or a,{}", OR_A_IMM)
CHECK_8BIT_IMM("or b,{}", OR_B_IMM)
CHECK_8BIT_IMM("or c,{}", OR_C_IMM)
CHECK_8BIT_IMM("or d,{}", OR_D_IMM)
CHECK_SIMPLE_INSTR("mov a,*cd", MOV_A_IND_CD)
CHECK_SIMPLE_INSTR("mov b,*cd", MOV_B_IND_CD)

CHECK_SIMPLE_INSTR("rti", RTI)
CHECK_16BIT_IMM("nmi {}", NMIVEC)
CHECK_SIMPLE_INSTR("hlt", HLT)

}