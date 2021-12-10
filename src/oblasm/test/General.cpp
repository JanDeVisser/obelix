/*
* Mov.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
*
* This file is part of obelix.
*
* obelix is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* obelix is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with obelix.  If not, see <http://www.gnu.org/licenses/>.
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
