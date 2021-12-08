/*
 * Opcode.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/StringUtil.h>
#include <oblasm/Image.h>
#include <oblasm/Opcode.h>

namespace Obelix::Assembler {

RegisterDefinition registers[] = {
    { Register::a, "a", 8, 44 },
    { Register::b, "b", 8, 45 },
    { Register::c, "c", 8, 46 },
    { Register::d, "d", 8, 47 },
    { Register::ab, "ab", 16, 48 },
    { Register::cd, "ab", 16, 49 },
    { Register::si, "si", 16, 50 },
    { Register::di, "di", 16, 51 },
    { Register::sp, "sp", 16, 52 },
    { (Register) -1, nullptr, 0, 0 }
};

std::optional<RegisterDefinition> get_register(char const* reg)
{
    for (auto ix = 0; registers[ix].name; ix++) {
        if (!stricmp(reg, registers[ix].name))
            return registers[ix];
    }
    return {};
}

std::optional<RegisterDefinition> get_register(std::string const& reg)
{
    return get_register(reg.c_str());
}

std::optional<RegisterDefinition> get_register(TokenCode code)
{
    for (auto ix = 0; registers[ix].name; ix++) {
        if (registers[ix].token_code == (int) code)
            return registers[ix];
    }
    return {};
}

RegisterDefinition& get_definition(Register reg)
{
    return registers[(int) reg];
}

bool is_register(char const* s)
{
    return get_register(s).has_value();
}

bool is_register(std::string const& s)
{
    return is_register(s.c_str());
}

bool is_register(TokenCode code)
{
    return get_register(code).has_value();
}

OpcodeDefinition opcode_definitions[] = {
    // mnemonic, src imm, src ind, src reg, target imm, target ind, target reg, opcode, bytes
    /* nop           */ { Mnemonic::NOP, false, false, Register::None, false, false, Register::None, 0, 1 },
    /* mov a,#$xx    */ { Mnemonic::MOV, false, false, Register::a, true, false, Register::None, 1, 2 },
    /* mov a,*$xxxx  */ { Mnemonic::MOV, false, false, Register::a, true, true, Register::None, 2, 3 },
    /* mov a,b       */ { Mnemonic::MOV, false, false, Register::a, false, false, Register::b, 3, 1 },
    /* mov a,c       */ { Mnemonic::MOV, false, false, Register::a, false, false, Register::c, 4, 1 },
    /* mov a,d       */ { Mnemonic::MOV, false, false, Register::a, false, false, Register::d, 5, 1 },
    /* mov b,#$xx    */ { Mnemonic::MOV, false, false, Register::b, true, false, Register::None,6, 2 },
    /* mov b,*$xxxx  */ { Mnemonic::MOV, false, false, Register::b, true, true, Register::None,7, 3 },
    /* mov b,a       */ { Mnemonic::MOV, false, false, Register::b, false, false, Register::a, 8, 1 },
    /* mov b,c       */ { Mnemonic::MOV, false, false, Register::b, false, false, Register::c, 9, 1 },
    /* mov b,d       */ { Mnemonic::MOV, false, false, Register::b, false, false, Register::d, 10, 1 },
    /* mov c,#$xx    */ { Mnemonic::MOV, false, false, Register::c, true, false, Register::None, 11, 2 },
    /* mov c,*$xxxx  */ { Mnemonic::MOV, false, false, Register::c, true, true, Register::None, 12, 3 },
    /* mov c,a       */ { Mnemonic::MOV, false, false, Register::c, false, false, Register::a, 13, 1 },
    /* mov c,b       */ { Mnemonic::MOV, false, false, Register::c, false, false, Register::b, 14, 1 },
    /* mov c,d       */ { Mnemonic::MOV, false, false, Register::c, false, false, Register::d, 15, 1 },
    /* mov d,#$xx    */ { Mnemonic::MOV, false, false, Register::d, true, false, Register::None, 16, 2 },
    /* mov d,*$xxxx  */ { Mnemonic::MOV, false, false, Register::d, true, true, Register::None, 17, 3 },
    /* mov d,a       */ { Mnemonic::MOV, false, false, Register::d, false, false, Register::a, 18, 1 },
    /* mov d,b       */ { Mnemonic::MOV, false, false, Register::d, false, false, Register::b, 19, 1 },
    /* mov d,c       */ { Mnemonic::MOV, false, false, Register::d, false, false, Register::c, 20, 1 },
    /* mov sp,#$xxxx */ { Mnemonic::MOV, false, false, Register::sp, true, false, Register::None, 21, 3 },
    /* mov sp,#$xxxx */ { Mnemonic::MOV, false, false, Register::sp, false, true, Register::None, 22, 3 },
    /* mov sp,si     */ { Mnemonic::MOV, false, false, Register::sp, false, false, Register::si, 23, 1 },
    /* mov si,#$xxxx */ { Mnemonic::MOV, false, false, Register::si, true, false, Register::None, 24, 3 },
    /* mov si,#$xxxx */ { Mnemonic::MOV, false, false, Register::si, false, true, Register::None, 25, 3 },
    /* mov si,cd     */ { Mnemonic::MOV, false, false, Register::si, false, false, Register::cd, 26, 1 },
    /* mov di,#$xxxx */ { Mnemonic::MOV, false, false, Register::di, true, false, Register::None, 27, 3 },
    /* mov di,#$xxxx */ { Mnemonic::MOV, false, false, Register::di, false, true, Register::None, 28, 3 },
    /* mov di,cd     */ { Mnemonic::MOV, false, false, Register::di, false, false, Register::cd, 29, 1 },
    /* mov a,*si     */ { Mnemonic::MOV, false, false, Register::a, false, true, Register::si, 30, 1 },
    /* mov b,*si     */ { Mnemonic::MOV, false, false, Register::b, false, true, Register::si, 31, 1 },
    /* mov c,*si     */ { Mnemonic::MOV, false, false, Register::c, false, true, Register::si, 32, 1 },
    /* mov d,*si     */ { Mnemonic::MOV, false, false, Register::d, false, true, Register::si, 33, 1 },
    /* mov a,*di     */ { Mnemonic::MOV, false, false, Register::a, false, true, Register::di, 34, 1 },
    /* mov b,*di     */ { Mnemonic::MOV, false, false, Register::b, false, true, Register::di, 35, 1 },
    /* mov c,*di     */ { Mnemonic::MOV, false, false, Register::c, false, true, Register::di, 36, 1 },
    /* mov d,*di     */ { Mnemonic::MOV, false, false, Register::d, false, true, Register::di, 37, 1 },
    /* mov *di,*si   */ { Mnemonic::MOV, false, true, Register::di, false, true, Register::si, 38, 1 },
    /* jmp #$xxxx    */ { Mnemonic::JMP, true, false, Register::None, false, false, Register::None, 39, 3 },
    /* jnz #$xxxx    */ { Mnemonic::JNZ, true, false, Register::None, false, false, Register::None, 39, 3 },
    /* jc #$xxxx     */ { Mnemonic::JC, true, false, Register::None, false, false, Register::None, 39, 3 },
    /* jv #$xxxx     */ { Mnemonic::JV, true, false, Register::None, false, false, Register::None, 39, 3 },
    /* call #$xxxx   */ { Mnemonic::CALL, true, false, Register::None, false, false, Register::None, 39, 3 },
    /* ret           */ { Mnemonic::RET, false, false, Register::None, false, false, Register::None, 44, 1 },
    /* push a        */ { Mnemonic::PUSH, false, false, Register::a, false, false, Register::None, 45},
    /* push b        */ { Mnemonic::PUSH, false, false, Register::b, false, false, Register::None, 46},
    /* push c        */ { Mnemonic::PUSH, false, false, Register::c, false, false, Register::None, 47},
    /* push d        */ { Mnemonic::PUSH, false, false, Register::d, false, false, Register::None, 48},
    /* push si       */ { Mnemonic::PUSH, false, false, Register::si, false, false, Register::None, 49},
    /* push di       */ { Mnemonic::PUSH, false, false, Register::di, false, false, Register::None, 50},
    /* pop a         */ { Mnemonic::POP, false, false, Register::a, false, false, Register::None, 51},
    /* pop b         */ { Mnemonic::POP, false, false, Register::b, false, false, Register::None, 52},
    /* pop c         */ { Mnemonic::POP, false, false, Register::c, false, false, Register::None, 53},
    /* pop d         */ { Mnemonic::POP, false, false, Register::d, false, false, Register::None, 54},
    /* pop si        */ { Mnemonic::POP, false, false, Register::si, false, false, Register::None, 55},
    /* pop di        */ { Mnemonic::POP, false, false, Register::di, false, false, Register::None, 56},
    /* mov *$xxxx,a  */ { Mnemonic::MOV, true, true, Register::None, false, false, Register::a, 57, 3},
    /* mov *di,a     */ { Mnemonic::MOV, false, true, Register::di, false, false, Register::a, 58, 1},
    /* mov *$xxxx,b  */ { Mnemonic::MOV, true, true, Register::None, false, false, Register::b, 59, 3},
    /* mov *di,b     */ { Mnemonic::MOV, false, true, Register::di, false, false, Register::b, 60, 1},
    /* mov *$xxxx,c  */ { Mnemonic::MOV, true, true, Register::None, false, false, Register::c, 61, 3},
    /* mov *di,c     */ { Mnemonic::MOV, false, true, Register::di, false, false, Register::c, 62, 1},
    /* mov *$xxxx,d  */ { Mnemonic::MOV, true, true, Register::None, false, false, Register::d, 63, 3},
    /* mov *di,d     */ { Mnemonic::MOV, false, true, Register::di, false, false, Register::d, 64, 1},
    /* mov *$xxxx,si */ { Mnemonic::MOV, true, true, Register::None, false, false, Register::si, 65, 3},
    /* mov *$xxxx,di */ { Mnemonic::MOV, true, true, Register::None, false, false, Register::di, 66, 3},
    /* mov *$xxxx,cd */ { Mnemonic::MOV, true, true, Register::None, false, false, Register::cd, 67, 3},
    /* mov *si,cd    */ { Mnemonic::MOV, false, true, Register::si, false, false, Register::cd, 68, 1 },
    /* mov *di,cd    */ { Mnemonic::MOV, false, true, Register::di, false, false, Register::cd, 69, 1 },
    /* add a,b       */ { Mnemonic::ADD, false, false, Register::a, false, false, Register::b, 70, 1 },
    /* adc a,b       */ { Mnemonic::ADC, false, false, Register::a, false, false, Register::b, 71, 1 },
    /* sub a,b       */ { Mnemonic::SUB, false, false, Register::a, false, false, Register::b, 72, 1 },
    /* sbb a,b       */ { Mnemonic::SBB, false, false, Register::a, false, false, Register::b, 73, 1 },
    /* and a,b       */ { Mnemonic::AND, false, false, Register::a, false, false, Register::b, 74, 1 },
    /* or a,b        */ { Mnemonic::OR, false, false, Register::a, false, false, Register::b, 75, 1 },
    /* xor a,b       */ { Mnemonic::XOR, false, false, Register::a, false, false, Register::b, 76, 1 },
    /* not a         */ { Mnemonic::NOT, false, false, Register::a, false, false, Register::None, 77, 1 },
    /* shl a         */ { Mnemonic::SHL, false, false, Register::a, false, false, Register::None, 78, 1 },
    /* shr a         */ { Mnemonic::SHR, false, false, Register::a, false, false, Register::None, 79, 1 },
    /* add a,c       */ { Mnemonic::ADD, false, false, Register::a, false, false, Register::c, 80, 1 },
    /* adc a,c       */ { Mnemonic::ADC, false, false, Register::a, false, false, Register::c, 81, 1 },
    /* sub a,c       */ { Mnemonic::SUB, false, false, Register::a, false, false, Register::c, 82, 1 },
    /* sbb a,c       */ { Mnemonic::SBB, false, false, Register::a, false, false, Register::c, 83, 1 },
    /* and a,c       */ { Mnemonic::AND, false, false, Register::a, false, false, Register::c, 84, 1 },
    /* or a,c        */ { Mnemonic::OR, false, false, Register::a, false, false, Register::c, 85, 1 },
    /* xor a,c       */ { Mnemonic::XOR, false, false, Register::a, false, false, Register::c, 86, 1 },
    /* add a,d       */ { Mnemonic::ADD, false, false, Register::a, false, false, Register::d, 87, 1 },
    /* adc a,d       */ { Mnemonic::ADC, false, false, Register::a, false, false, Register::d, 88, 1 },
    /* sub a,d       */ { Mnemonic::SUB, false, false, Register::a, false, false, Register::d, 89, 1 },
    /* sbb a,d       */ { Mnemonic::SBB, false, false, Register::a, false, false, Register::d, 90, 1 },
    /* and a,d       */ { Mnemonic::AND, false, false, Register::a, false, false, Register::d, 91, 1 },
    /* or a,d        */ { Mnemonic::OR, false, false, Register::a, false, false, Register::d, 92, 1 },
    /* xor a,d       */ { Mnemonic::XOR, false, false, Register::a, false, false, Register::d, 93, 1 },
    /* add b,c       */ { Mnemonic::ADD, false, false, Register::b, false, false, Register::c, 94, 1 },
    /* adc b,c       */ { Mnemonic::ADC, false, false, Register::b, false, false, Register::c, 95, 1 },
    /* sub b,c       */ { Mnemonic::SUB, false, false, Register::b, false, false, Register::c, 96, 1 },
    /* sbb b,c       */ { Mnemonic::SBB, false, false, Register::b, false, false, Register::c, 97, 1 },
    /* and b,c       */ { Mnemonic::AND, false, false, Register::b, false, false, Register::c, 98, 1 },
    /* or b,c        */ { Mnemonic::OR, false, false, Register::b, false, false, Register::c, 99, 1 },
    /* xor b,c       */ { Mnemonic::XOR, false, false, Register::b, false, false, Register::c, 100, 1 },
    /* not b         */ { Mnemonic::NOT, false, false, Register::b, false, false, Register::None, 101, 1 },
    /* shl b         */ { Mnemonic::SHL, false, false, Register::b, false, false, Register::None, 102, 1 },
    /* shr b         */ { Mnemonic::SHR, false, false, Register::b, false, false, Register::None, 103, 1 },
    /* add b,d       */ { Mnemonic::ADD, false, false, Register::b, false, false, Register::d, 104, 1 },
    /* adc b,d       */ { Mnemonic::ADC, false, false, Register::b, false, false, Register::d, 105, 1 },
    /* sub b,d       */ { Mnemonic::SUB, false, false, Register::b, false, false, Register::d, 106, 1 },
    /* sbb b,d       */ { Mnemonic::SBB, false, false, Register::b, false, false, Register::d, 107, 1 },
    /* and b,d       */ { Mnemonic::AND, false, false, Register::b, false, false, Register::d, 108, 1 },
    /* or b,d        */ { Mnemonic::OR, false, false, Register::b, false, false, Register::d, 109, 1 },
    /* xor b,d       */ { Mnemonic::XOR, false, false, Register::b, false, false, Register::d, 110, 1 },
    /* add c,d       */ { Mnemonic::ADD, false, false, Register::c, false, false, Register::d, 111, 1 },
    /* adc c,d       */ { Mnemonic::ADC, false, false, Register::c, false, false, Register::d, 112, 1 },
    /* sub c,d       */ { Mnemonic::SUB, false, false, Register::c, false, false, Register::d, 113, 1 },
    /* sbb c,d       */ { Mnemonic::SBB, false, false, Register::c, false, false, Register::d, 114, 1 },
    /* and c,d       */ { Mnemonic::AND, false, false, Register::c, false, false, Register::d, 115, 1 },
    /* or c,d        */ { Mnemonic::OR, false, false, Register::c, false, false, Register::d, 116, 1 },
    /* xor c,d       */ { Mnemonic::XOR, false, false, Register::c, false, false, Register::d, 117, 1 },
    /* not c         */ { Mnemonic::NOT, false, false, Register::c, false, false, Register::None, 118, 1 },
    /* shl c         */ { Mnemonic::SHL, false, false, Register::c, false, false, Register::None, 119, 1 },
    /* shr c         */ { Mnemonic::SHR, false, false, Register::c, false, false, Register::None, 120, 1 },
    /* not d         */ { Mnemonic::NOT, false, false, Register::d, false, false, Register::None, 121, 1 },
    /* shl d         */ { Mnemonic::SHL, false, false, Register::d, false, false, Register::None, 122, 1 },
    /* shr d         */ { Mnemonic::SHR, false, false, Register::d, false, false, Register::None, 123, 1 },
    /* clr a         */ { Mnemonic::CLR, false, false, Register::a, false, false, Register::None, 124, 1 },
    /* clr b         */ { Mnemonic::CLR, false, false, Register::b, false, false, Register::None, 125, 1 },
    /* clr c         */ { Mnemonic::CLR, false, false, Register::c, false, false, Register::None, 126, 1 },
    /* clr d         */ { Mnemonic::CLR, false, false, Register::d, false, false, Register::None, 127, 1 },
    /* swp a,b       */ { Mnemonic::SWP, false, false, Register::a, false, false, Register::b, 128, 1 },
    /* swp a,c       */ { Mnemonic::SWP, false, false, Register::a, false, false, Register::c, 129, 1 },
    /* swp a,d       */ { Mnemonic::SWP, false, false, Register::a, false, false, Register::d, 130, 1 },
    /* swp b,c       */ { Mnemonic::SWP, false, false, Register::b, false, false, Register::c, 131, 1 },
    /* swp b,d       */ { Mnemonic::SWP, false, false, Register::b, false, false, Register::d, 132, 1 },
    /* swp c,d       */ { Mnemonic::SWP, false, false, Register::c, false, false, Register::d, 133, 1 },
    /* add ab,cd     */ { Mnemonic::ADD, false, false, Register::ab, false, false, Register::cd, 134, 1 },
    /* adc ab,cd     */ { Mnemonic::ADC, false, false, Register::ab, false, false, Register::cd, 135, 1 },
    /* sub ab,cd     */ { Mnemonic::SUB, false, false, Register::ab, false, false, Register::cd, 136, 1 },
    /* sbb ab,cd     */ { Mnemonic::SBB, false, false, Register::ab, false, false, Register::cd, 137, 1 },
    /* jmp *$xxxx    */ { Mnemonic::JMP, true, true, Register::None, false, false, Register::None, 138, 3 },
    /* jnz *$xxxx    */ { Mnemonic::JNZ, true, true, Register::None, false, false, Register::None, 139, 3 },
    /* jc *$xxxx     */ { Mnemonic::JC, true, true, Register::None, false, false, Register::None, 140, 3 },
    /* jv *$xxxx     */ { Mnemonic::JV, true, true, Register::None, false, false, Register::None, 141, 3 },
    /* call *$xxxx   */ { Mnemonic::CALL, true, true, Register::None, false, false, Register::None, 142, 3 },
    /* cmp a,b       */ { Mnemonic::CMP, false, false, Register::a, false, false, Register::b, 143, 1 },
    /* cmp a,c       */ { Mnemonic::CMP, false, false, Register::a, false, false, Register::c, 144, 1 },
    /* cmp a,d       */ { Mnemonic::CMP, false, false, Register::a, false, false, Register::d, 145, 1 },
    /* cmp b,c       */ { Mnemonic::CMP, false, false, Register::b, false, false, Register::c, 146, 1 },
    /* cmp b,d       */ { Mnemonic::CMP, false, false, Register::b, false, false, Register::d, 147, 1 },
    /* cmp c,d       */ { Mnemonic::CMP, false, false, Register::c, false, false, Register::d, 148, 1 },
    /* inc a         */ { Mnemonic::INC, false, false, Register::a, false, false, Register::None, 149, 1 },
    /* inc b         */ { Mnemonic::INC, false, false, Register::b, false, false, Register::None, 150, 1 },
    /* inc c         */ { Mnemonic::INC, false, false, Register::c, false, false, Register::None, 151, 1 },
    /* inc d         */ { Mnemonic::INC, false, false, Register::d, false, false, Register::None, 152, 1 },
    /* dec a         */ { Mnemonic::DEC, false, false, Register::a, false, false, Register::None, 153, 1 },
    /* dec b         */ { Mnemonic::DEC, false, false, Register::b, false, false, Register::None, 154, 1 },
    /* dec c         */ { Mnemonic::DEC, false, false, Register::c, false, false, Register::None, 155, 1 },
    /* dec d         */ { Mnemonic::DEC, false, false, Register::d, false, false, Register::None, 156, 1 },
    /* inc si        */ { Mnemonic::INC, false, false, Register::si, false, false, Register::None, 157, 1 },
    /* inc di        */ { Mnemonic::INC, false, false, Register::di, false, false, Register::None, 158, 1 },
    /* dec si        */ { Mnemonic::DEC, false, false, Register::si, false, false, Register::None, 159, 1 },
    /* dec di        */ { Mnemonic::DEC, false, false, Register::di, false, false, Register::None, 160, 1 },
    /* out #$xx,a    */ { Mnemonic::OUT, true, false, Register::None, false, false, Register::a, 161, 2 },
    /* out #$xx,b    */ { Mnemonic::OUT, true, false, Register::None, false, false, Register::b, 162, 2 },
    /* out #$xx,c    */ { Mnemonic::OUT, true, false, Register::None, false, false, Register::c, 163, 2 },
    /* out #$xx,d    */ { Mnemonic::OUT, true, false, Register::None, false, false, Register::d, 164, 2 },
    /* in a,#$xx     */ { Mnemonic::IN, false, false, Register::a, true, false, Register::None, 165, 2 },
    /* in b,#$xx     */ { Mnemonic::IN, false, false, Register::b, true, false, Register::None, 166, 2 },
    /* in c,#$xx     */ { Mnemonic::IN, false, false, Register::c, true, false, Register::None, 167, 2 },
    /* in d,#$xx     */ { Mnemonic::IN, false, false, Register::d, true, false, Register::None, 168, 2 },
    /* pushfl        */ { Mnemonic::PUSHFL, false, false, Register::None, false, false, Register::None, 169, 1 },
    /* popfl         */ { Mnemonic::POPFL, false, false, Register::None, false, false, Register::None, 170, 1 },
    /* clrfl         */ { Mnemonic::CLRFL, false, false, Register::None, false, false, Register::None, 171, 1 },
    /* jz #$xxxx     */ { Mnemonic::JZ, true, false, Register::None, false, false, Register::None, 172, 3 },
    /* jz *$xxxx     */ { Mnemonic::JZ, true, true, Register::None, false, false, Register::None, 173, 3 },
    /* mov *cd,a     */ { Mnemonic::MOV, false, true, Register::cd, false, false, Register::a, 174, 1 },
    /* mov *cd,b     */ { Mnemonic::MOV, false, true, Register::cd, false, false, Register::b, 175, 1 },
    /* mov a,*cd     */ { Mnemonic::MOV, false, false, Register::a, false, true, Register::cd, 188, 1 },
    /* mov b,*cd     */ { Mnemonic::MOV, false, false, Register::b, false, true, Register::cd, 189, 1 },

    /* rti           */ { Mnemonic::RTI, false, false, Register::None, false, false, Register::None, 253, 1 },
    /* nmi #$xxxx    */ { Mnemonic::NMI, true, false, Register::None, false, false, Register::None, 254, 2 },
    /* hlt           */ { Mnemonic::HLT, false, false, Register::None, false, false, Register::None, 255, 1 },

                        { Mnemonic::None, false, false, Register::None, false, false, Register::None, 0, 0 },
};

OpcodeDefinition const& get_opcode_definition(Mnemonic m, Argument const& target, Argument const &source)
{
    bool target_immediate = target.valid() && (target.type != Argument::ArgumentType::Register);
    bool target_indirect = target.valid() && target.indirect;
    bool source_immediate = source.valid() && (source.type != Argument::ArgumentType::Register);
    bool source_indirect = source.valid() && source.indirect;

    auto ix = 0;
    for (; opcode_definitions[ix].mnemonic != Mnemonic::None; ++ix) {
        if (opcode_definitions[ix].mnemonic != m) continue;
        if ((opcode_definitions[ix].source_immediate != source_immediate) ||
            (opcode_definitions[ix].source_indirect != source_indirect) ||
            (opcode_definitions[ix].target_immediate != target_immediate) ||
                    (opcode_definitions[ix].target_indirect != target_indirect))
            continue;
        return opcode_definitions[ix];
    }
    return opcode_definitions[ix];
}

std::optional<uint16_t> Argument::constant_value(Image const& image) const
{
    switch (type) {
    case ArgumentType::Register:
        return {};
    case ArgumentType::Constant:
        return constant;
    case ArgumentType::Label:
        return image.label_value(label);
    }
}


}
