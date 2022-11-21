/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <lexer/Tokenizer.h>

namespace Obelix {

void CommentScanner::find_eol(Tokenizer& tokenizer)
{
    for (auto ch = tokenizer.get_char(); m_state == CommentState::Text;) {
        if (!ch || (ch == '\r') || (ch == '\n')) {
            m_state = CommentState::None;
            tokenizer.accept(TokenCode::Comment);
        } else {
            tokenizer.discard();
            ch = tokenizer.get_char();
        }
    }
}

void CommentScanner::find_end_marker(Tokenizer& tokenizer)
{
    Token ret {};
    assert(!m_match->end.empty());
    debug(lexer, "find_end_marker: {}", m_match->end);
    int ch;
    for (ch = tokenizer.get_char(); ch && (m_state != CommentState::None);) {
        switch (m_state) {
        case CommentState::Text:
            if (ch == m_match->end[0]) {
                m_token = "";
                m_token += (char)ch;
                m_state = CommentState::EndMarker;
                tokenizer.discard();
            } else {
                tokenizer.push();
            }
            ch = tokenizer.get_char();
            break;
        case CommentState::EndMarker:
            /*
             * Append current character to our token:
             */
            m_token += (char)ch;
            if ((m_token.length() == m_match->end.length()) && (m_token.back() == m_match->end.back())) {
                /*
                 * We matched the full end marker. Set the state of the scanner.
                 */
                tokenizer.discard();
                m_state = CommentState::None;
                tokenizer.accept(TokenCode::Comment);
            } else if (m_token.back() != m_match->end[m_token.length() - 1]) {
                /*
                 * The match of the end marker was lost. Reset the state. It's possible
                 * though that this is the start of a new end marker match though.
                 */
                m_state = CommentState::Text;
            } else {
                /*
                 * Still matching the end marker. Read next character:
                 */
                ch = tokenizer.get_char();
            }
            break;
        default:
            fatal("Unreachable");
        }
    }
    if (!ch) {
        tokenizer.accept_token(TokenCode::Error, "Unterminated comment");
    }
}

void CommentScanner::match(Tokenizer& tokenizer)
{
    m_state = CommentState::None;
    m_token = "";
    auto at_top = tokenizer.at_top();
    for (auto& marker : m_markers) {
        marker.matched = !marker.hashpling || at_top;
    }
    int ch;
    for (m_state = CommentState::StartMarker, ch = tokenizer.get_char();
         ch && m_state != CommentState::None;) {

        /*
         * Append current character to our token:
         */
        m_token += (char)ch;
        m_num_matches = 0;
        for (auto& marker : m_markers) {
            if (marker.matched) {
                marker.matched = (marker.start.substr(0, m_token.length()) == m_token);
                if (marker.matched) {
                    m_num_matches++;
                    m_match = &marker;
                }
            }
        }

        if ((m_num_matches == 1) && (m_token == m_match->start)) {
            tokenizer.discard();
            debug(lexer, "Full match of comment start marker '{}'", m_match->start);
            m_state = CommentState::Text;
            if (!m_match->eol) {
                find_end_marker(tokenizer);
            } else {
                find_eol(tokenizer);
            }
        } else if (m_num_matches > 0) {
            tokenizer.discard();
            debug(lexer, "Matching '{}' comment start markers", m_num_matches);
            m_match = nullptr;
            ch = tokenizer.get_char();
        } else {
            m_match = nullptr;
            m_state = CommentState::None;
        }
    }
}

}
