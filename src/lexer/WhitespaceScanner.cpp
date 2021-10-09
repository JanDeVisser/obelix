//
// Created by Jan de Visser on 2021-10-05.
//

#include <lexer/Tokenizer.h>

namespace Obelix {

WhitespaceScanner::WhitespaceScanner(Tokenizer& tokenizer)
    : Scanner(tokenizer, 20)
{
}

WhitespaceScanner::WhitespaceScanner(Tokenizer& tokenizer, WhitespaceScanner::Config const& config)
    : Scanner(tokenizer, 20)
    , m_config(config)
{
}

WhitespaceScanner::WhitespaceScanner(Tokenizer& tokenizer, bool ignore_all_ws)
    : Scanner(tokenizer, 20)
{
    if (ignore_all_ws) {
        m_config.newlines_are_spaces = true;
        m_config.ignore_spaces = true;
    }
}

void WhitespaceScanner::match()
{
    for (m_state = WhitespaceState::Init; m_state != WhitespaceState::Done; ) {
        auto ch = tokenizer().get_char();
        if (ch == '\n') {
            // TODO update line number in tokenizer here

            if (!m_config.newlines_are_spaces) {
                if (m_state == WhitespaceState::Whitespace) {
                    if (m_config.ignore_spaces) {
                        tokenizer().skip();
                    } else {
                        tokenizer().accept(TokenCode::Whitespace);
                    }
                }

                tokenizer().push_as('\n');
                if (m_config.ignore_newlines) {
                    tokenizer().skip();
                } else {
                    tokenizer().accept(TokenCode::NewLine);
                }
                m_state = WhitespaceState::Done;
                continue;
            }
        }

        switch (m_state) {
        case WhitespaceState::Init:
            if (isspace(ch)) {
                if (ch == '\r') {
                    m_state = (m_config.newlines_are_spaces) ? WhitespaceState::Whitespace : WhitespaceState::CR;
                } else {
                    m_state = WhitespaceState::Whitespace;
                }
                tokenizer().push();
            } else {
                m_state = WhitespaceState::Done;
            }
            break;

        case WhitespaceState::CR:
            if (ch == '\r') {
                // TODO update line number in tokenizer here
                if (m_config.newlines_are_spaces) {
                    tokenizer().push();
                    break;
                }
            }
            if (m_config.ignore_newlines) {
                tokenizer().skip();
            } else {
                tokenizer().accept(TokenCode::NewLine);
            }
            m_state = WhitespaceState::Done;
            break;

        case WhitespaceState::Whitespace:
            if (isspace(ch)) {
                tokenizer().push();
            } else {
                if (m_config.ignore_spaces) {
                    tokenizer().skip();
                } else {
                    tokenizer().accept(TokenCode::Whitespace);
                }
                m_state = WhitespaceState::Done;
            }
            break;

        default:
            break;
        }
    }
}

}
