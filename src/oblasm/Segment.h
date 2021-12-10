/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
