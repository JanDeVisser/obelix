/*
 * Image.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <cstdio>

#include <core/Format.h>
#include <oblasm/Image.h>

namespace Obelix::Assembler {

std::vector<std::string> bytes_to_strings(std::vector<uint8_t> bytes)
{
    std::vector<std::string> ret;
    int ix = 0;
    while (ix < bytes.size()) {
        auto s = format("{04x}  ", bytes);
        for (auto b = 0; (ix < bytes.size()) && (b < 8); ++b, ++ix) {
            s += format("{02x} ", bytes[ix]);
        }
        ret.push_back(s);
    }
    return ret;
}

Image::Image(uint16_t size)
    : m_size(size)
    , m_segments({ std::make_shared<Segment>(0) })
    , m_current(m_segments[0])
{
}

void Image::add(std::shared_ptr<Entry> const& entry)
{
    m_current->add(entry);
}

void Image::add(std::shared_ptr<Label> const& label)
{
    m_current->add(std::dynamic_pointer_cast<Entry>(label));
    m_labels.insert_or_assign(label->label(), label);
}

bool Image::has_label(const std::string& label) const
{
    return m_labels.contains(label);
}

std::shared_ptr<Label> const& Image::label(std::string const& lbl) const
{
    assert(m_labels.contains(lbl));
    return m_labels.at(lbl);
}

std::optional<uint16_t> Image::label_value(std::string const& label_name) const
{
    if (has_label(label_name)) {
        auto lbl = m_labels.at(label_name);
        return lbl->value();
    }
    return {};
}

void Image::new_segment(uint16_t addr)
{
    if (m_current->size() == 0)
        m_segments.pop_back();
    auto segment = std::make_shared<Segment>(addr);
    m_segments.push_back(segment);
    m_current = segment;
}

void Image::append(std::string const& data)
{
    append(static_cast<std::string_view>(data));
}

void Image::append(char const* data)
{
    append(static_cast<std::string_view>(data));
}

void Image::append(std::string_view const& data)
{
    for (auto ch : data) {
        append(static_cast<uint8_t>(ch));
    }
}

void Image::append(uint8_t data)
{
    m_image.push_back(data);
}

void Image::append(int data)
{
    if ((data >= -128) && (data < 256))
        m_image.push_back(static_cast<uint8_t>(data));
}

void Image::append(std::vector<uint8_t> const &data)
{
    for (auto byte : data) {
        append(byte);
    }
}

std::vector<std::string> const& Image::errors() const
{
    return m_errors;
}

std::vector<uint8_t> const& Image::assemble()
{
    for (auto& segment : m_segments) {
        segment->append_to(*this);
        m_errors.insert(m_errors.end(), segment->errors().cbegin(), segment->errors().cend());
    }
    return m_image;
}

void Image::list(bool list_addresses)
{
    auto list_entry = [list_addresses](uint16_t address, std::shared_ptr<Entry> const& entry) {
        std::string_view prefix = entry->prefix();
        if (prefix.empty()) {
            prefix = (list_addresses) ? format("{04x}\t", address) : "\t";
        }
        printf("%s\n", format("{}{}", prefix, entry->to_string()).c_str());
        for (auto& err : entry->errors()) {
            printf("ERROR: %s\n", err.c_str());
        }
    };

    for (auto& segment : m_segments) {
        uint16_t addr = segment->start_address();
        list_entry(addr, segment);
        for (auto& entry : segment->entries()) {
            list_entry(addr, entry);
            addr += entry->size();
        }
    }
}

void Image::set_address(uint16_t address)
{
    assert(address >= m_address);
    if (!m_start_address.has_value())
        m_start_address = address;
    while (m_address < address)
        append(0);
}

void Image::align(uint16_t boundary)
{
    for (auto remainder = m_address % boundary; remainder; --remainder) {
        append(0);
    }
}

void Image::dump()
{
    printf("\nBinary dump\n");
    auto strings = bytes_to_strings(m_image);
    for (auto& s : strings) {
        printf("%s\n", s.c_str());
    }
}

ErrorOr<uint16_t> Image::write(const std::string& file_name)
{
    FILE *file = fopen(file_name.c_str(), "wb");
    if (!file)
        return Error { ErrorCode::IOError, format("Could not open file {} for writing: {}", file_name, strerror(errno)) };
    uint16_t ret = fwrite(m_image.data(), 1, m_image.size(), file);
    fclose(file);
    if (ret != m_image.size())
        return Error { ErrorCode::IOError, format("Could not write image to file {}", file_name) };
    return ret;
}

}
