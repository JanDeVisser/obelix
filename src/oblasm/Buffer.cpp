/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
