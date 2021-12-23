/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/StringUtil.h>
#include <oblasm/Bytes.h>
#include <oblasm/Image.h>

namespace Obelix::Assembler {

Bytes::Bytes(Mnemonic m, const std::string& args)
    : Entry(m, args)
{
    switch (m) {
    case Mnemonic::DB:
        m_width = Width::Byte;
        break;
    case Mnemonic::DW:
        m_width = Width::Word;
        break;
    case Mnemonic::DDW:
        m_width = Width::DWord;
        break;
    case Mnemonic::DLW:
        m_width = Width::LWord;
        break;
    default:
        assert(false);
        break;
    }
    append(args);
}

template<typename T>
T shr(T t)
{
    return t >> 8;
}

template<>
uint8_t shr(uint8_t t)
{
    return 0;
}

void Bytes::push_back(auto data_value)
{
    auto num_bytes_to_push = std::min((int)m_width, (int)sizeof(data_value));
    auto ix = 0;
    while (ix++ < num_bytes_to_push) {
        m_bytes.push_back((uint8_t)(data_value & 0xFF));
        data_value = shr(data_value);
    }
    while (ix++ < (int)m_width) {
        m_bytes.push_back(0);
    }
};

void Bytes::append(uint8_t byte)
{
    push_back(byte);
}

void Bytes::append(uint16_t word)
{
    push_back(word);
}

void Bytes::append(uint32_t dword)
{
    push_back(dword);
}

void Bytes::append(uint64_t lword)
{
    push_back(lword);
}

void Bytes::append(const std::string& data)
{
    auto parts = split(data, ' ');
    for (auto& part : parts) {
        auto ulong_maybe = to_ulong(part);
        if (!ulong_maybe.has_value()) {
            add_error(format("Could not parse {} value '{}'", Mnemonic_name(mnemonic()), part));
            return;
        }
        append(ulong_maybe.value());
    }
}

std::string Bytes::to_string() const
{
    return format("[{} bytes]", size());
}

void Bytes::append_to(Image& image)
{
    image.append(m_bytes);
}

}
