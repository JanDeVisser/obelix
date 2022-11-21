/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-10-05.
//

#include <lexer/Tokenizer.h>

namespace Obelix {

IdentifierScanner::IdentifierScanner()
    : Scanner(15)
{
}

IdentifierScanner::IdentifierScanner(Config config)
    : Scanner()
    , m_config(std::move(config))
{
}

bool IdentifierScanner::filter_character(Tokenizer& tokenizer, int ch) const {
    bool ret;

    if (!ch) {
        return false;
    }

    auto filter_against = [ch](auto filter_against, auto alpha_class, auto digits_allowed) {
        if (isalpha(ch)) {
            switch (alpha_class) {
            case IdentifierCharacterClass::NoAlpha:
                return false;
            case IdentifierCharacterClass::OnlyLower:
                return (bool) islower(ch);
            case IdentifierCharacterClass::OnlyUpper:
                return (bool) isupper(ch);
            default:
                return true;
            }
        } else if (isdigit(ch)) {
            return digits_allowed;
        } else if (!filter_against.empty()) {
            return filter_against.find_first_of(ch) != std::string::npos;
        }
        return true;
    };
    ret = filter_against(m_config.filter, m_config.alpha, m_config.digits);

    if (ret && tokenizer.token().empty()) {
        ret = filter_against(m_config.starts_with, m_config.startswith_alpha, m_config.startswith_digits);
    }
    return ret;
}

void IdentifierScanner::match(Tokenizer& tokenizer)
{
    int ch;
    bool identifier_found { false };

    for (ch = tokenizer.get_char(); filter_character(tokenizer, ch); ch = tokenizer.get_char()) {
        identifier_found = true;
        switch (m_config.alpha) {
        case IdentifierCharacterClass::CaseSensitive:
        case IdentifierCharacterClass::OnlyLower:
        case IdentifierCharacterClass::OnlyUpper:
            tokenizer.push();
            break;
        case IdentifierCharacterClass::FoldToUpper:
            tokenizer.push_as(toupper(ch));
            break;
        case IdentifierCharacterClass::FoldToLower:
            tokenizer.push_as(tolower(ch));
            break;
        default:
            break;
        }
    }
    if (identifier_found)
        tokenizer.accept(m_config.code);
}

}
