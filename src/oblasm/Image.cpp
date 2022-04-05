/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <array>
#include <cstdio>

#include <core/Format.h>
#include <oblasm/Image.h>
#include <oblasm/Instruction.h>

namespace Obelix::Assembler {

std::vector<std::string> bytes_to_strings(std::vector<uint8_t> bytes)
{
    std::vector<std::string> ret;
    int ix = 0;
    std::array<uint8_t, 8> prev_row = {};
    std::array<uint8_t, 8> cur_row = {};
    while (ix < bytes.size()) {
        for (auto b = ix; (b < bytes.size()) && (b < (ix + 8)); ++b) {
            cur_row[b - ix] = bytes[b];
        }
        if ((ix == 0) || (cur_row != prev_row)) {
            auto s = format("{04x}  ", ix);
            for (auto b = ix; (b < bytes.size()) && (b < (ix + 8)); ++b) {
                s += format("{02x} ", bytes[b]);
            }
            ret.push_back(s);
        } else {
            std::string s = "      ...";
            if (ret.back() != s)
                ret.push_back(s);
        }
        ix += 8;
        prev_row = cur_row;
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

void Image::add(Segment& segment, std::shared_ptr<Label> const& label)
{
    segment.add(std::dynamic_pointer_cast<Entry>(label));
    m_labels.insert_or_assign(label->label(), label);
}

void Image::add(std::shared_ptr<Segment> const& segment)
{
    m_segments.push_back(segment);
    m_current = m_segments.back();
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
    if (!errors().empty())
        m_image.clear();
    return m_image;
}

void Image::list(bool list_addresses)
{
    auto list_entry = [list_addresses](uint16_t address, std::shared_ptr<Entry> const& entry) {
        auto& e = *entry;
        if (typeid(e) != typeid(Instruction&)) {
            printf("%s\n", format("{}", entry->to_string()).c_str());
        } else if (list_addresses) {
            printf("%s\n", format("{04x}\t{}", address, entry->to_string()).c_str());
        } else {
            printf("%s\n", format("\t{}", entry->to_string()).c_str());
        }
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
    m_image.resize(address);
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
    for (auto& row : strings) {
        printf("%s\n", row.c_str());
    }
}

ErrorOr<uint16_t> Image::write(const std::string& file_name)
{
    FILE *file = fopen(file_name.c_str(), "wb");
    if (!file)
        return Error<int> { ErrorCode::IOError, errno, format("Could not open file {} for writing: {}", file_name, strerror(errno)) };
    m_image.resize(m_size);
    uint16_t ret = fwrite(m_image.data(), 1, m_image.size(), file);
    fclose(file);
    if (ret != m_image.size())
        return Error<int> { ErrorCode::IOError, errno, format("Could not write image to file {}", file_name) };
    return ret;
}

}
