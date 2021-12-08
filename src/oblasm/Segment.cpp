/*
 * Segment.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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
