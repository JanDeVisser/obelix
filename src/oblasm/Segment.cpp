/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <oblasm/Image.h>
#include <oblasm/Segment.h>

namespace Obelix::Assembler {

Segment::Segment(uint16_t start_address)
    : Entry(Mnemonic::Segment, format("org={}", start_address))
    , m_start_address(start_address)
{
}

Segment::Segment(std::string const& start_address)
    : Entry(Mnemonic::Segment, format("org={}", start_address))
{
    auto long_maybe = to_long(start_address);
    if (!long_maybe.has_value()) {
        add_error(format("Invalid segment start address '{}'", start_address));
        return;
    }
    m_start_address = long_maybe.value();
}

std::string Segment::to_string() const
{
    return format(".segment 0x{04x}", start_address());
}

void Segment::add(std::shared_ptr<Entry> const& entry)
{
    m_entries.push_back(entry);
    m_size += entry->size();
}

void Segment::append_to(Image& image)
{
    auto addr = start_address();
    image.set_address(addr);
    for (auto& entry : entries()) {
        entry->append_to(image);
        addr += entry->size();
        add_errors(entry->errors());
    }
}

}
