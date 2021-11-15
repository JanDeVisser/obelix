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

#include <vm/VirtualMachine.h>

void fibonacci(Obelix::VirtualMachine& vm)
{
    vm.push(Obelix::Instruction { Obelix::InstructionCode::PushValue, Obelix::VMValue { Obelix::ValueType::Int, { 20 } } });
    vm.push(Obelix::Instruction { Obelix::InstructionCode::PopToRegister, Obelix::VMValue { Obelix::ValueType::Int, { 0 } } });
    vm.push(Obelix::Instruction { Obelix::InstructionCode::PushValue, Obelix::VMValue { Obelix::ValueType::Int, { 0 } } });
    vm.push(Obelix::Instruction { Obelix::InstructionCode::PushValue, Obelix::VMValue { Obelix::ValueType::Int, { 1 } } });
    vm.push(Obelix::Instruction { Obelix::InstructionCode::Dup, Obelix::VMValue { Obelix::ValueType::Int, { 1 } } });
    vm.push(Obelix::Instruction { Obelix::InstructionCode::Dup, Obelix::VMValue { Obelix::ValueType::Int, { 1 } } });
    vm.push(Obelix::Instruction { Obelix::InstructionCode::Add, Obelix::VMValue { Obelix::ValueType::Int, { 0 } } });
    vm.push(Obelix::Instruction { Obelix::InstructionCode::DecRegister, Obelix::VMValue { Obelix::ValueType::Int, { 0 } } });
    vm.push(Obelix::Instruction { Obelix::InstructionCode::PushRegister, Obelix::VMValue { Obelix::ValueType::Int, { 0 } } });
    vm.push(Obelix::Instruction { Obelix::InstructionCode::JumpIf, Obelix::VMValue { Obelix::ValueType::Int, { 4 } } });
}

int main()
{
    Obelix::VirtualMachine vm;

    fibonacci(vm);
    vm.run();
    vm.dump();
    return 0;
}