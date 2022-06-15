/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <oblasm/Directive.h>

namespace Obelix::Assembler {

class Bytes : public Entry {
public:
    enum class Width {
        Byte = 0x01,  // 8 bits
        Word = 0x02,  // 16 bits
        DWord = 0x04, // 32 bits
        LWord = 0x08, // 64 bits
    };

    explicit Bytes(Mnemonic, std::string const& = "");
    void append(uint8_t);
    void append(uint16_t);
    void append(uint32_t);
    void append(uint64_t);
    void append(unsigned long lword) { append((uint64_t)lword); }
    void append(std::string const& data);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] uint16_t size() const override { return m_bytes.size(); }
    void append_to(Image&) override;

private:
    Width m_width { Width::Byte };
    std::vector<uint8_t> m_bytes {};

    void push_back(auto data_value);
};

}
