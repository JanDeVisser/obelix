/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <core/FileBuffer.h>
#include <lexer/Lexer.h>

namespace Obelix {

class BasicParser {
public:
    static ErrorOr<std::shared_ptr<BasicParser>,SystemError> create(std::string const& file_name, BufferLocator* locator = nullptr);

    explicit BasicParser(StringBuffer& src);
    explicit BasicParser();
    [[nodiscard]] std::string const& text() const { return m_lexer.buffer().str(); }
    ErrorOr<void,SystemError> read_file(std::string const&, BufferLocator* locator = nullptr);
    [[nodiscard]] std::vector<SyntaxError> const& errors() const { return m_errors; };
    [[nodiscard]] bool has_errors() const { return !m_errors.empty(); }
    Token const& peek();
    TokenCode current_code();

    Token const& lex();
    Token const& replace(Token);
    std::optional<Token const> match(TokenCode, char const* = nullptr);
    std::optional<Token const> skip(TokenCode);
    bool expect(TokenCode, char const* = nullptr);
    bool expect(char const*, char const* = nullptr);
    void add_error(Token const& token, std::string const& message) { add_error(token.location(), message); }
    void add_error(Span const&, std::string const&);
    void clear_errors() { m_errors.clear(); }
    [[nodiscard]] bool was_successful() const { return m_errors.empty(); }
    [[nodiscard]] Lexer& lexer() { return m_lexer; }
    [[nodiscard]] std::string const& file_name() const { return m_file_name; }
    [[nodiscard]] std::string const& file_path() const { return m_file_path; }
    void mark() { m_lexer.mark(); }
    void discard_mark() { m_lexer.discard_mark(); }
    void rewind() { m_lexer.rewind(); }

    template <typename ...Args>
    void add_error(Token const& token, std::string message, Args&&... args)
    {
        add_error(token, format(message, std::forward<Args>(args)...));
    }

protected:
    explicit BasicParser(std::string const& file_name, BufferLocator* locator = nullptr);

private:
    std::string m_file_name { "<literal>" };
    std::string m_file_path;
    Lexer m_lexer;
    std::vector<SyntaxError> m_errors {};
};

}
