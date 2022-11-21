/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-10-06.
//

#include <core/StringBuffer.h>

namespace Obelix {

StringBuffer::StringBuffer(std::string str)
    : m_buffer(move(str))
{
}

StringBuffer::StringBuffer(std::string_view const& str)
    : StringBuffer(std::string(str))
{
}

StringBuffer::StringBuffer(char const* str)
    : StringBuffer(std::string(str ? str : ""))
{
}

StringBuffer& StringBuffer::assign(std::string buffer)
{
    m_buffer = move(buffer);
    rewind();
    return *this;
}

StringBuffer& StringBuffer::assign(const char* buffer)
{
    m_buffer = buffer;
    rewind();
    return *this;
}

StringBuffer& StringBuffer::assign(StringBuffer buffer)
{
    m_buffer = move(buffer.m_buffer);
    rewind();
    return *this;
}

void StringBuffer::rewind()
{
    m_pos = 0;
}

void StringBuffer::partial_rewind(size_t num)
{
    if (num > m_pos)
        num = m_pos;
    m_pos -= num;
}

std::string StringBuffer::read(size_t num)
{
    if ((num < 0) || ((m_pos + num) > m_buffer.length())) {
        num = m_buffer.length() - m_pos;
    }
    if (num > 0) {
        auto ret = m_buffer.substr(m_pos, num);
        m_pos = m_pos + num;
        return ret;
    } else {
        return "";
    }
}

int StringBuffer::peek(size_t num) const
{
    return ((m_pos + num) < m_buffer.length()) ? m_buffer[m_pos + num] : 0;
}

int StringBuffer::readchar()
{
    auto ret = peek();
    m_pos++;
    return ret;
}

void StringBuffer::skip(size_t num)
{
    if (m_pos + num > m_buffer.length()) {
        num = m_buffer.length() - m_pos;
    }
    m_pos += num;
}

bool StringBuffer::expect(char ch, size_t offset)
{
    if (peek(offset) != ch)
        return false;
    m_pos += offset + 1;
    return true;
}

bool StringBuffer::expect(std::string const& str, size_t offset)
{
    if (m_buffer.length() < m_pos + offset)
        return false;
    if (m_buffer.length() < m_pos + offset + str.length())
        return false;
    if (m_buffer.substr(m_pos + offset, str.length()) != str)
        return false;
    m_pos += offset + str.length();
    return true;
}

bool StringBuffer::is_one_of(std::string const& str, size_t offset) const
{
    for (auto ch : str) {
        if (peek(offset) == ch) {
            return true;
        }
    }
    return false;
}

bool StringBuffer::expect_one_of(std::string const& str, size_t offset)
{
    if (is_one_of(str, offset)) {
        m_pos += offset + 1;
        return true;
    }
    return false;
}

int StringBuffer::one_of(std::string const& str)
{
    if (str.find_first_of((char) peek()) != std::string::npos) {
        return readchar();
    }
    return 0;
}

void StringBuffer::pushback(size_t num)
{
    if (num > m_pos) {
        num = m_pos;
    }
    m_pos -= num;
}

void StringBuffer::reset()
{
    if (m_pos < m_buffer.length()) {
        m_buffer.erase(0, m_pos);
    } else {
        m_buffer = "";
    }
    rewind();
}

}
