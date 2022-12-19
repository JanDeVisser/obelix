/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-10-05.
//

#include <lexer/Tokenizer.h>

namespace Obelix {

QStringScanner::QStringScanner(std::string quotes)
    : Scanner()
    , m_quotes(std::move(quotes))
{
}

void QStringScanner::match(Tokenizer& tokenizer)
{
    int ch;

    for (m_state = QStrState::Init; m_state != QStrState::Done; ) {
        ch = tokenizer.get_char();
        if (!ch) {
            break;
        }

        switch (m_state) {
        case QStrState::Init:
            if (m_quotes.find_first_of((char) ch) != std::string::npos) {
                tokenizer.discard();
                m_quote = (char) ch;
                m_state = QStrState::QString;
            } else {
                m_state = QStrState::Done;
            }
            break;

        case QStrState::QString:
            if (ch == m_quote) {
                tokenizer.discard();
                tokenizer.accept(TokenCode_by_char(m_quote));
                m_state = QStrState::Done;
            } else if (ch == '\\') {
                tokenizer.discard();
                m_state = QStrState::Escape;
            } else {
                tokenizer.push();
            }
            break;

        case QStrState::Escape:
            if (ch == 'r') {
                tokenizer.push_as('\r');
            } else if (ch == 'n') {
                tokenizer.push_as('\n');
            } else if (ch == 't') {
                tokenizer.push_as('\t');
            } else {
                tokenizer.push();
            }
            m_state = QStrState::QString;
            break;

        default:
            fatal("Unreachable");
        }
    }
    if (!ch && ((m_state == QStrState::QString) || (m_state == QStrState::Escape))) {
        tokenizer.accept_token(TokenCode::Error, "Unterminated string");
    }
}

}
