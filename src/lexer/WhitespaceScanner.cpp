//
// Created by Jan de Visser on 2021-10-05.
//

#include <lexer/Lexer.h>

namespace Obelix {

WhitespaceScanner::WhitespaceScanner(Lexer& lexer)
    : Scanner(lexer, 20)
{
}

WhitespaceScanner::WhitespaceScanner(Lexer& lexer, WhitespaceScanner::Config const& config)
    : Scanner(lexer, 20)
    , m_config(config)
{
}

WhitespaceScanner::WhitespaceScanner(Lexer& lexer, bool ignore_all_ws)
    : Scanner(lexer, 20)
{
    if (ignore_all_ws) {
        m_config.newlines_are_spaces = true;
        m_config.ignore_spaces = true;
    }
}

void WhitespaceScanner::match()
{
    for (m_state = WhitespaceState::Init; m_state != WhitespaceState::Done; ) {
        auto ch = lexer().get_char();
        if (ch == '\n') {
            // TODO update line number in lexer here

            if (!m_config.newlines_are_spaces) {
                if (m_state == WhitespaceState::Whitespace) {
                    if (m_config.ignore_spaces) {
                        lexer().skip();
                    } else {
                        lexer().accept(TokenCode::Whitespace);
                    }
                }

                lexer().push_as('\n');
                if (m_config.ignore_newlines) {
                    lexer().skip();
                } else {
                    lexer().accept(TokenCode::NewLine);
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
                lexer().push();
            } else {
                m_state = WhitespaceState::Done;
            }
            break;

        case WhitespaceState::CR:
            if (ch == '\r') {
                // TODO update line number in lexer here
                if (m_config.newlines_are_spaces) {
                    lexer().push();
                    break;
                }
            }
            if (m_config.ignore_newlines) {
                lexer().skip();
            } else {
                lexer().accept(TokenCode::NewLine);
            }
            m_state = WhitespaceState::Done;
            break;

        case WhitespaceState::Whitespace:
            if (isspace(ch)) {
                lexer().push();
            } else {
                if (m_config.ignore_spaces) {
                    lexer().skip();
                } else {
                    lexer().accept(TokenCode::Whitespace);
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
