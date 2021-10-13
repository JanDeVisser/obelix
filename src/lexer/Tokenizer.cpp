//
// Created by Jan de Visser on 2021-09-20.
//

#include <lexer/Tokenizer.h>

namespace Obelix {

logging_category(lexer);

class CatchAll : public Scanner {
public:
    explicit CatchAll(Tokenizer& tokenizer)
        : Scanner(tokenizer, 0)
    {
    }

    void match_2nd_pass() override
    {
        if (tokenizer().state() != TokenizerState::Success) {
            debug(lexer, "Catchall scanner 2nd pass");
            auto ch = tokenizer().get_char();
            if (ch > 0) {
                tokenizer().push();
                tokenizer().accept(TokenCode_by_char(ch));
            }
        }
    }

    [[nodiscard]] char const* name() override { return "catchall"; }

private:
};

Tokenizer::Tokenizer(std::string_view const& text)
    : m_buffer(text)
{
}

Tokenizer::Tokenizer(StringBuffer text)
    : m_buffer(std::move(text))
{
}

void Tokenizer::assign(StringBuffer buffer)
{
    m_buffer.assign(std::move(buffer));
}

void Tokenizer::assign(char const* text)
{
    m_buffer.assign(text);
}

void Tokenizer::assign(std::string text)
{
    m_buffer.assign(move(text));
}

std::vector<Token> const& Tokenizer::tokenize(std::optional<std::string_view const> text)
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

void Tokenizer::match_token()
{
    debug(lexer, "tokenizer::match_token");
    m_state = TokenizerState::Init;
    m_scanned = 0;

    for (auto& scanner : m_scanners) {
        debug(lexer, "First pass with scanner '{}'", scanner->name());
        rewind();
        scanner->match();
        if (m_state == TokenizerState::Success) {
            debug(lexer, "First pass with scanner {} succeeded", scanner->name());
            break;
        }
    }
    if (m_state != TokenizerState::Success) {
        for (auto& scanner : m_scanners) {
            debug(lexer, "Second pass with scanner '{}'", scanner->name());
            rewind();
            scanner->match_2nd_pass();
            if (m_state == TokenizerState::Success) {
                debug(lexer, "Second pass with scanner {} succeeded", scanner->name());
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
}

Token Tokenizer::accept(TokenCode code)
{
    return accept_token(Token(code, m_token));
}

Token Tokenizer::accept_token(Token const& token)
{
    skip();
    debug(lexer, "Lexer::accept_token({})", token.to_string().c_str());
    m_state = TokenizerState::Success;
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
    m_consumed++;
    if (ch) {
        m_token += (char) ch;
    }
    m_current = 0;
}

void Tokenizer::discard() {
    push_as(0);
}

int Tokenizer::get_char() {
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
    debug(lexer, "m_current '{c}' m_scanned {}", m_current, m_scanned);
    return m_current;
}

}