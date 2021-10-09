//
// Created by Jan de Visser on 2021-10-06.
//

#pragma once

#include <string>

namespace Obelix {

class StringBuffer {
public:
    StringBuffer() = default;
    explicit StringBuffer(std::string);
    explicit StringBuffer(std::string_view const&);
    explicit StringBuffer(char const*);
    void rewind();
    std::string read(size_t);
    int peek();
    int readchar();
    void skip(size_t = 1);
    void pushback(size_t = 1);
    void reset();
    StringBuffer& assign(char const*);
    StringBuffer& assign(std::string);

private:
    std::string m_buffer;
    size_t m_pos { 0 };
};

}
