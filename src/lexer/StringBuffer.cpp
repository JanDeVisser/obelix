//
// Created by Jan de Visser on 2021-10-06.
//

#include <lexer/StringBuffer.h>

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

void StringBuffer::rewind()
{
    m_pos = 0;
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

int StringBuffer::peek()
{
    return (m_pos < m_buffer.length()) ? m_buffer[m_pos] : 0;
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