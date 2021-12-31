/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/StringUtil.h>
#include <oblasm/AssemblyTypes.h>
#include <oblasm/Image.h>

namespace Obelix::Assembler {

RegisterDefinition registers[] = {
    { Register::a, "a", 8, 44 },
    { Register::b, "b", 8, 45 },
    { Register::c, "c", 8, 46 },
    { Register::d, "d", 8, 47 },
    { Register::ab, "ab", 16, 48 },
    { Register::cd, "cd", 16, 49 },
    { Register::si, "si", 16, 50 },
    { Register::di, "di", 16, 51 },
    { Register::sp, "sp", 16, 52 },
    { Register::bp, "bp", 16, 53 },
    { Register::pc, "pc", 16, 0 },
    { Register::flags, "flags", 8, 0 },
    { (Register)-1, nullptr, 0, 0 }
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

OpcodeDefinition opcode_definitions[256] = {
    // mnemonic, src imm, src ind, src reg, target imm, target ind, target reg, opcode, bytes
    /* nop             */ { Mnemonic::NOP, AMNone, Register::None, AMNone, Register::None, 1 },
    /* mov a,#$xx      */ { Mnemonic::MOV, AMRegister, Register::a, AMImmediate, Register::None, 2 },
    /* mov a,*$xxxx    */ { Mnemonic::MOV, AMRegister, Register::a, AMImmediateIndirect, Register::None, 3 },
    /* mov a,b         */ { Mnemonic::MOV, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* mov a,c         */ { Mnemonic::MOV, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* mov a,d         */ { Mnemonic::MOV, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* mov b,#$xx      */ { Mnemonic::MOV, AMRegister, Register::b, AMImmediate, Register::None, 2 },
    /* mov b,*$xxxx    */ { Mnemonic::MOV, AMRegister, Register::b, AMImmediateIndirect, Register::None, 3 },
    /* mov b,a         */ { Mnemonic::MOV, AMRegister, Register::b, AMRegister, Register::a, 1 },
    /* mov b,c         */ { Mnemonic::MOV, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* mov b,d         */ { Mnemonic::MOV, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* mov c,#$xx      */ { Mnemonic::MOV, AMRegister, Register::c, AMImmediate, Register::None, 2 },
    /* mov c,*$xxxx    */ { Mnemonic::MOV, AMRegister, Register::c, AMImmediateIndirect, Register::None, 3 },
    /* mov c,a         */ { Mnemonic::MOV, AMRegister, Register::c, AMRegister, Register::a, 1 },
    /* mov c,b         */ { Mnemonic::MOV, AMRegister, Register::c, AMRegister, Register::b, 1 },
    /* mov c,d         */ { Mnemonic::MOV, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* mov d,#$xx      */ { Mnemonic::MOV, AMRegister, Register::d, AMImmediate, Register::None, 2 },
    /* mov d,*$xxxx    */ { Mnemonic::MOV, AMRegister, Register::d, AMImmediateIndirect, Register::None, 3 },
    /* mov d,a         */ { Mnemonic::MOV, AMRegister, Register::d, AMRegister, Register::a, 1 },
    /* mov d,b         */ { Mnemonic::MOV, AMRegister, Register::d, AMRegister, Register::b, 1 },
    /* mov d,c         */ { Mnemonic::MOV, AMRegister, Register::d, AMRegister, Register::c, 1 },
    /* mov sp,#$xxxx   */ { Mnemonic::MOV, AMRegister, Register::sp, AMImmediate, Register::None, 3 },
    /* mov sp,*$xxxx   */ { Mnemonic::MOV, AMRegister, Register::sp, AMImmediateIndirect, Register::None, 3 },
    /* mov sp,si       */ { Mnemonic::MOV, AMRegister, Register::sp, AMRegister, Register::si, 1 },
    /* mov si,#$xxxx   */ { Mnemonic::MOV, AMRegister, Register::si, AMImmediate, Register::None, 3 },
    /* mov si,*$xxxx   */ { Mnemonic::MOV, AMRegister, Register::si, AMImmediateIndirect, Register::None, 3 },
    /* mov si,cd       */ { Mnemonic::MOV, AMRegister, Register::si, AMRegister, Register::cd, 1 },
    /* mov di,#$xxxx   */ { Mnemonic::MOV, AMRegister, Register::di, AMImmediate, Register::None, 3 },
    /* mov di,*$xxxx   */ { Mnemonic::MOV, AMRegister, Register::di, AMImmediateIndirect, Register::None, 3 },
    /* mov di,cd       */ { Mnemonic::MOV, AMRegister, Register::di, AMRegister, Register::cd, 1 },
    /* mov a,*si       */ { Mnemonic::MOV, AMRegister, Register::a, AMRegisterIndirect, Register::si, 1 },
    /* mov b,*si       */ { Mnemonic::MOV, AMRegister, Register::b, AMRegisterIndirect, Register::si, 1 },
    /* mov c,*si       */ { Mnemonic::MOV, AMRegister, Register::c, AMRegisterIndirect, Register::si, 1 },
    /* mov d,*si       */ { Mnemonic::MOV, AMRegister, Register::d, AMRegisterIndirect, Register::si, 1 },
    /* mov a,*di       */ { Mnemonic::MOV, AMRegister, Register::a, AMRegisterIndirect, Register::di, 1 },
    /* mov b,*di       */ { Mnemonic::MOV, AMRegister, Register::b, AMRegisterIndirect, Register::di, 1 },
    /* mov c,*di       */ { Mnemonic::MOV, AMRegister, Register::c, AMRegisterIndirect, Register::di, 1 },
    /* mov d,*di       */ { Mnemonic::MOV, AMRegister, Register::d, AMRegisterIndirect, Register::di, 1 },
    /* mov *di,*si     */ { Mnemonic::MOV, AMRegisterIndirect, Register::di, AMRegisterIndirect, Register::si, 1 },
    /* jmp #$xxxx      */ { Mnemonic::JMP, AMImmediate, Register::None, AMNone, Register::None, 3 },
    /* jnz #$xxxx      */ { Mnemonic::JNZ, AMImmediate, Register::None, AMNone, Register::None, 3 },
    /* jc #$xxxx       */ { Mnemonic::JC, AMImmediate, Register::None, AMNone, Register::None, 3 },
    /* jv #$xxxx       */ { Mnemonic::JV, AMImmediate, Register::None, AMNone, Register::None, 3 },
    /* call #$xxxx     */ { Mnemonic::CALL, AMImmediate, Register::None, AMNone, Register::None, 3 },
    /* ret             */ { Mnemonic::RET, AMNone, Register::None, AMNone, Register::None, 1 },
    /* push a          */ { Mnemonic::PUSH, AMRegister, Register::a, AMNone, Register::None, 1 },
    /* push b          */ { Mnemonic::PUSH, AMRegister, Register::b, AMNone, Register::None, 1 },
    /* push c          */ { Mnemonic::PUSH, AMRegister, Register::c, AMNone, Register::None, 1 },
    /* push d          */ { Mnemonic::PUSH, AMRegister, Register::d, AMNone, Register::None, 1 },
    /* push si         */ { Mnemonic::PUSH, AMRegister, Register::si, AMNone, Register::None, 1 },
    /* push di         */ { Mnemonic::PUSH, AMRegister, Register::di, AMNone, Register::None, 1 },
    /* pop a           */ { Mnemonic::POP, AMRegister, Register::a, AMNone, Register::None, 1 },
    /* pop b           */ { Mnemonic::POP, AMRegister, Register::b, AMNone, Register::None, 1 },
    /* pop c           */ { Mnemonic::POP, AMRegister, Register::c, AMNone, Register::None, 1 },
    /* pop d           */ { Mnemonic::POP, AMRegister, Register::d, AMNone, Register::None, 1 },
    /* pop si          */ { Mnemonic::POP, AMRegister, Register::si, AMNone, Register::None, 1 },
    /* pop di          */ { Mnemonic::POP, AMRegister, Register::di, AMNone, Register::None, 1 },
    /* mov *$xxxx,a    */ { Mnemonic::MOV, AMImmediateIndirect, Register::None, AMRegister, Register::a, 3 },
    /* mov *di,a       */ { Mnemonic::MOV, AMRegisterIndirect, Register::di, AMRegister, Register::a, 1 },
    /* mov *$xxxx,b    */ { Mnemonic::MOV, AMImmediateIndirect, Register::None, AMRegister, Register::b, 3 },
    /* mov *di,b       */ { Mnemonic::MOV, AMRegisterIndirect, Register::di, AMRegister, Register::b, 1 },
    /* mov *$xxxx,c    */ { Mnemonic::MOV, AMImmediateIndirect, Register::None, AMRegister, Register::c, 3 },
    /* mov *di,c       */ { Mnemonic::MOV, AMRegisterIndirect, Register::di, AMRegister, Register::c, 1 },
    /* mov *$xxxx,d    */ { Mnemonic::MOV, AMImmediateIndirect, Register::None, AMRegister, Register::d, 3 },
    /* mov *di,d       */ { Mnemonic::MOV, AMRegisterIndirect, Register::di, AMRegister, Register::d, 1 },
    /* mov *$xxxx,si   */ { Mnemonic::MOV, AMImmediateIndirect, Register::None, AMRegister, Register::si, 3 },
    /* mov *$xxxx,di   */ { Mnemonic::MOV, AMImmediateIndirect, Register::None, AMRegister, Register::di, 3 },
    /* mov *$xxxx,cd   */ { Mnemonic::MOV, AMImmediateIndirect, Register::None, AMRegister, Register::cd, 3 },
    /* mov *si,cd      */ { Mnemonic::MOV, AMRegisterIndirect, Register::si, AMRegister, Register::cd, 1 },
    /* mov *di,cd      */ { Mnemonic::MOV, AMRegisterIndirect, Register::di, AMRegister, Register::cd, 1 },
    /* add a,b         */ { Mnemonic::ADD, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* adc a,b         */ { Mnemonic::ADC, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* sub a,b         */ { Mnemonic::SUB, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* sbb a,b         */ { Mnemonic::SBB, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* and a,b         */ { Mnemonic::AND, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* or a,b          */ { Mnemonic::OR, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* xor a,b         */ { Mnemonic::XOR, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* not a           */ { Mnemonic::NOT, AMRegister, Register::a, AMNone, Register::None, 1 },
    /* shl a           */ { Mnemonic::SHL, AMRegister, Register::a, AMNone, Register::None, 1 },
    /* shr a           */ { Mnemonic::SHR, AMRegister, Register::a, AMNone, Register::None, 1 },
    /* add a,c         */ { Mnemonic::ADD, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* adc a,c         */ { Mnemonic::ADC, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* sub a,c         */ { Mnemonic::SUB, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* sbb a,c         */ { Mnemonic::SBB, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* and a,c         */ { Mnemonic::AND, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* or a,c          */ { Mnemonic::OR, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* xor a,c         */ { Mnemonic::XOR, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* add a,d         */ { Mnemonic::ADD, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* adc a,d         */ { Mnemonic::ADC, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* sub a,d         */ { Mnemonic::SUB, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* sbb a,d         */ { Mnemonic::SBB, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* and a,d         */ { Mnemonic::AND, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* or a,d          */ { Mnemonic::OR, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* xor a,d         */ { Mnemonic::XOR, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* add b,c         */ { Mnemonic::ADD, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* adc b,c         */ { Mnemonic::ADC, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* sub b,c         */ { Mnemonic::SUB, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* sbb b,c         */ { Mnemonic::SBB, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* and b,c         */ { Mnemonic::AND, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* or b,c          */ { Mnemonic::OR, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* xor b,c         */ { Mnemonic::XOR, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* not b           */ { Mnemonic::NOT, AMRegister, Register::b, AMNone, Register::None, 1 },
    /* shl b           */ { Mnemonic::SHL, AMRegister, Register::b, AMNone, Register::None, 1 },
    /* shr b           */ { Mnemonic::SHR, AMRegister, Register::b, AMNone, Register::None, 1 },
    /* add b,d         */ { Mnemonic::ADD, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* adc b,d         */ { Mnemonic::ADC, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* sub b,d         */ { Mnemonic::SUB, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* sbb b,d         */ { Mnemonic::SBB, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* and b,d         */ { Mnemonic::AND, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* or b,d          */ { Mnemonic::OR, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* xor b,d         */ { Mnemonic::XOR, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* add c,d         */ { Mnemonic::ADD, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* adc c,d         */ { Mnemonic::ADC, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* sub c,d         */ { Mnemonic::SUB, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* sbb c,d         */ { Mnemonic::SBB, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* and c,d         */ { Mnemonic::AND, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* or c,d          */ { Mnemonic::OR, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* xor c,d         */ { Mnemonic::XOR, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* not c           */ { Mnemonic::NOT, AMRegister, Register::c, AMNone, Register::None, 1 },
    /* shl c           */ { Mnemonic::SHL, AMRegister, Register::c, AMNone, Register::None, 1 },
    /* shr c           */ { Mnemonic::SHR, AMRegister, Register::c, AMNone, Register::None, 1 },
    /* not d           */ { Mnemonic::NOT, AMRegister, Register::d, AMNone, Register::None, 1 },
    /* shl d           */ { Mnemonic::SHL, AMRegister, Register::d, AMNone, Register::None, 1 },
    /* shr d           */ { Mnemonic::SHR, AMRegister, Register::d, AMNone, Register::None, 1 },
    /* clr a           */ { Mnemonic::CLR, AMRegister, Register::a, AMNone, Register::None, 1 },
    /* clr b           */ { Mnemonic::CLR, AMRegister, Register::b, AMNone, Register::None, 1 },
    /* clr c           */ { Mnemonic::CLR, AMRegister, Register::c, AMNone, Register::None, 1 },
    /* clr d           */ { Mnemonic::CLR, AMRegister, Register::d, AMNone, Register::None, 1 },
    /* swp a,b         */ { Mnemonic::SWP, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* swp a,c         */ { Mnemonic::SWP, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* swp a,d         */ { Mnemonic::SWP, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* swp b,c         */ { Mnemonic::SWP, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* swp b,d         */ { Mnemonic::SWP, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* swp c,d         */ { Mnemonic::SWP, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* add ab,cd       */ { Mnemonic::ADD, AMRegister, Register::ab, AMRegister, Register::cd, 1 },
    /* adc ab,cd       */ { Mnemonic::ADC, AMRegister, Register::ab, AMRegister, Register::cd, 1 },
    /* sub ab,cd       */ { Mnemonic::SUB, AMRegister, Register::ab, AMRegister, Register::cd, 1 },
    /* sbb ab,cd       */ { Mnemonic::SBB, AMRegister, Register::ab, AMRegister, Register::cd, 1 },
    /* jmp *$xxxx      */ { Mnemonic::JMP, AMImmediateIndirect, Register::None, AMNone, Register::None, 3 },
    /* jnz *$xxxx      */ { Mnemonic::JNZ, AMImmediateIndirect, Register::None, AMNone, Register::None, 3 },
    /* jc *$xxxx       */ { Mnemonic::JC, AMImmediateIndirect, Register::None, AMNone, Register::None, 3 },
    /* jv *$xxxx       */ { Mnemonic::JV, AMImmediateIndirect, Register::None, AMNone, Register::None, 3 },
    /* call *$xxxx     */ { Mnemonic::CALL, AMImmediateIndirect, Register::None, AMNone, Register::None, 3 },
    /* cmp a,b         */ { Mnemonic::CMP, AMRegister, Register::a, AMRegister, Register::b, 1 },
    /* cmp a,c         */ { Mnemonic::CMP, AMRegister, Register::a, AMRegister, Register::c, 1 },
    /* cmp a,d         */ { Mnemonic::CMP, AMRegister, Register::a, AMRegister, Register::d, 1 },
    /* cmp b,c         */ { Mnemonic::CMP, AMRegister, Register::b, AMRegister, Register::c, 1 },
    /* cmp b,d         */ { Mnemonic::CMP, AMRegister, Register::b, AMRegister, Register::d, 1 },
    /* cmp c,d         */ { Mnemonic::CMP, AMRegister, Register::c, AMRegister, Register::d, 1 },
    /* inc a           */ { Mnemonic::INC, AMRegister, Register::a, AMNone, Register::None, 1 },
    /* inc b           */ { Mnemonic::INC, AMRegister, Register::b, AMNone, Register::None, 1 },
    /* inc c           */ { Mnemonic::INC, AMRegister, Register::c, AMNone, Register::None, 1 },
    /* inc d           */ { Mnemonic::INC, AMRegister, Register::d, AMNone, Register::None, 1 },
    /* dec a           */ { Mnemonic::DEC, AMRegister, Register::a, AMNone, Register::None, 1 },
    /* dec b           */ { Mnemonic::DEC, AMRegister, Register::b, AMNone, Register::None, 1 },
    /* dec c           */ { Mnemonic::DEC, AMRegister, Register::c, AMNone, Register::None, 1 },
    /* dec d           */ { Mnemonic::DEC, AMRegister, Register::d, AMNone, Register::None, 1 },
    /* inc si          */ { Mnemonic::INC, AMRegister, Register::si, AMNone, Register::None, 1 },
    /* inc di          */ { Mnemonic::INC, AMRegister, Register::di, AMNone, Register::None, 1 },
    /* dec si          */ { Mnemonic::DEC, AMRegister, Register::si, AMNone, Register::None, 1 },
    /* dec di          */ { Mnemonic::DEC, AMRegister, Register::di, AMNone, Register::None, 1 },
    /* out #$xx,a      */ { Mnemonic::OUT, AMImmediate, Register::None, AMRegister, Register::a, 2 },
    /* out #$xx,b      */ { Mnemonic::OUT, AMImmediate, Register::None, AMRegister, Register::b, 2 },
    /* out #$xx,c      */ { Mnemonic::OUT, AMImmediate, Register::None, AMRegister, Register::c, 2 },
    /* out #$xx,d      */ { Mnemonic::OUT, AMImmediate, Register::None, AMRegister, Register::d, 2 },
    /* in a,#$xx       */ { Mnemonic::IN, AMRegister, Register::a, AMImmediate, Register::None, 2 },
    /* in b,#$xx       */ { Mnemonic::IN, AMRegister, Register::b, AMImmediate, Register::None, 2 },
    /* in c,#$xx       */ { Mnemonic::IN, AMRegister, Register::c, AMImmediate, Register::None, 2 },
    /* in d,#$xx       */ { Mnemonic::IN, AMRegister, Register::d, AMImmediate, Register::None, 2 },
    /* pushfl          */ { Mnemonic::PUSHFL, AMNone, Register::None, AMNone, Register::None, 1 },
    /* popfl           */ { Mnemonic::POPFL, AMNone, Register::None, AMNone, Register::None, 1 },
    /* clrfl           */ { Mnemonic::CLRFL, AMNone, Register::None, AMNone, Register::None, 1 },
    /* jz #$xxxx       */ { Mnemonic::JZ, AMImmediate, Register::None, AMNone, Register::None, 3 },
    /* jz *$xxxx       */ { Mnemonic::JZ, AMImmediateIndirect, Register::None, AMNone, Register::None, 3 },
    /* mov *cd,a       */ { Mnemonic::MOV, AMRegisterIndirect, Register::cd, AMRegister, Register::a, 1 },
    /* mov *cd,b       */ { Mnemonic::MOV, AMRegisterIndirect, Register::cd, AMRegister, Register::b, 1 },
    /* cmp a,#$xx      */ { Mnemonic::CMP, AMRegister, Register::a, AMImmediate, Register::None, 2 },
    /* cmp b,#$xx      */ { Mnemonic::CMP, AMRegister, Register::b, AMImmediate, Register::None, 2 },
    /* cmp c,#$xx      */ { Mnemonic::CMP, AMRegister, Register::c, AMImmediate, Register::None, 2 },
    /* cmp d,#$xx      */ { Mnemonic::CMP, AMRegister, Register::d, AMImmediate, Register::None, 2 },
    /* and a,#$xx      */ { Mnemonic::AND, AMRegister, Register::a, AMImmediate, Register::None, 2 },
    /* and b,#$xx      */ { Mnemonic::AND, AMRegister, Register::b, AMImmediate, Register::None, 2 },
    /* and c,#$xx      */ { Mnemonic::AND, AMRegister, Register::c, AMImmediate, Register::None, 2 },
    /* and d,#$xx      */ { Mnemonic::AND, AMRegister, Register::d, AMImmediate, Register::None, 2 },
    /* or a,#$xx       */ { Mnemonic::OR, AMRegister, Register::a, AMImmediate, Register::None, 2 },
    /* or b,#$xx       */ { Mnemonic::OR, AMRegister, Register::b, AMImmediate, Register::None, 2 },
    /* or c,#$xx       */ { Mnemonic::OR, AMRegister, Register::c, AMImmediate, Register::None, 2 },
    /* or d,#$xx       */ { Mnemonic::OR, AMRegister, Register::d, AMImmediate, Register::None, 2 },
    /* mov a,*cd       */ { Mnemonic::MOV, AMRegister, Register::a, AMRegisterIndirect, Register::cd, 1 },
    /* mov b,*cd       */ { Mnemonic::MOV, AMRegister, Register::b, AMRegisterIndirect, Register::cd, 1 },
    /* mov *si,#$xxxx  */ { Mnemonic::MOV, AMRegisterIndirect, Register::si, AMImmediate, Register::None, 3 },
    /* mov *di,#$xxxx  */ { Mnemonic::MOV, AMRegisterIndirect, Register::di, AMImmediate, Register::None, 3 },
    /* mov *cd,#$xxxx  */ { Mnemonic::MOV, AMRegisterIndirect, Register::cd, AMImmediate, Register::None, 3 },
    /* mov cd,#$xxxx   */ { Mnemonic::MOV, AMRegister, Register::cd, AMImmediate, Register::None, 3 },
    /* mov bp,sp       */ { Mnemonic::MOV, AMRegister, Register::bp, AMRegister, Register::sp, 1 },
    /* mov sp,bp       */ { Mnemonic::MOV, AMRegister, Register::sp, AMRegister, Register::bp, 1 },
    /* mov si,bp[$xx]  */ { Mnemonic::MOV, AMRegister, Register::si, AMIndexed, Register::bp, 2 },
    /* mov di,bp[$xx]  */ { Mnemonic::MOV, AMRegister, Register::di, AMIndexed, Register::bp, 2 },
    /* mov di,si[$xx]  */ { Mnemonic::MOV, AMRegister, Register::di, AMIndexed, Register::si, 2 },
    /* mov a,bp[$xx]   */ { Mnemonic::MOV, AMRegister, Register::a, AMIndexed, Register::bp, 2 },
    /* mov b,bp[$xx]   */ { Mnemonic::MOV, AMRegister, Register::b, AMIndexed, Register::bp, 2 },
    /* mov c,bp[$xx]   */ { Mnemonic::MOV, AMRegister, Register::c, AMIndexed, Register::bp, 2 },
    /* mov d,bp[$xx]   */ { Mnemonic::MOV, AMRegister, Register::d, AMIndexed, Register::bp, 2 },
    /* mov bp[$xx],si  */ { Mnemonic::MOV, AMIndexed, Register::bp, AMRegister, Register::si, 2 },
    /* mov bp[$xx],di  */ { Mnemonic::MOV, AMIndexed, Register::bp, AMRegister, Register::di, 2 },
    /* mov si[$xx],di  */ { Mnemonic::MOV, AMIndexed, Register::si, AMRegister, Register::di, 2 },
    /* mov bp[$xx],a   */ { Mnemonic::MOV, AMIndexed, Register::bp, AMRegister, Register::a, 2 },
    /* mov bp[$xx],b   */ { Mnemonic::MOV, AMIndexed, Register::bp, AMRegister, Register::b, 2 },
    /* mov bp[$xx],c   */ { Mnemonic::MOV, AMIndexed, Register::bp, AMRegister, Register::c, 2 },
    /* mov bp[$xx],d   */ { Mnemonic::MOV, AMIndexed, Register::bp, AMRegister, Register::d, 2 },
    /* push bp         */ { Mnemonic::PUSH, AMRegister, Register::bp, AMNone, Register::None, 1 },
    /* pop bp          */ { Mnemonic::POP, AMRegister, Register::bp, AMNone, Register::None, 1 },
    /* push #$xxxx     */ { Mnemonic::PUSH, AMImmediate, Register::None, AMNone, Register::None, 2 },
    /* push #$xxxx     */ { Mnemonic::PUSHW, AMImmediate, Register::None, AMNone, Register::None, 3 },
    /* push ab         */ { Mnemonic::PUSH, AMRegister, Register::ab, AMNone, Register::None, 1 },
    /* push cd         */ { Mnemonic::PUSH, AMRegister, Register::cd, AMNone, Register::None, 1 },
    /* push bp[$xx]    */ { Mnemonic::PUSH, AMIndexed, Register::bp, AMNone, Register::None, 2 },
    /* pop ab          */ { Mnemonic::POP, AMRegister, Register::ab, AMNone, Register::None, 1 },
    /* pop cd          */ { Mnemonic::POP, AMRegister, Register::cd, AMNone, Register::None, 1 },
    /* pop bp[$xx]     */ { Mnemonic::POP, AMIndexed, Register::bp, AMNone, Register::None, 2 },

    { /* 220 */ },
    { /* 221 */ },
    { /* 222 */ },
    { /* 223 */ },
    { /* 224 */ },
    { /* 225 */ },
    { /* 226 */ },
    { /* 227 */ },
    { /* 228 */ },
    { /* 229 */ },
    { /* 230 */ },
    { /* 231 */ },
    { /* 232 */ },
    { /* 233 */ },
    { /* 234 */ },
    { /* 235 */ },
    { /* 236 */ },
    { /* 237 */ },
    { /* 238 */ },
    { /* 239 */ },
    { /* 240 */ },
    { /* 241 */ },
    { /* 242 */ },
    { /* 243 */ },
    { /* 244 */ },
    { /* 245 */ },
    { /* 246 */ },
    { /* 247 */ },
    { /* 248 */ },
    { /* 249 */ },
    { /* 250 */ },
    { /* 251 */ },
    { /* 252 */ },

    /* rti           */ { Mnemonic::RTI, AMNone, Register::None, AMNone, Register::None, 1 },
    /* nmi #$xxxx    */ { Mnemonic::NMI, AMImmediate, Register::None, AMNone, Register::None, 3 },
    /* hlt           */ { Mnemonic::HLT, AMNone, Register::None, AMNone, Register::None, 1 },
};

std::optional<OpcodeDefinition> get_opcode_definition(Mnemonic m, Argument const& target, Argument const& source)
{
    for (int ix = 0; ix < sizeof(opcode_definitions); ++ix) {
        if (opcode_definitions[ix].mnemonic != m)
            continue;
        if ((opcode_definitions[ix].am_source != source.addressing_mode) || (opcode_definitions[ix].am_target != target.addressing_mode))
            continue;
        if ((target.reg != opcode_definitions[ix].target) || (source.reg != opcode_definitions[ix].source))
            continue;
        auto ret = opcode_definitions[ix];
        ret.opcode = ix;
        return ret;
    }
    return {};
}

std::optional<uint16_t> Argument::constant_value(Image const& image) const
{
    if (addressing_mode == AMIndexed) {
        return constant;
    }
    if (addressing_mode == AMNone) {
        return {};
    }
    if (addressing_mode & AMImmediate) {
        switch (immediate_type) {
        case ImmediateType::Label:
            return image.label_value(label);
        case ImmediateType::Constant:
            return constant;
        default:
            return {};
        }
    }
    return {};
}

}
