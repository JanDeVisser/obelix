//
// Created by Jan de Visser on 2021-09-18.
//

#include "Token.h"

std::string TokenType_name(TokenType t)
{
    switch (t) {
#undef __ENUMERATE_TOKEN_TYPE
#define __ENUMERATE_TOKEN_TYPE(type, c, str) \
    case TokenType::type:          \
        return #type;
        ENUMERATE_TOKEN_TYPES(__ENUMERATE_TOKEN_TYPE)
#undef __ENUMERATE_TOKEN_TYPE
    default:
        assert(false);
    }
}

std::string Token::to_string() const
{
    std::string ret = TokenType_name(type());
    if (!m_value.empty()) {
        ret += " [" + value() + "]";
    }
    return ret;
}

std::optional<int> Token::to_number() const
{
    char* endptr;
    auto ret = std::strtol(m_value.c_str(), &endptr, 10);
    if (endptr == m_value.c_str()) {
        return ret;
    }
    return {};
}

std::shared_ptr<Object> Token::to_object() const
{
    switch (type()) {
    case TokenType::Number:
        {
            auto maybe_number = to_number();
            if (maybe_number.has_value())
                return std::make_shared<Number>(maybe_number.value());
        }
        break;
    case TokenType::DoubleQuotedString:
    case TokenType::SingleQuotedString:
    case TokenType::BackQuotedString:
        return std::make_shared<String>(m_value);
    case TokenType::Identifier:
        if (m_value == "true")
            return Boolean::True();
        if (m_value == "false")
            return Boolean::False();
        break;
    default:
        break;
    }
    return nullptr;
}
