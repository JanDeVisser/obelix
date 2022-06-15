/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
    explicit Image(uint16_t size = 16 * 1024);
    void add(std::shared_ptr<Entry> const& directive);
    void add(std::shared_ptr<Label> const& label);
    void add(Segment&, std::shared_ptr<Label> const&);
    void add(std::shared_ptr<Segment> const&);
    [[nodiscard]] uint16_t size() const { return m_size; }
    [[nodiscard]] std::shared_ptr<Segment> get_segment(int ix) { return m_segments.at(ix); }
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
    uint16_t m_size;
    std::vector<std::shared_ptr<Segment>> m_segments;
    std::shared_ptr<Segment> m_current;
    std::unordered_map<std::string, std::shared_ptr<Label>> m_labels {};
    std::optional<size_t> m_start_address {};
    size_t m_address {0};
    std::vector<std::string> m_errors {};
    std::vector<uint8_t> m_image {};
};

}
