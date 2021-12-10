/*
 * Instruction.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#pragma once

#include <optional>

#include <core/Format.h>
#include <core/StringUtil.h>
#include <oblasm/AssemblyTypes.h>
#include <oblasm/Buffer.h>
#include <oblasm/Bytes.h>
#include <oblasm/Directive.h>
#include <oblasm/String.h>

namespace Obelix::Assembler {

class Instruction : public Entry {
public:
    explicit Instruction(Mnemonic);
    Instruction(Mnemonic, Argument);
    Instruction(Mnemonic, Argument, Argument);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] uint16_t size() const override;
    void append_to(Image& image) override;
    [[nodiscard]] bool valid() const;
    [[nodiscard]] Argument const& target() const { return m_target; }
    [[nodiscard]] Argument const& source() const { return m_source; }
    [[nodiscard]] OpcodeDefinition const& definition() const { return m_definition; }

private:
    Argument m_target;
    Argument m_source;
    OpcodeDefinition m_definition;
};

}
