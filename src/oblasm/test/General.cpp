/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <oblasm/Assembler.h>

#include "AsmTest.h"

namespace Obelix::Assembler {

CHECK_INSTR(UpperCase, "NOP", 1, NOP)
CHECK_INSTR(MixedCase, "Nop", 1, NOP)
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
CHECK_INSTR(JumpAhead, "nop jmp #%lbl nop lbl: nop", 6, NOP, JMP, 0x05, 0x00, NOP, NOP)
CHECK_ASSEMBLY_ERROR(LabelMissing, "nop lbl: nop jmp #%notthere", 5)

CHECK_INSTR(Db, "db $55", 1, 0x55);
CHECK_INSTR(Dw, "dw $5544", 2, 0x44, 0x55);
CHECK_INSTR(Ddw, "ddw $55443322", 4, 0x22, 0x33, 0x44, 0x55)
CHECK_INSTR(Dlw, "dlw $9988776655443322", 8, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99)
CHECK_INSTR(Data, "db $99 $88 $77 $66 $55 $44 $33 $22", 8, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22)

CHECK_INSTR(Str, "str \"Hello Friends\"", 13, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x46, 0x72, 0x69, 0x65, 0x6e, 0x64, 0x73, 0x00)
CHECK_INSTR(Asciz, "asciz \"Hello Friends\"", 14, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x46, 0x72, 0x69, 0x65, 0x6e, 0x64, 0x73, 0x00)

}
