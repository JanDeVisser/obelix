/*
 * Bytes.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <oblasm/Directive.h>

namespace Obelix::Assembler {

class Bytes : public Entry {
public:
    enum class Width {
        Byte,  // 8 bits
        Word,  // 16 bits
        DWord  // 32 bits
    };

    Bytes(Mnemonic, std::string const&);
    void append(uint8_t);
    void append(uint16_t);
    void append(uint32_t);
    void append(std::string const& data);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] uint16_t size() const override { return m_bytes.size(); }
    void append_to(Image&) override;

private:
    Width m_width { Width::Byte };
    std::vector<uint8_t> m_bytes {};
};

}
