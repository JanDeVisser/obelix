/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <lexer/Tokenizer.h>

namespace Obelix {

extern_logging_category(lexer);

class CatchAll : public Scanner {
public:
    explicit CatchAll()
        : Scanner(99)
    {
    }

    void match(Tokenizer& tokenizer) override
    {
    }

    [[nodiscard]] char const* name() const override { return "catchall"; }

private:
};

Tokenizer::Tokenizer(std::string_view const& text, std::string file_name)
    : m_buffer(text)
    , m_location { std::move(file_name), 1, 1, 1, 1 }
{
}

Tokenizer::Tokenizer(StringBuffer text, std::string file_name)
    : m_buffer(std::move(text))
    , m_location { std::move(file_name), 1, 1, 1, 1 }
{
}

std::vector<Token> const& Tokenizer::tokenize(std::optional<std::string_view const> text)
{
    if (text.has_value()) {
        m_buffer = StringBuffer(text.value());
        m_tokens.clear();
    }
    if (m_tokens.empty()) {
        debug(lexer, "Scanners:");
        for (auto &scanner : m_scanners) {
            debug(lexer, "{} priority {}", scanner->name(), scanner->priority());
        }
        while (!m_eof) {
            match_token();
        }
        oassert(!m_tokens.empty(), "tokenize() found no tokens, not even EOF");
        oassert(m_tokens.back().code() == TokenCode::EndOfFile, "tokenize() did not leave an EOF");
    }
    return m_tokens;
}

void Tokenizer::match_token()
{
    debug(lexer, "tokenizer::match_token");
    m_state = TokenizerState::Init;
    m_scanned = 0;

    for (auto& scanner : m_scanners) {
        debug(lexer, "Matching with scanner '{}'", scanner->name());
        rewind();
        scanner->match(*this);
        if (m_state == TokenizerState::Success) {
            debug(lexer, "Match with scanner {} succeeded", scanner->name());
            break;
        }
    }

    if (state() != TokenizerState::Success) {
        rewind();
        debug(lexer, "Catchall scanner");
        auto ch = get_char();
        if (ch > 0) {
            push();
            accept(TokenCode_by_char(ch));
        }
    }

    reset();
    if (m_eof) {
        debug(lexer, "End-of-file. Accepting TokenCode::EndOfFile");
        accept_token(TokenCode::EndOfFile, "End of File Marker");
    }
}

std::string const& Tokenizer::token() const
{
    return m_token;
}

void Tokenizer::chop(size_t num)
{
    m_token.erase(0, num);
}

TokenizerState Tokenizer::state() const
{
    return m_state;
}

bool Tokenizer::at_top() const
{
    return m_total_count == 0;
}

bool Tokenizer::at_end() const
{
    return m_eof;
}

/**
 * Rewind the tokenizer to the point just after the last token was identified.
 */
void Tokenizer::rewind()
{
    debug(lexer, "Rewinding tokenizer");
    if (m_scanned > 0)
        m_eof = false;
    m_token = "";
    m_buffer.rewind();
    m_scanned = 0;
    m_consumed = 0;
    m_location.end_line = m_location.start_line;
    m_location.end_column = m_location.start_column;
}

void Tokenizer::partial_rewind(size_t num)
{
    if (num > m_consumed)
        num = m_scanned;
    m_scanned -= num;
    m_consumed -= num;
    m_token = m_token.substr(0, m_token.length() - num);
    m_buffer.partial_rewind(num);

    // FIXME doesn't work if we're rewinding over a linebreak
    m_location.end_column -= num;
}

/**
 * Mark the current point, discarding everything that came before it.
 */
void Tokenizer::reset() {
    debug(lexer, "Resetting tokenizer");
    m_buffer.rewind();
    m_buffer.skip(m_consumed);
    m_buffer.reset();
    if (m_scanned > 0)
        m_eof = false;
    m_current = 0;
    m_token = "";
    m_total_count += m_consumed;
    m_scanned = 0;
    m_consumed = 0;
    m_prev_was_cr = false;
    m_location.start_line = m_location.end_line;
    m_location.start_column = m_location.end_column;
}

Token Tokenizer::accept(TokenCode code)
{
    return accept_token(code, m_token);
}

Token Tokenizer::accept_token(TokenCode code, std::string value)
{
    Token ret = Token(code, value);
    return accept_token(ret);
}

Token Tokenizer::accept_token(Token& token)
{
    token.location(m_location);
    skip();
    debug(lexer, "Lexer::accept_token({})", token.to_string());
    m_state = TokenizerState::Success;
    if (!m_filtered_codes.contains(token.code()))
        m_tokens.push_back(token);
    return token;
}

void Tokenizer::skip()
{
    reset();
    m_state = TokenizerState::Success;
}

/**
 * Rewinds the buffer and reads <code>num</code> characters from the buffer,
 * returning the read string as a token with code <code>code</code>.
 */
Token Tokenizer::get_accept(TokenCode code, int num) {
    rewind();
    for (auto i = 0; i < num; i++) {
        get_char();
        push();
    }
    return accept(code);
}

void Tokenizer::push() {
    push_as(m_current);
}

void Tokenizer::push_as(int ch) {
    m_location.end_column++;
    m_consumed++;
    if (ch) {
        m_token += (char) ch;
    }
    m_current = 0;
}

void Tokenizer::discard() {
    push_as(0);
}

int Tokenizer::get_char()
{
    if (m_eof)
        return 0;
    m_current = m_buffer.readchar();
    if (m_current <= 0) {
        debug(lexer, "EOF reached");
        m_eof = true;
        m_current = 0;
        return 0;
    }
    if ((m_current == '\n') || m_prev_was_cr) {
        m_location.end_line++;
        m_location.end_column = 1;
    }
    m_prev_was_cr = (m_current == '\r');
    m_scanned++;
    debug(lexer, "m_current '{c}' m_scanned {}", m_current, m_scanned);
    return m_current;
}

}
