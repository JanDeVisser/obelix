/*
 * String.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/Format.h>
#include <oblasm/Image.h>
#include <oblasm/String.h>

namespace Obelix::Assembler {

String::String(Mnemonic mnemonic, std::string const& str)
    : Entry(mnemonic, str)
    , m_str(str)
{
    switch (mnemonic) {
    case Mnemonic::ASCIZ:
        m_type = Type::ASCIZ;
        break;
    case Mnemonic::STR:
        m_type = Type::ASCII;
        break;
    default:
        assert(false && "Unrecognized mnemonic in String handler");
        break;
    }
}

std::string String::to_string() const
{
    return format("{} {}", Type_name(m_type), m_str);
}

uint16_t String::size() const
{
    auto ret = m_str.length();
    if (m_type == Type::ASCIZ)
        ++ret;
    return ret;
}

void String::append_to(Image& image)
{
    image.append(m_str);
    if (m_type == Type::ASCIZ)
        image.append(0);
}

}
