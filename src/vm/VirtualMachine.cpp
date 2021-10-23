/*
 * vm.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <optional>
#include <unistd.h>

#include <core/Arguments.h>
#include <core/NativeFunction.h>
#include <vm/VirtualMachine.h>

namespace Obelix {

InstructionCode InstructionCode_by_name(char const* name)
{
#undef __ENUM_InstructionCode_V
#define __ENUM_InstructionCode_V(value) \
    if (!strcmp(name, #value)) {        \
        return InstructionCode::value;  \
    }
    __ENUM_InstructionCode(__ENUM_InstructionCode_V)
#undef __ENUM_InstructionCode_V
        return (InstructionCode)-1;
}

InstructionResult Instruction::execute(VirtualMachine& vm)
{
    switch (m_code) {
    case InstructionCode::Nop:
        break;
    case InstructionCode::PushValue: {
        auto ret = vm.push(m_operand);
        if (ret != InstructionResultCode::Success)
            return { ret, m_operand };
    } break;
    case InstructionCode::PushRegister: {
        auto value = vm.reg_value(m_operand);
        vm.push(value);
    } break;
    case InstructionCode::PushFromMemory: {
        auto value = vm.mem_value(m_operand, ValueType::Int);
        if (value.type == ValueType::Error) {
            return InstructionResult { value.error_code, m_operand };
        }
        vm.push(value);
    } break;
    case InstructionCode::PopToRegister: {
        auto value = vm.pop_value();
        if (value.code != InstructionResultCode::Success) {
            return value;
        }
        auto result = vm.assign_reg(m_operand, value.value);
        if (result != InstructionResultCode::Success) {
            return InstructionResult { result, m_operand };
        }
    } break;
    case InstructionCode::PopToMemory: {
        auto value = vm.pop_value();
        if (value.code == InstructionResultCode::Success) {
            return value;
        }
        auto result = vm.assign_mem(m_operand, value.value);
        if (result != InstructionResultCode::Success) {
            return InstructionResult { result, m_operand };
        }
    } break;
    case InstructionCode::Dup: {
        if (m_operand.type != ValueType::Int)
            return { InstructionResultCode::TypeMismatch, m_operand };
        auto ret = vm.dup(m_operand.int_value);
        if (ret != InstructionResultCode::Success)
            return { ret, m_operand };
    } break;
    case InstructionCode::IncRegister: {
        auto value = vm.reg_value(m_operand);
        if (value.type == ValueType::Error)
            return { value.error_code, value };
        if (value.type != ValueType::Int)
            return { InstructionResultCode::TypeMismatch, value };
        auto result = vm.assign_reg(m_operand, VMValue { ValueType::Int, { value.int_value + 1 } });
        if (result != InstructionResultCode::Success)
            return InstructionResult { result, m_operand };
    } break;
    case InstructionCode::DecRegister: {
        auto value = vm.reg_value(m_operand);
        if (value.type == ValueType::Error)
            return { value.error_code, value };
        if (value.type != ValueType::Int)
            return { InstructionResultCode::TypeMismatch, value };
        auto result = vm.assign_reg(m_operand, VMValue { ValueType::Int, { value.int_value - 1 } });
        if (result != InstructionResultCode::Success)
            return InstructionResult { result, m_operand };
    } break;
    case InstructionCode::Add: {
        auto value1 = vm.pop_int();
        auto value2 = vm.pop_int();
        vm.push(VMValue { .type = ValueType::Int, .int_value = (value1.value.int_value + value2.value.int_value) });
    } break;
    case InstructionCode::AddRegisters: {
        auto values = vm.reg_values(m_operand);
        if (values.first.type == ValueType::Error) {
            return InstructionResult { values.first.error_code, m_operand };
        }
        if (values.first.type != ValueType::Int) {
            return InstructionResult { InstructionResultCode::TypeMismatch, values.first };
        }
        if (values.second.type != ValueType::Int) {
            return InstructionResult { InstructionResultCode::TypeMismatch, values.second };
        }
        vm.assign_reg(m_operand.register_pair.reg1,
            VMValue { .type = ValueType::Int, .int_value = (values.first.int_value + values.second.int_value) });
    } break;
    case InstructionCode::Sub: {
        auto value1 = vm.pop_int();
        auto value2 = vm.pop_int();
        vm.push(VMValue { .type = ValueType::Int, .int_value = (value2.value.int_value - value1.value.int_value) });
    } break;
    case InstructionCode::SubRegisters: {
        auto values = vm.reg_values(m_operand);
        if (values.first.type == ValueType::Error) {
            return InstructionResult { values.first.error_code, m_operand };
        }
        if (values.first.type != ValueType::Int) {
            return InstructionResult { InstructionResultCode::TypeMismatch, values.first };
        }
        if (values.second.type != ValueType::Int) {
            return InstructionResult { InstructionResultCode::TypeMismatch, values.second };
        }
        vm.assign_reg(m_operand.register_pair.reg1,
            VMValue { .type = ValueType::Int, .int_value = (values.first.int_value - values.second.int_value) });
    } break;
    case InstructionCode::NativeCall: {
        if (m_operand.type != ValueType::Pointer)
            return InstructionResult { InstructionResultCode::TypeMismatch, m_operand };
        auto function_ptr = (native_t)m_operand.pointer_value;
        auto num_args = vm.pop_int();
        if (num_args.code == InstructionResultCode::Success) {
            return num_args;
        }
        Ptr<Arguments> args = make_typed<Arguments>();
        for (auto ix = 0u; ix < num_args.value.int_value; ix++) {
            auto value = vm.pop_value();
            if (value.code == InstructionResultCode::Success) {
                return value;
            }
            Obj obj;
            switch (value.value.type) {
            case ValueType::Int:
                obj = make_obj<Integer>(value.value.int_value);
                break;
            case ValueType::Float:
                obj = make_obj<Float>(value.value.float_value);
                break;
            case ValueType::Pointer:
                obj = make_obj<String>((char const*)value.value.pointer_value);
                break;
            default:
                return InstructionResult { InstructionResultCode::TypeMismatch, value.value };
            }
            args->add(obj);
        }
        Obj ret;
        ((native_t)function_ptr)("**function**", &args, &ret);
        auto ret_value_maybe = ret.to_long();
        long ret_value = (ret_value_maybe.has_value()) ? ret_value_maybe.value() : 0L;
        return InstructionResult { InstructionResultCode::Success, VMValue { ValueType::Int, { ret_value } } };
    };
    case InstructionCode::Halt: {
        auto exit_code = vm.pop_value();
        if (exit_code.code != InstructionResultCode::Success) {
            return exit_code;
        }
        return InstructionResult { InstructionResultCode::HaltVM, exit_code.value };
    } break;
    case InstructionCode::Jump: {
        return InstructionResult { InstructionResultCode::JumpTo, m_operand };
    } break;
    case InstructionCode::JumpIf: {
        auto condition = vm.pop_int();
        if (condition.code != InstructionResultCode::Success) {
            return condition;
        }
        if (condition.value.int_value) {
            return InstructionResult { InstructionResultCode::JumpTo, m_operand };
        }
    } break;
    case InstructionCode::JumpZero: {
        auto condition = vm.pop_int();
        if (condition.code != InstructionResultCode::Success) {
            return condition;
        }
        if (condition.value.int_value == 0) {
            return InstructionResult { InstructionResultCode::JumpTo, m_operand };
        }
    } break;
    default:
        auto err = (long)m_code;
        return InstructionResult { InstructionResultCode::ValueError, { ValueType::Int, { err } } };
    }
    return {};
}

void VirtualMachine::push(Instruction const& instruction)
{
    m_instructions.push_back(instruction);
}

InstructionResultCode VirtualMachine::push(VMValue const& value)
{
    if (m_stack.size() > MAX_STACK_SIZE) {
        return InstructionResultCode::StackOverflow;
    }
    m_stack.push_back(value);
    return InstructionResultCode::Success;
}

VMValue VirtualMachine::pop()
{
    if (m_stack.empty()) {
        return VMValue { ValueType::Error, { (int)InstructionResultCode::StackUnderflow } };
    }
    auto ret = m_stack.back();
    m_stack.pop_back();
    return ret;
}

InstructionResult VirtualMachine::pop_value()
{
    auto value = pop();
    if (value.type == ValueType::Error)
        return InstructionResult { value.error_code, value };
    return InstructionResult { InstructionResultCode::Success, value };
};

InstructionResult VirtualMachine::pop_int()
{
    auto value = pop();
    if (value.type == ValueType::Error)
        return InstructionResult { value.error_code, value };
    if (value.type != ValueType::Int)
        return InstructionResult { InstructionResultCode::TypeMismatch, value };
    return InstructionResult { InstructionResultCode::Success, value };
};

InstructionResult VirtualMachine::pop_pointer()
{
    auto value = pop();
    if (value.type == ValueType::Error)
        return InstructionResult { value.error_code, value };
    if (value.type != ValueType::Pointer)
        return InstructionResult { InstructionResultCode::TypeMismatch, value };
    if (!is_valid_pointer(value)) {
        return InstructionResult { InstructionResultCode::PointerOutOfBounds, value };
    }
    return InstructionResult { InstructionResultCode::Success, value };
};

InstructionResultCode VirtualMachine::dup(uint32_t depth)
{
    if (depth >= m_stack.size())
        return InstructionResultCode::StackUnderflow;
    auto value = m_stack[m_stack.size() - depth - 1];
    m_stack.push_back(value);
    return InstructionResultCode::Success;
}

bool VirtualMachine::is_valid_pointer(VMValue const& value) const
{
    if ((value.type == ValueType::Pointer) && (value.pointer_value < MEM_SIZE))
        return true;
    if ((value.type == ValueType::Int) && (value.int_value >= 0) && (value.int_value < MEM_SIZE))
        return true;
    return false;
}

bool VirtualMachine::is_valid_register(uint32_t value) const
{
    return value < NUM_REGS;
}

bool VirtualMachine::is_valid_register(VMValue const& value) const
{
    if ((value.type != ValueType::Int) || (value.int_value < 0) || (value.int_value >= NUM_REGS))
        return false;
    return true;
}

bool VirtualMachine::is_valid_register_pair(VMValue const& value) const
{
    if (value.type != ValueType::Registers)
        return false;
    if ((value.register_pair.reg1 >= NUM_REGS) || (value.register_pair.reg2 >= NUM_REGS))
        return false;
    return true;
}

VMValue VirtualMachine::reg_value(uint32_t reg) const
{
    if (!is_valid_register(reg))
        return VMValue { ValueType::Error, { (int)InstructionResultCode::ValueError } };
    return m_registers[reg];
}

VMValue VirtualMachine::reg_value(VMValue const& reg) const
{
    if (!is_valid_register(reg))
        return VMValue { ValueType::Error, { (int)InstructionResultCode::ValueError } };
    return reg_value(reg.int_value);
}

std::pair<VMValue, VMValue> VirtualMachine::reg_values(VMValue const& regs) const
{
    if (!is_valid_register_pair(regs))
        return { VMValue { ValueType::Error, { (int)InstructionResultCode::ValueError } },
            VMValue { ValueType::Error, { (int)InstructionResultCode::ValueError } } };
    return { m_registers[regs.register_pair.reg1], m_registers[regs.register_pair.reg2] };
}

InstructionResultCode VirtualMachine::assign_reg(uint32_t reg, VMValue const& value)
{
    if (!is_valid_register(reg))
        return InstructionResultCode::ValueError;
    m_registers[reg] = value;
    return InstructionResultCode::Success;
}

InstructionResultCode VirtualMachine::assign_reg(VMValue const& reg, VMValue const& value)
{
    if (!is_valid_register(reg))
        return InstructionResultCode::ValueError;
    m_registers[reg.int_value] = value;
    return InstructionResultCode::Success;
}

VMValue VirtualMachine::mem_value(VMValue const& address, ValueType type)
{
    if (!is_valid_pointer(address))
        return VMValue { ValueType::Error, { (int)InstructionResultCode::PointerOutOfBounds } };

    VMValue ret { type, { 0 } };
    ret.pointer_value = 0;
    size_t addr = address.pointer_value;
    for (int ix = 0; ix < 8; ix++) {
        ret.pointer_value <<= 8;
        ret.pointer_value |= m_heap[addr++];
    }
    return ret;
}

InstructionResultCode VirtualMachine::assign_mem(VMValue const& address, VMValue const& value)
{
    if (!is_valid_pointer(address))
        return InstructionResultCode::PointerOutOfBounds;
    size_t addr = address.pointer_value;
    size_t val = value.pointer_value;
    for (int ix = 0; ix < 8; ix++) {
        m_heap[addr++] = val & 0xFF;
        val >>= 8;
    }
    return InstructionResultCode::Success;
}

VMValue VirtualMachine::run()
{
    for (size_t ip = 0; ip < m_instructions.size();) {
        auto instruction = m_instructions[ip];
        auto result = instruction.execute(*this);
        switch (result.code) {
        case InstructionResultCode::Success:
            ip++;
            break;
        case InstructionResultCode::JumpTo:
            ip = result.value.pointer_value;
            break;
        case InstructionResultCode::HaltVM:
            return result.value;
        default:
            fprintf(stderr, "VM CRASH: %ld:%ld\n", (long)result.code, result.value.int_value);
            return VMValue { ValueType::Error, { -1 } };
        }
    }
    return VMValue { ValueType::Int, { 0 } };
}

void VirtualMachine::dump()
{
    for (int ix = 0; ix < NUM_REGS / 2; ix++) {
        VMValue value = m_registers[ix];
        fprintf(stderr, "Reg%02d %s %lu ", ix, ValueType_name(value.type), value.int_value);
    }
    fprintf(stderr, "\n");
    for (int ix = NUM_REGS / 2; ix < NUM_REGS; ix++) {
        auto value = m_registers[ix];
        fprintf(stderr, "Reg%02d %s %lu ", ix, ValueType_name(value.type), value.int_value);
    }
    fprintf(stderr, "\n\nStack:\n");
    for (size_t ix = m_stack.size() - 1; (int)ix >= 0; ix--) {
        auto value = m_stack[ix];
        fprintf(stderr, "%s %lu\n", ValueType_name(value.type), value.int_value);
    }
}

}
