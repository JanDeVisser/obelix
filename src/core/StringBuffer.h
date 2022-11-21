/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

namespace Obelix {

class StringBuffer {
public:
    StringBuffer() = default;
    explicit StringBuffer(std::string);
    explicit StringBuffer(std::string_view const&);
    explicit StringBuffer(char const*);
    [[nodiscard]] std::string const& str() const { return m_buffer; }
    [[nodiscard]] operator std::string_view() const { return std::string_view(m_buffer); }
    void rewind();
    void partial_rewind(size_t);
    std::string read(size_t);
    [[nodiscard]] int peek(size_t = 0) const;
    int one_of(std::string const&);
    bool expect(char, size_t = 0);
    bool expect(std::string const&, size_t = 0);
    [[nodiscard]] bool is_one_of(std::string const&, size_t = 0) const;
    bool expect_one_of(std::string const&, size_t = 0);
    int readchar();
    void skip(size_t = 1);
    void pushback(size_t = 1);
    void reset();
    StringBuffer& assign(char const*);
    StringBuffer& assign(std::string);
    StringBuffer& assign(StringBuffer);

private:
    std::string m_buffer;
    size_t m_pos { 0 };
};

}
