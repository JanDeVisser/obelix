//
// Created by Jan de Visser on 2021-10-05.
//

#include <lexer/Lexer.h>

namespace Obelix {

QStringScanner::QStringScanner(Lexer& lexer, std::string quotes)
    : Scanner(lexer)
    , m_quotes(move(quotes))
{
}

void QStringScanner::match()
{
    int ch;

    for (m_state = QStrState::Init; m_state != QStrState::Done; ) {
        ch = lexer().get_char();
        if (!ch) {
            break;
        }

        switch (m_state) {
        case QStrState::Init:
            if (m_quotes.find_first_of(ch) != std::string::npos) {
                lexer().discard();
                m_quote = ch;
                m_state = QStrState::QString;
            } else {
                m_state = QStrState::Done;
            }
            break;

        case QStrState::QString:
            if (ch == m_quote) {
                lexer().discard();
                lexer().accept(TokenCode_by_char(m_quote));
                m_state = QStrState::Done;
            } else if (ch == '\\') {
                lexer().discard();
                m_state = QStrState::Escape;
            } else {
                lexer().push();
            }
            break;

        case QStrState::Escape:
            if (ch == 'r') {
                lexer().push_as('\r');
            } else if (ch == 'n') {
                lexer().push_as('\n');
            } else if (ch == 't') {
                lexer().push_as('\t');
            } else {
                lexer().push();
            }
            m_state = QStrState::QString;
            break;

        default:
            fatal("Unreachable");
        }
    }
    if (!ch && ((m_state == QStrState::QString) || (m_state == QStrState::Escape))) {
        lexer().accept_token(Token(TokenCode::Error, "Unterminated string"));
    }
}

}
