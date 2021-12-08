/*
 * Buffer.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <oblasm/Buffer.h>
#include <oblasm/Image.h>

namespace Obelix::Assembler {

Buffer::Buffer(Mnemonic mnemonic, std::string const& size)
    : Entry(mnemonic, size)
{
    auto size_maybe = to_ulong(size);
    if (!size_maybe.has_value()) {
        add_error(format("Could not parse Buffer size '{}'", size));
        return;
    }
    auto sz = size_maybe.value();
    if (sz == 0) {
        add_error("Buffer size must be bigger than 0");
        return;
    }
    if (sz > 256) {
        add_error(format("Buffer size can be at most 256, so {} is too large", sz));
        return;
    }
    m_size = sz;
}

std::string Buffer::to_string() const
{
    return format("buffer 0x{02x}", m_size);
}

uint16_t Buffer::size() const
{
    return m_size;
}

void Buffer::append_to(Image& image)
{
    for (auto ix = 0; ix < m_size; ++ix)
        image.append(0);
}

}
