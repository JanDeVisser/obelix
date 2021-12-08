//
// Created by Jan de Visser on 2021-10-07.
//

#pragma once

#include <lexer/Tokenizer.h>

namespace Obelix {

class Lexer {
public:
    explicit Lexer(char const* text = nullptr)
        : m_buffer((text) ? text : "")
    {
    }

    explicit Lexer(StringBuffer text)
        : m_buffer(std::move(text))
    {
    }

    template<typename... Args>
    void filter_codes(TokenCode code, Args&&... args)
    {
        m_filtered_codes.insert(code);
        filter_codes(std::forward<Args>(args)...);
    }

    void filter_codes()
    {
    }

    void assign(char const* text)
    {
        m_buffer.assign(text);
        m_tokens.clear();
        m_current = 0;
    }

    void assign(std::string const& text)
    {
        m_buffer.assign(text);
        m_tokens.clear();
        m_current = 0;
    }

    [[nodiscard]] StringBuffer const& buffer() const { return m_buffer; }

    std::vector<Token> const& tokenize(char const* text = nullptr)
    {
        if (text)
            assign(text);
        Tokenizer tokenizer(m_buffer);
        tokenizer.add_scanners(m_scanners);
        tokenizer.filter_codes(m_filtered_codes);
        m_tokens = tokenizer.tokenize();
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
        auto ret = std::make_shared<ScannerClass>(std::forward<Args>(args)...);
        m_scanners.insert(std::dynamic_pointer_cast<Scanner>(ret));
        return ret;
    }

    void mark()
    {
        m_bookmarks.push_back(m_current);
    }

    void discard_mark()
    {
        m_bookmarks.pop_back();
    }

    void rewind()
    {
        m_current = m_bookmarks.back();
        discard_mark();
    }

private:
    StringBuffer m_buffer;
    std::vector<Token> m_tokens {};
    size_t m_current { 0 };
    std::vector<size_t> m_bookmarks {};
    std::unordered_set<TokenCode> m_filtered_codes {};
    std::set<std::shared_ptr<Scanner>> m_scanners {};
};

}
