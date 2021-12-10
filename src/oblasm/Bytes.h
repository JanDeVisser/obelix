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
        Byte,  // 8 bits
        Word,  // 16 bits
        DWord  // 32 bits
    };

    Bytes(Mnemonic, std::string const&);
    void append(uint8_t);
    void append(uint16_t);
    void append(uint32_t);
    void append(std::string const& data);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] uint16_t size() const override { return m_bytes.size(); }
    void append_to(Image&) override;

private:
    Width m_width { Width::Byte };
    std::vector<uint8_t> m_bytes {};
};

}
