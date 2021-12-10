/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
