/*
 * Instruction.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <regex>

#include <oblasm/Instruction.h>
#include <oblasm/Image.h>

namespace Obelix::Assembler {

Instruction::Instruction(Mnemonic mnemonic)
    : Entry(mnemonic, "")
{
    if (m_definition.mnemonic == Mnemonic::None)
        m_definition = get_opcode_definition(mnemonic, m_target, m_source);
}

Instruction::Instruction(Mnemonic mnemonic, Argument target)
    : Entry(mnemonic, "")
    , m_target(std::move(target))
{
    if (m_definition.mnemonic == Mnemonic::None)
        m_definition = get_opcode_definition(mnemonic, m_target, m_source);
}

Instruction::Instruction(Mnemonic mnemonic, Argument target, Argument source)
    : Entry(mnemonic, "")
    , m_target(std::move(target))
    , m_source(std::move(source))
{
    if (m_definition.mnemonic == Mnemonic::None)
        m_definition = get_opcode_definition(mnemonic, m_target, m_source);
}

std::string Instruction::to_string() const
{
    std::string ret = Mnemonic_name(mnemonic());
    if (m_target.valid()) {
        ret += " " + m_target.to_string(m_definition.bytes - 1);
        if (m_source.valid())
            ret += "," + m_source.to_string(m_definition.bytes - 1);
    }
    return ret;
}

uint16_t Instruction::size() const
{
    return m_definition.bytes;
}

void Instruction::append_to(Image& image)
{
    image.append(m_definition.opcode);
    if (m_definition.bytes > 1) {
        auto constant_maybe = m_target.constant_value(image);
        if (!constant_maybe.has_value())
            constant_maybe = m_source.constant_value(image);
        assert(constant_maybe.has_value());
        auto constant = constant_maybe.value();
        image.append((uint8_t) (constant & 0x00FF));
        if (m_definition.bytes > 2) {
            image.append((uint8_t) (constant >> 8));
        }
    }
}

bool Instruction::valid() const
{
    return m_definition.mnemonic == mnemonic();
}

}
