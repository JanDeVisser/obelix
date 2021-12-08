/*
 * Image.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/Error.h>
#include <oblasm/Directive.h>
#include <oblasm/Segment.h>

namespace Obelix::Assembler {

std::vector<std::string> bytes_to_strings(std::vector<uint8_t>);

class Image {
public:
    explicit Image(uint16_t size = 16*1024);
    void add(std::shared_ptr<Entry> const& directive);
    void add(std::shared_ptr<Label> const& label);
    void add(std::shared_ptr<Segment> const& segment);
    [[nodiscard]] bool has_label(std::string const&) const;
    [[nodiscard]] std::shared_ptr<Label> const& label(std::string const&) const;
    [[nodiscard]] uint16_t current_address() const { return m_current->current_address(); }
    [[nodiscard]] std::vector<std::string> const& errors() const;
    std::vector<uint8_t> const& assemble();

    void list(bool list_addresses = false);
    void set_address(uint16_t);

    void append(std::string const& data);
    void append(char const* data);
    void append(std::string_view const& data);
    void append(uint8_t data);
    void append(int data);
    void append(std::vector<uint8_t> const &data);

    [[nodiscard]] std::optional<uint16_t> label_value(std::string const&) const;
    void align(uint16_t);
    ErrorOr<uint16_t> write(std::string const&);
    void dump();

private:
    [[maybe_unused]] uint16_t m_size;
    std::vector<std::shared_ptr<Segment>> m_segments;
    std::shared_ptr<Segment> m_current;
    std::unordered_map<std::string, std::shared_ptr<Label>> m_labels {};
    std::optional<size_t> m_start_address {};
    size_t m_address {0};
    std::vector<std::string> m_errors {};
    std::vector<uint8_t> m_image {};
};

}
