/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <regex>

#include <oblasm/Instruction.h>
#include <oblasm/Image.h>

namespace Obelix::Assembler {

Instruction::Instruction(Mnemonic m)
    : Entry(m, "")
{
    if (m_definition.mnemonic == Mnemonic::None)
        m_definition = get_opcode_definition(m, m_target, m_source);
    if (m_definition.mnemonic != mnemonic()) {
        add_error("Invalid opcode");
    }
}

Instruction::Instruction(Mnemonic m, Argument target)
    : Entry(m, "")
    , m_target(std::move(target))
{
    if (m_definition.mnemonic == Mnemonic::None)
        m_definition = get_opcode_definition(m, m_target, m_source);
    if (m_definition.mnemonic != mnemonic()) {
        add_error("Invalid opcode");
    }
}

Instruction::Instruction(Mnemonic m, Argument target, Argument source)
    : Entry(m, "")
    , m_target(std::move(target))
    , m_source(std::move(source))
{
    if (m_definition.mnemonic == Mnemonic::None)
        m_definition = get_opcode_definition(m, m_target, m_source);
    if (m_definition.mnemonic != mnemonic()) {
        add_error("Invalid opcode");
    }
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
       if (!constant_maybe.has_value()) {
           add_error(format("Could not resolve immediate value for {}", to_string()));
           return;
       }
        auto constant = constant_maybe.value();
        image.append((uint8_t) (constant & 0x00FF));
        if (m_definition.bytes > 2) {
            image.append((uint8_t) (constant >> 8));
        }
    }
}

}
