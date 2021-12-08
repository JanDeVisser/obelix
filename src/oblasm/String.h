/*
 * String.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

class String : public Entry {
public:
    enum class Type {
        ASCII,
        ASCIZ
    };

    constexpr static char const* Type_name(Type type)
    {
        switch (type) {
        case Type::ASCII:
            return "ascii";
        case Type::ASCIZ:
            return "asciz";
        }
    }

    explicit String(Mnemonic mnemonic, std::string const& str);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] uint16_t size() const override;
    void append_to(Image& image) override;

private:
    Type m_type { Type::ASCII };
    std::string m_str;
};

}
