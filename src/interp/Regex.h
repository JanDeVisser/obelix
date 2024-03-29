/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-10-04.
//

#pragma once

#include <core/Object.h>
#include <regex>

namespace Obelix {

class Regex : public Object {
public:
     explicit Regex(std::string pattern, std::string flags = "")
        : Object(TypeRegex)
        , m_pattern("(" + move(pattern) + ")")
        , m_flags(move(flags))
     {
         if (m_flags.find_first_of('i') != std::string::npos) {
             m_re_flags |= std::regex_constants::icase;
         }
     }

     Ptr<Object> match(std::string const& text);
     Ptr<Object> replace(std::string const& text, std::vector<std::string> replacements);
     [[nodiscard]] std::string to_string() const override;
     [[nodiscard]] int compare(Obj const& other) const override;


private:
    Ptr<Object> compile();

    std::string m_pattern;
    std::string m_flags;
    std::regex m_compiled;
    std::regex_constants::syntax_option_type m_re_flags { std::regex_constants::ECMAScript };
    bool m_is_compiled { false };
};

}
