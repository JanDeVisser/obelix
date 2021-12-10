/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <oblasm/Assembler.h>

#include "AsmTest.h"

namespace Obelix::Assembler {

CHECK_INSTR(UpperCase, "NOP",  1,  NOP)
CHECK_INSTR(MixedCase, "Nop",  1,  NOP)
CHECK_INSTR(MovAImmBA, "mov a,#$55 mov b,a", 3, MOV_A_IMM, 0x55, MOV_B_A)
CHECK_INSTR(OtherHexFormat, "mov a,#0x55", 2, MOV_A_IMM, 0x55)
CHECK_INSTR(ImmInd, "mov *$c0de, a", 3, MOV_IMM_IND_A, 0xde, 0xc0)
CHECK_INSTR(OtherHexFormatImmInd, "mov *0xc0de, a", 3, MOV_IMM_IND_A, 0xde, 0xc0)
CHECK_ERROR(InvalidMov, "mov a, cd")
CHECK_ERROR(NopWithOneArgument, "nop a")
CHECK_ERROR(NopWithTwoArguments, "nop a,b")
CHECK_ERROR(NopWithImmArgument, "nop #$55")
CHECK_ERROR(NopWithImmIndArgument, "nop *$c0de")
CHECK_ERROR(InvalidMovIntoImmediate, "mov #55, a")
CHECK_ERROR(NoSourceRegisterAfterComma, "mov a, \n mov b,c")

CHECK_INSTR(Label, "nop lbl: nop jmp #%lbl", 5, NOP, NOP, JMP, 0x01, 0x00)
CHECK_INSTR(JumpAhead, "nop jmp #%lbl nop lbl: nop", 6, NOP, JMP, 0x05, 0x00, NOP, NOP )
CHECK_ASSEMBLY_ERROR(LabelMissing, "nop lbl: nop jmp #%notthere", 5)

}
