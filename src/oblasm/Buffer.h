/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <core/StringUtil.h>
#include <oblasm/Directive.h>

namespace Obelix::Assembler {

class Buffer : public Entry {
public:
    Buffer(Mnemonic, std::string const&);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] uint16_t size() const override;
    void append_to(Image& image) override;

private:
    uint16_t m_size { 0 };
};

}
