/*
 * Directive.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <oblasm/Directive.h>
#include <oblasm/Image.h>

namespace Obelix::Assembler {

std::string_view Entry::prefix() const
{
    return "";
}

std::string Entry::to_string() const
{
    return format("{} {}", Mnemonic_name(mnemonic()), join(arguments(), ' '));
}

uint16_t Entry::size() const
{
    return 0;
}

void Entry::append_to(Image&)
{
}

Label::Label(std::string const& label, uint16_t value)
    : Label(Mnemonic::Label, label, value)
{
}

Label::Label(std::string const& label, std::string const& value)
    : Label(Mnemonic::Label, label, value)
{
}

Label::Label(Mnemonic mnemonic, std::string const& label, uint16_t value)
    : Entry(mnemonic, format("{}={}", label, value))
    , m_label(label)
    , m_value(value)
{
    assert(!is_register(label));
}

Label::Label(Mnemonic mnemonic, std::string const& label, std::string const& value)
    : Entry(mnemonic, format("{}={}", label, value))
    , m_label(label)
{
    assert(!is_register(label));
    auto long_maybe = to_long(value);
    if (!long_maybe.has_value()) {
        add_error(format("Invalid directive value '{}'", value));
        return;
    }
    m_value = long_maybe.value();
}

Define::Define(std::string const& label, uint16_t value)
    : Label(label, value)
{
}

Define::Define(std::string const& label, std::string const& value)
    : Label(label, value)
{
}

Align::Align(uint16_t boundary)
    : Entry(Mnemonic::Align, format("boundary={}", boundary))
    , m_boundary(boundary)
{
}

Align::Align(std::string const& boundary)
    : Entry(Mnemonic::Align, format("boundary={}", boundary))
{
    auto long_maybe = to_long(boundary);
    if (!long_maybe.has_value()) {
        add_error(format("Invalid .align boundary '{}'", boundary));
        return;
    }
    m_boundary = long_maybe.value();
}

void Align::append_to(Image& image)
{
    image.align(boundary());
}

}
