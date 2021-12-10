/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <oblasm/Directive.h>

namespace Obelix::Assembler {

class String : public Entry {
public:
    enum class Type {
        ASCII,
        ASCIZ
    };

    constexpr static char const* Type_name(Type type)
    {
        switch (type) {
        case Type::ASCII:
            return "ascii";
        case Type::ASCIZ:
            return "asciz";
        }
    }

    explicit String(Mnemonic mnemonic, std::string const& str);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] uint16_t size() const override;
    void append_to(Image& image) override;

private:
    Type m_type { Type::ASCII };
    std::string m_str;
};

}
