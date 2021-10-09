//
// Created by Jan de Visser on 2021-10-05.
//

#include <lexer/Tokenizer.h>

namespace Obelix {

IdentifierScanner::IdentifierScanner(Tokenizer& tokenizer)
    : Scanner(tokenizer)
{
}

IdentifierScanner::IdentifierScanner(Tokenizer& tokenizer, Config const& config)
    : Scanner(tokenizer)
{
}

bool IdentifierScanner::filter_character(int ch) {
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

    if (ret && tokenizer().token().empty()) {
        ret = filter_against(m_config.starts_with, m_config.startswith_alpha, m_config.startswith_digits);
    }
    return ret;
}

void IdentifierScanner::match()
{
    int ch;
    bool identifier_found { false };

    for (ch = tokenizer().get_char(); filter_character(ch); ch = tokenizer().get_char()) {
        identifier_found = true;
        switch (m_config.alpha) {
        case IdentifierCharacterClass::CaseSensitive:
        case IdentifierCharacterClass::OnlyLower:
        case IdentifierCharacterClass::OnlyUpper:
            tokenizer().push();
            break;
        case IdentifierCharacterClass::FoldToUpper:
            tokenizer().push_as(toupper(ch));
            break;
        case IdentifierCharacterClass::FoldToLower:
            tokenizer().push_as(tolower(ch));
            break;
        default:
            break;
        }
    }
    if (identifier_found)
        tokenizer().accept(m_config.code);
}

}