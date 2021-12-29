/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <optional>
#include <string>

#include <lexer/Token.h>
#include <oblasm/Opcode.h>

namespace Obelix::Assembler {

#define __ENUM_InstructionTemplate(S) \
    S(NoArgs)                         \
    S(Move)                        \
    S(OneArg)                      \
    S(TwoArg)                      \
    S(Jump)                        \
    S(Bytes)                       \
    S(Buffer)                      \
    S(String)

enum class InstructionTemplate {
#undef __InstructionTemplate
#define __InstructionTemplate(value) value,
    __ENUM_InstructionTemplate(__InstructionTemplate)
#undef __InstructionTemplate
};

constexpr inline char const* InstructionTemplate_name(InstructionTemplate instr_template)
{
    switch (instr_template) {
#undef __InstructionTemplate
#define __InstructionTemplate(value) \
    case InstructionTemplate::value: \
        return #value;
        __ENUM_InstructionTemplate(__InstructionTemplate)
#undef __InstructionTemplate
    }
}

#define ENUM_Mnemonic(S)  \
    S(CLRFL, NoArgs, 0)   \
    S(HLT, NoArgs, 1)     \
    S(NOP, NoArgs, 2)     \
    S(POPFL, NoArgs, 3)   \
    S(PUSHFL, NoArgs, 4)  \
    S(RET, NoArgs, 5)     \
    S(RTI, NoArgs, 6)     \
    S(IN, NoArgs, 7)      \
    S(MOV, Move, 8)       \
    S(OUT, Move, 9)       \
    S(CLR, OneArg, 10)    \
    S(DEC, OneArg, 11)    \
    S(INC, OneArg, 12)    \
    S(NOT, OneArg, 13)    \
    S(PUSH, OneArg, 14)   \
    S(PUSHW, OneArg, 15)  \
    S(POP, OneArg, 16)    \
    S(SHL, OneArg, 17)    \
    S(SHR, OneArg, 18)    \
    S(ADC, TwoArg, 19)    \
    S(ADD, TwoArg, 20)    \
    S(AND, Move, 21)      \
    S(CMP, Move, 22)      \
    S(OR, Move, 23)       \
    S(SUB, TwoArg, 24)    \
    S(SBB, TwoArg, 25)    \
    S(SWP, TwoArg, 26)    \
    S(XOR, TwoArg, 27)    \
    S(CALL, Jump, 28)     \
    S(JC, Jump, 29)       \
    S(JV, Jump, 30)       \
    S(JNZ, Jump, 32)      \
    S(JMP, Jump, 33)      \
    S(JZ, Jump, 34)       \
    S(NMI, Jump, 35)      \
    S(DB, Bytes, 36)      \
    S(DW, Bytes, 37)      \
    S(DDW, Bytes, 38)     \
    S(DLW, Bytes, 39)     \
    S(BUFFER, Buffer, 40) \
    S(ASCIZ, String, 41)  \
    S(STR, String, 42)

enum class Mnemonic {
    None = -1,
#undef __Mnemonic
#define __Mnemonic(value, arg_template, num) value = num,
    ENUM_Mnemonic(__Mnemonic)
#undef __Mnemonic
    Label,
    Define,
    Align,
    Segment,
    Include,
};

constexpr inline char const* Mnemonic_name(Mnemonic mnemonic)
{
    switch (mnemonic) {
    case Mnemonic::None:
        return "None";
    case Mnemonic::Label:
        return "Label";
    case Mnemonic::Define:
        return "Define";
    case Mnemonic::Align:
        return "Align";
    case Mnemonic::Segment:
        return "Segment";
    case Mnemonic::Include:
        return "Include";
#undef __Mnemonic
#define __Mnemonic(value, arg_template, num) \
    case Mnemonic::value:                    \
        return #value;
        ENUM_Mnemonic(__Mnemonic)
#undef __Mnemonic
    }
}

inline std::optional<Mnemonic> Mnemonic_for_name(std::string const& name)
{
    std::string upr;
    for (auto& ch : name) {
        upr += toupper(ch);
    }
    if (upr == "NONE")
        return Mnemonic::None;
    if (upr == "LABEL")
        return Mnemonic::Label;
    if (upr == "DEFINE")
        return Mnemonic::Define;
    if (upr == "ALIGN")
        return Mnemonic::Align;
    if (upr == "SEGMENT")
        return Mnemonic::Segment;
    if (upr == "INCLUDE")
        return Mnemonic::Include;
#undef __Mnemonic
#define __Mnemonic(value, arg_template, num) \
    if (upr == #value) \
        return Mnemonic::value;
    ENUM_Mnemonic(__Mnemonic)
#undef __Mnemonic
    return {};
}

constexpr static TokenCode KeywordCLRFL = TokenCode::Keyword0;
constexpr static TokenCode KeywordHLT = TokenCode::Keyword1;
constexpr static TokenCode KeywordNOP = TokenCode::Keyword2;
constexpr static TokenCode KeywordPOPFL = TokenCode::Keyword3;
constexpr static TokenCode KeywordPUSHFL = TokenCode::Keyword4;
constexpr static TokenCode KeywordRET = TokenCode::Keyword5;
constexpr static TokenCode KeywordRTI = TokenCode::Keyword6;
constexpr static TokenCode KeywordIN = TokenCode::Keyword7;
constexpr static TokenCode KeywordMOV = TokenCode::Keyword8;
constexpr static TokenCode KeywordOUT = TokenCode::Keyword9;
constexpr static TokenCode KeywordCLR = TokenCode::Keyword10;
constexpr static TokenCode KeywordDEC = TokenCode::Keyword11;
constexpr static TokenCode KeywordINC = TokenCode::Keyword12;
constexpr static TokenCode KeywordNOT = TokenCode::Keyword13;
constexpr static TokenCode KeywordPUSH = TokenCode::Keyword14;
constexpr static TokenCode KeywordPUSHW = TokenCode::Keyword15;
constexpr static TokenCode KeywordPOP = TokenCode::Keyword16;
constexpr static TokenCode KeywordSHL = TokenCode::Keyword17;
constexpr static TokenCode KeywordSHR = TokenCode::Keyword18;
constexpr static TokenCode KeywordADC = TokenCode::Keyword19;
constexpr static TokenCode KeywordADD = TokenCode::Keyword20;
constexpr static TokenCode KeywordAND = TokenCode::Keyword21;
constexpr static TokenCode KeywordCMP = TokenCode::Keyword22;
constexpr static TokenCode KeywordOR = TokenCode::Keyword23;
constexpr static TokenCode KeywordSUB = TokenCode::Keyword24;
constexpr static TokenCode KeywordSBB = TokenCode::Keyword25;
constexpr static TokenCode KeywordSWP = TokenCode::Keyword26;
constexpr static TokenCode KeywordXOR = TokenCode::Keyword27;
constexpr static TokenCode KeywordCALL = TokenCode::Keyword28;
constexpr static TokenCode KeywordJC = TokenCode::Keyword29;
constexpr static TokenCode KeywordJV = TokenCode::Keyword30;
constexpr static TokenCode KeywordJNZ = TokenCode::Keyword32;
constexpr static TokenCode KeywordJMP = TokenCode::Keyword33;
constexpr static TokenCode KeywordJZ = TokenCode::Keyword34;
constexpr static TokenCode KeywordNMI = TokenCode::Keyword35;
constexpr static TokenCode KeywordDB = TokenCode::Keyword36;
constexpr static TokenCode KeywordDW = TokenCode::Keyword37;
constexpr static TokenCode KeywordDDW = TokenCode::Keyword38;
constexpr static TokenCode KeywordDLW = TokenCode::Keyword39;
constexpr static TokenCode KeywordBUFFER = TokenCode::Keyword40;
constexpr static TokenCode KeywordASCIZ = TokenCode::Keyword41;
constexpr static TokenCode KeywordSTR = TokenCode::Keyword42;

// Directives:
constexpr static TokenCode KeywordSegment = TokenCode::Keyword40;
constexpr static TokenCode KeywordDefine = TokenCode::Keyword41;
constexpr static TokenCode KeywordInclude = TokenCode::Keyword42;
constexpr static TokenCode KeywordAlign = TokenCode::Keyword43;

inline std::optional<Mnemonic> get_mnemonic(TokenCode code)
{
    switch (code) {
#undef __Mnemonic
#define __Mnemonic(value, arg_template, num) \
    case TokenCode::Keyword##num:            \
        return Mnemonic::value;
        ENUM_Mnemonic(__Mnemonic)
#undef __Mnemonic
            default : return {};
    }
}

inline constexpr InstructionTemplate get_template(Mnemonic mnemonic)
{
    switch (mnemonic) {
#undef __Mnemonic
#define __Mnemonic(value, instr_template, num) \
    case Mnemonic::value:                    \
        return InstructionTemplate::instr_template;
        ENUM_Mnemonic(__Mnemonic)
#undef __Mnemonic
    default:
        fatal("Unreachable");
    }
}

enum class Register {
    None = -1,
    a,
    b,
    c,
    d,
    ab,
    cd,
    si,
    di,
    sp,
    pc,
    bp,
    flags,
};

struct RegisterDefinition {
    Register reg;
    char const* name;
    uint8_t bits;
    int token_code;
};

constexpr static TokenCode KeywordA = TokenCode::Keyword44;
constexpr static TokenCode KeywordB = TokenCode::Keyword45;
constexpr static TokenCode KeywordC = TokenCode::Keyword46;
constexpr static TokenCode KeywordD = TokenCode::Keyword47;
constexpr static TokenCode KeywordAB = TokenCode::Keyword48;
constexpr static TokenCode KeywordCD = TokenCode::Keyword49;
constexpr static TokenCode KeywordSi = TokenCode::Keyword50;
constexpr static TokenCode KeywordDi = TokenCode::Keyword51;
constexpr static TokenCode KeywordSP = TokenCode::Keyword52;
constexpr static TokenCode KeywordBP = TokenCode::Keyword53;

std::optional<RegisterDefinition> get_register(char const*);
std::optional<RegisterDefinition> get_register(std::string const&);
std::optional<RegisterDefinition> get_register(TokenCode);
RegisterDefinition& get_definition(Register);
bool is_register(char const*);
bool is_register(std::string const&);
bool is_register(TokenCode);

enum AddressingMode : uint8_t {
    AMNone = 0x00,              // Nothing. No argument.
    AMRegister = 0x01,          // Straight register value
    AMImmediate = 0x02,         // Immediate, constant, value
    AMIndirect = 0x04,          // Dereference the register or immediate value
    AMRegisterIndirect = 0x05,  // Combination of Register and Indirect
    AMImmediateIndirect = 0x06, // Combination of Immediate and Indirect
    AMIndexed = 0x08,           // Indexed. Index the pointer held by the register with the constant value
};

struct Argument {
    enum class ImmediateType {
        None,
        Constant,
        Label
    };
    AddressingMode addressing_mode { AMNone };
    ImmediateType immediate_type { ImmediateType::None };
    uint16_t constant { 0 };
    Register reg { Register::None };
    std::string label;

    [[nodiscard]] std::string to_string(uint8_t bytes) const
    {
        if (addressing_mode == AMIndexed) {
            auto r = get_definition(reg);
            return format("{}[$02x]", r.name, constant);
        }
        if (addressing_mode == AMNone) {
            return "";
        }
        std::string ret;
        if (addressing_mode & AMIndirect)
            ret += "*";
        if (addressing_mode & AMImmediate) {
            switch (immediate_type) {
            case ImmediateType::Label:
                ret += "%" + label;
                break;
            case ImmediateType::Constant:
                ret += format(format("${{0{}x}", bytes * 2), constant);
                break;
            default:
                fatal("Unreachable");
            }
        }
        if (addressing_mode & AMRegister) {
            assert(reg != Register::None);
            auto r = get_definition(reg);
            ret += r.name;
        }
        return ret;
    }

    [[nodiscard]] bool valid() const
    {
        if (addressing_mode == AMNone)
            return (immediate_type == ImmediateType::None) && (reg == Register::None);
        if (addressing_mode == AMIndexed)
            return (immediate_type != ImmediateType::None) && (reg != Register::None);
        if (addressing_mode & AMImmediate) {
            if ((addressing_mode != AMImmediate) && (addressing_mode != AMImmediateIndirect))
                return false;
            return (immediate_type != ImmediateType::None) && (reg == Register::None);
        }
        if (addressing_mode & AMRegister) {
            if ((addressing_mode != AMRegister) && (addressing_mode != AMRegisterIndirect))
                return false;
            return (immediate_type == ImmediateType::None) && (reg != Register::None);
        }
        return false;
    }

    [[nodiscard]] std::optional<uint16_t> constant_value(class Image const&) const;
};

struct OpcodeDefinition {
    Mnemonic mnemonic { Mnemonic::None };
    AddressingMode am_target { AMNone };
    Register target { Register::None };
    AddressingMode am_source { AMNone };
    Register source { Register::None };
    uint8_t bytes { 0 };
    uint8_t opcode { 0 };
};

std::optional<OpcodeDefinition> get_opcode_definition(Mnemonic m, Argument const& target, Argument const& src);

}
