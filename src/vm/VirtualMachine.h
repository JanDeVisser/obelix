/*
 * vm.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <array>
#include <string>
#include <vector>

namespace Obelix {

#define __ENUM_InstructionCode(S) \
    S(Nop)                        \
    S(PushValue)                  \
    S(PushRegister)               \
    S(PushFromMemory)             \
    S(PopToRegister)              \
    S(PopToMemory)                \
    S(Dup)                        \
    S(IncRegister)                \
    S(DecRegister)                \
    S(Add)                        \
    S(AddRegisters)               \
    S(Sub)                        \
    S(SubRegisters)               \
    S(NativeCall)                 \
    S(Jump)                       \
    S(JumpIf)                     \
    S(JumpZero)                   \
    S(Halt)

enum class InstructionCode {
#undef __ENUM_InstructionCode_V
#define __ENUM_InstructionCode_V(value) \
    value,
    __ENUM_InstructionCode(__ENUM_InstructionCode_V)
#undef __ENUM_InstructionCode_V
};

constexpr char const* InstructionCode_name(InstructionCode code)
{
    switch (code) {
#undef __ENUM_InstructionCode_V
#define __ENUM_InstructionCode_V(value) \
    case InstructionCode::value:        \
        return #value;
        __ENUM_InstructionCode(__ENUM_InstructionCode_V)
#undef __ENUM_InstructionCode_V
            default : return "(Unknown InstructionCode)";
    }
}

InstructionCode InstructionCode_by_name(char const* name);

#define __ENUM_InstructionResultCode(S) \
    S(Success)                          \
    S(JumpTo)                           \
    S(PointerOutOfBounds)               \
    S(StackOverflow)                    \
    S(StackUnderflow)                   \
    S(TypeMismatch)                     \
    S(ValueError)                       \
    S(IllegalInstruction)               \
    S(HaltVM)

enum class InstructionResultCode {
#undef __ENUM_InstructionResultCode_V
#define __ENUM_InstructionResultCode_V(value) \
    value,
    __ENUM_InstructionResultCode(__ENUM_InstructionResultCode_V)
#undef __ENUM_InstructionResultCode_V
};

constexpr char const* InstructionResultCode_name(InstructionResultCode code)
{
    switch (code) {
#undef __ENUM_InstructionResultCode_V
#define __ENUM_InstructionResultCode_V(value) \
    case InstructionResultCode::value:        \
        return #value;
        __ENUM_InstructionResultCode(__ENUM_InstructionResultCode_V)
#undef __ENUM_InstructionResultCode_V
            default : return "(Unknown InstructionResultCode)";
    }
}

struct RegisterPair {
    uint32_t reg1;
    uint32_t reg2;
};

#define __ENUM_ValueType(S) \
    S(Int)                  \
    S(Float)                \
    S(Pointer)              \
    S(Error)                \
    S(Registers)

enum class ValueType {
#undef __ENUM_ValueType_V
#define __ENUM_ValueType_V(type) \
    type,
    __ENUM_ValueType(__ENUM_ValueType_V)
#undef _ENUM_ValueType_V
};

constexpr char const* ValueType_name(ValueType code)
{
    switch (code) {
#undef __ENUM_ValueType_V
#define __ENUM_ValueType_V(value) \
    case ValueType::value:        \
        return #value;
        __ENUM_ValueType(__ENUM_ValueType_V)
#undef __ENUM_ValueType_V
            default : return "(Unknown ValueType)";
    }
}

struct VMValue {
    ValueType type;
    union {
        long int_value;
        double float_value;
        uint64_t pointer_value;
        InstructionResultCode error_code;
        RegisterPair register_pair;
    };
};

struct InstructionResult {
    InstructionResultCode code { InstructionResultCode::Success };
    VMValue value { ValueType::Int, { 0 } };
};

class VirtualMachine;

class Instruction {
public:
    Instruction(InstructionCode code, VMValue operand)
        : m_code(code)
        , m_operand(operand)
    {
    }

    InstructionResult execute(VirtualMachine&);

private:
    std::string m_label;
    InstructionCode m_code;
    VMValue m_operand;
};

class VirtualMachine {
public:
    constexpr static size_t MAX_STACK_SIZE = 16 * 1024;
    constexpr static size_t MEM_SIZE = 1024 * 1024;
    constexpr static size_t NUM_REGS = 16;

    void push(Instruction const&);
    InstructionResultCode push(VMValue const&);
    [[nodiscard]] VMValue pop();
    [[nodiscard]] InstructionResult pop_value();
    [[nodiscard]] InstructionResult pop_int();
    [[nodiscard]] InstructionResult pop_pointer();
    [[nodiscard]] InstructionResultCode dup(uint32_t = 0);

    [[nodiscard]] bool is_valid_register(uint32_t) const;
    [[nodiscard]] bool is_valid_pointer(VMValue const&) const;
    [[nodiscard]] bool is_valid_register(VMValue const&) const;
    [[nodiscard]] bool is_valid_register_pair(VMValue const&) const;

    [[nodiscard]] VMValue reg_value(uint32_t) const;
    [[nodiscard]] VMValue reg_value(VMValue const&) const;
    [[nodiscard]] std::pair<VMValue, VMValue> reg_values(VMValue const&) const;
    InstructionResultCode assign_reg(uint32_t, VMValue const&);
    InstructionResultCode assign_reg(VMValue const&, VMValue const&);
    VMValue mem_value(VMValue const&, ValueType);
    InstructionResultCode assign_mem(VMValue const&, VMValue const&);

    VMValue run();
    void dump();

private:
    std::vector<Instruction> m_instructions {};
    std::vector<VMValue> m_stack {};
    std::array<VMValue, NUM_REGS> m_registers {};
    std::array<uint8_t, MEM_SIZE> m_heap {};
};

}
