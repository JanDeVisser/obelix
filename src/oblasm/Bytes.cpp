/*
 * Bytes.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/StringUtil.h>
#include <oblasm/Bytes.h>
#include <oblasm/Image.h>

namespace Obelix::Assembler {

Bytes::Bytes(Mnemonic m, const std::string& args)
    : Entry(m, args)
{
    switch (m) {
    case Mnemonic::DB:
        m_width = Width::Byte;
        break;
    case Mnemonic::DW:
        m_width = Width::Word;
        break;
    case Mnemonic::DATA:
        m_width = Width::Byte;
        break;
    default:
        assert(false);
        break;
    }

}

void Bytes::append(uint8_t byte)
{
    m_bytes.push_back(byte);
    if (m_width != Width::Byte)
        m_bytes.push_back(0);
}

void Bytes::append(uint16_t word)
{
    m_bytes.push_back(word & 0xFF);
    if (m_width != Width::Byte)
        m_bytes.push_back(word >> 8);
}

void Bytes::append(uint32_t dword)
{
    append((uint16_t) (dword & 0x0000FFFF));
    if (m_width == Width::DWord)
        append((uint16_t) (dword >> 16));
}

void Bytes::append(const std::string& data)
{
    auto ulong_maybe = to_ulong(data);
    if (!ulong_maybe.has_value()) {
        add_error(format("Could not parse {} value '{}'", data, Mnemonic_name(mnemonic())));
        return;
    }
    append((uint32_t) ulong_maybe.value());
}

std::string Bytes::to_string() const
{
    return format("[{} bytes]", size());
}

void Bytes::append_to(Image& image)
{
    image.append(m_bytes);
}

}
