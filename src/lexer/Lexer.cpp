//
// Created by Jan de Visser on 2021-09-20.
//

#include <lexer/Lexer.h>

namespace Obelix {

logging_category(lexer);

class CatchAll : public Scanner {
public:
    explicit CatchAll(Lexer& lexer)
        : Scanner(lexer, 0)
    {
    }

    void match_2nd_pass() override
    {
        if (lexer().state() != LexerState::Success) {
            debug(lexer, "Catchall scanner 2nd pass");
            auto ch = lexer().get_char();
            if (ch > 0) {
                lexer().push();
                lexer().accept(TokenCode_by_char(ch));
            }
        }
    }

    [[nodiscard]] char const* name() override { return "catchall"; }

private:
};

Lexer::Lexer(std::string_view const& text)
    : m_buffer(text)
{
}

std::vector<Token> const& Lexer::tokenize(std::optional<std::string_view const> text)
{
    if (text.has_value()) {
        m_buffer = StringBuffer(text.value());
        m_tokens.clear();
    }
    if (m_tokens.empty()) {
        if (!m_has_catch_all) {
            add_scanner<CatchAll>();
            m_has_catch_all = true;
        }
        while (!m_eof) {
            match_token();
        }
        oassert(!m_tokens.empty(), "tokenize() found no tokens, not even EOF");
        oassert(m_tokens.back().code() == TokenCode::EndOfFile, "tokenize() did not leave an EOF");
    }
    return m_tokens;
}

void Lexer::match_token()
{
    debug(lexer, "lexer::match_token");
    m_state = LexerState::Init;
    m_scanned = 0;

    for (auto& scanner : m_scanners) {
        debug(lexer, "First pass with scanner '%s'", scanner->name());
        rewind();
        scanner->match();
        if (m_state == LexerState::Success) {
            debug(lexer, "First pass with scanner %s succeeded", scanner->name());
            break;
        }
    }
    if (m_state != LexerState::Success) {
        for (auto& scanner : m_scanners) {
            debug(lexer, "Second pass with scanner '%s'", scanner->name());
            rewind();
            scanner->match_2nd_pass();
            if (m_state == LexerState::Success) {
                debug(lexer, "Second pass with scanner %s succeeded", scanner->name());
                break;
            }
        }
    }
    reset();
    if (m_eof) {
        debug(lexer, "End-of-file. Accepting TokenCode::EndOfFile");
        accept_token(Token(TokenCode::EndOfFile, "End of File Marker"));
    }
}

std::string const& Lexer::token() const
{
    return m_token;
}

void Lexer::chop(size_t num)
{
    m_token.erase(0, num);
}

LexerState Lexer::state() const
{
    return m_state;
}

bool Lexer::at_top() const
{
    return m_total_count == 0;
}

bool Lexer::at_end() const
{
    return m_eof;
}

/**
 * Rewind the lexer to the point just after the last token was identified.
 */
void Lexer::rewind()
{
    debug(lexer, "Rewinding lexer");
    if (m_scanned > 0)
        m_eof = false;
    m_token = "";
    m_buffer.rewind();
    m_scanned = 0;
    m_consumed = 0;
}

/**
 * Mark the current point, discarding everything that came before it.
 */
void Lexer::reset() {
    debug(lexer, "Resetting lexer");
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
}

Token Lexer::accept(TokenCode code)
{
    return accept_token(Token(code, m_token));
}

Token Lexer::accept_token(Token const& token)
{
    skip();
    debug(lexer, "Lexer::accept_token(%s)", token.to_string().c_str());
    m_state = LexerState::Success;
    m_tokens.push_back(token);
    return token;
}

void Lexer::skip()
{
    reset();
    m_state = LexerState::Success;
}

/**
 * Rewinds the buffer and reads <code>num</code> characters from the buffer,
 * returning the read string as a token with code <code>code</code>.
 */
Token Lexer::get_accept(TokenCode code, int num) {
    rewind();
    for (auto i = 0; i < num; i++) {
        get_char();
        push();
    }
    return accept(code);
}

void Lexer::push() {
    push_as(m_current);
}

void Lexer::push_as(int ch) {
    m_consumed++;
    if (ch) {
        m_token += (char) ch;
    }
    m_current = 0;
}

void Lexer::discard() {
    push_as(0);
}

int Lexer::get_char() {
    if (m_eof)
        return 0;
    m_current = m_buffer.readchar();
    if (m_current <= 0) {
        debug(lexer, "EOF reached");
        m_eof = true;
        m_current = 0;
        return 0;
    }
    m_scanned++;
    debug(lexer, "m_current %c m_scanned %d", m_current, m_scanned);
    return m_current;
}

}