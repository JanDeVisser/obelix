//
// Created by Jan de Visser on 2021-10-07.
//

#pragma once

#include <lexer/Tokenizer.h>

namespace Obelix {

class Lexer {
public:
    explicit Lexer(std::string text)
        : m_tokenizer(move(text))
    {
    }

    explicit Lexer(char const* text = nullptr)
        : m_tokenizer((text) ? text : "")
    {
    }

    explicit Lexer(StringBuffer text)
        : m_tokenizer(std::move(text))
    {
    }

    template<typename... Args>
    void filter_codes(Args&&... args)
    {
        m_tokenizer.filter_codes(std::forward<Args>(args)...);
    }

    void assign(char const* text)
    {
        m_tokenizer.assign(text);
    }

    [[nodiscard]] StringBuffer const& buffer() const { return m_tokenizer.buffer(); }

    std::vector<Token> const& tokenize(char const* text = nullptr)
    {
        if (text)
            m_tokens = m_tokenizer.tokenize(text);
        else
            m_tokens = m_tokenizer.tokenize();
        return m_tokens;
    }

    Token const& peek(size_t how_many = 0)
    {
        if (m_tokens.empty())
            tokenize();
        oassert(m_current + how_many < m_tokens.size(), "Token buffer underflow");
        return m_tokens[m_current + how_many];
    }

    Token const& lex()
    {
        auto const& ret = peek(0);
        if (m_current < (m_tokens.size() - 1))
            m_current++;
        return ret;
    }

    std::optional<Token const> match(TokenCode code)
    {
        if (peek().code() != code)
            return {};
        return lex();
    }

    TokenCode current_code()
    {
        return peek().code();
    }

    bool expect(TokenCode code)
    {
        auto token_maybe = match(code);
        return token_maybe.has_value();
    }

    template<class ScannerClass, class... Args>
    std::shared_ptr<ScannerClass> add_scanner(Args&&... args)
    {
        auto ret = m_tokenizer.add_scanner<ScannerClass>(std::forward<Args>(args)...);
        return ret;
    }

private:
    Tokenizer m_tokenizer;
    std::vector<Token> m_tokens {};
    size_t m_current { 0 };
};

}
