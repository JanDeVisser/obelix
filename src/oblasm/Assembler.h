/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <string>

#include <lexer/BasicParser.h>
#include <oblasm/Image.h>

namespace Obelix::Assembler {

class Assembler : public BasicParser {
public:
    explicit Assembler(Image&);

    void parse(std::string const& text);

private:
    void parse_label(std::string const&);
    void parse_directive();
    void parse_mnemonic();
    std::optional<Argument> parse_argument();

    Image& m_image;
};

}
