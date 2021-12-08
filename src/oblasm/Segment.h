/*
 * Segment.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <memory>

#include <oblasm/Directive.h>

namespace Obelix::Assembler {

class Segment : public Entry {
public:
    explicit Segment(uint16_t start_address = 0);
    explicit Segment(std::string const& start_address);
    ~Segment() override = default;

    [[nodiscard]] uint16_t size() const override { return m_size; }
    [[nodiscard]] uint16_t start_address() const { return m_start_address; }
    [[nodiscard]] std::vector<std::shared_ptr<Entry>> const& entries() const { return m_entries; }
    [[nodiscard]] uint16_t current_address() const { return start_address() + size(); }
    [[nodiscard]] std::string to_string() const override;

    void add(std::shared_ptr<Entry> const&);
    void append_to(Image&) override;

private:
    uint16_t m_size { 0 };
    uint16_t m_start_address { 0 };
    std::vector<std::shared_ptr<Entry>> m_entries {};
};

}
