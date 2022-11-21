/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-10-06.
//

#include <lexer/Tokenizer.h>

namespace Obelix {

void KeywordScanner::add_keyword(TokenCode keyword_code)
{
    add_keyword(Token { keyword_code, TokenCode_name(keyword_code) });
}

void KeywordScanner::add_keyword(Token const& keyword_token)
{
    Token token;
    if (m_case_sensitive) {
        token = keyword_token;
    } else {
        token = Token(keyword_token.code(), to_upper(keyword_token.value()));
    }
    Keyword keyword = { token };
    auto ch = token.value()[token.value().length() - 1];
    keyword.is_operator = (!isalnum(ch) && (ch != '_'));
    m_keywords.push_back(keyword);
    std::sort(m_keywords.begin(), m_keywords.end(), [](Keyword const& a, Keyword const& b) {
        return a.token.value() < b.token.value();
    });
}

void KeywordScanner::match_character(int ch)
{
    if (m_state == KeywordScannerState::Init) {
        m_match_min = 0;
        m_match_max = m_keywords.size();
        m_scanned = "";
    }
    if (!m_case_sensitive)
        ch = toupper(ch);
    m_scanned += (char)ch;
    auto len = m_scanned.length();
    auto fullmatch = -1;

    for (auto ix = m_match_min; ix < m_match_max; ix++) {
        std::string kw = m_keywords[ix].token.value();
        auto cmp = kw.compare(m_scanned);
        if (cmp < 0) {
            m_match_min = ix + 1;
        } else if (cmp == 0) {
            fullmatch = (int)ix;
        } else { /* cmp > 0 */
            if (m_scanned.substr(0, len) != kw.substr(0, len)) {
                m_match_max = ix;
                break;
            }
        }
    }

    if (m_match_min > m_match_max) {
        m_matchcount = 0;
    } else {
        m_matchcount = m_match_max - m_match_min;
    }

    /*
     * Determine new state.
     */
    switch (m_matchcount) {
    case 0:
        /*
         * No matches. This means that either there wasn't any match at all, or
         * we lost the match.
         */
        switch (m_state) {
        case KeywordScannerState::FullMatchAndPrefixes:
        case KeywordScannerState::FullMatch:

            /*
             * We had a full match (and maybe some additional prefix matches to)
             * but now lost it or all of them:
             */
            m_state = KeywordScannerState::FullMatchLost;
            break;

        case KeywordScannerState::PrefixesMatched:
        case KeywordScannerState::PrefixMatched:

            /*
             * We had one or more prefix matches, but lost it or all of them:
             */
            m_state = KeywordScannerState::PrefixMatchLost;
            break;

        default:

            /*
             * No match at all.
             */
            m_state = KeywordScannerState::NoMatch;
            break;
        }
        break;
    case 1:

        /*
         * Only one match. If it's a full match, i.e. the token matches the
         * keyword, we have a full match. Otherwise it's a prefix match.
         */
        m_fullmatch = fullmatch;
        m_state = (fullmatch >= 0)
            ? KeywordScannerState::FullMatch
            : KeywordScannerState::PrefixMatched;
        break;

    default: /* m_matchcount > 1 */

        /*
         * More than one match. If one of them is a full match, i.e. the token
         * matches exactly the keyword, it's a full-and-prefix match, otherwise
         * it's a prefixes-match.
         */
        m_fullmatch = fullmatch;
        m_state = (fullmatch >= 0)
            ? KeywordScannerState::FullMatchAndPrefixes
            : KeywordScannerState::PrefixesMatched;
        break;
    }
    debug(lexer, "_kw_scanner_match: scanned: '{s}' matchcount: {d} match_min: {d}, match_max: {d} new state {}",
        m_scanned, m_matchcount, m_match_min, m_match_max, KeywordScannerState_name(m_state));
}

void KeywordScanner::reset()
{
    m_state = KeywordScannerState::Init;
    m_matchcount = 0;
    m_fullmatch = -1;
}

void KeywordScanner::match(Tokenizer& tokenizer)
{
    if (m_keywords.empty()) {
        debug(lexer, "No keywords...");
        return;
    }

    reset();
    bool carry_on { true };
    for (int ch = tokenizer.get_char();
         ch && carry_on;
         ch = tokenizer.get_char()) {
        match_character(ch);

        carry_on = false;
        switch (m_state) {
        case KeywordScannerState::NoMatch:
            break;

        case KeywordScannerState::FullMatch:
        case KeywordScannerState::FullMatchAndPrefixes:
        case KeywordScannerState::PrefixesMatched:
        case KeywordScannerState::PrefixMatched:
            carry_on = true;
            break;

        case KeywordScannerState::PrefixMatchLost:
            /*
             * We lost the match, but there was never a full match.
             */
            m_state = KeywordScannerState::NoMatch;
            break;

        case KeywordScannerState::FullMatchLost:
            /*
             * Here we have to do a heuristic hack: suppose we have the
             * keyword 'for' and we're matching against 'format', that
             * obviously shouldn't match. So the hack here is that if
             * we lost the match we only recognize the keyword if the
             * keyword is an operator, which is loosely defined as a keyword
             * that ends in a non-alphanumeric, non-underscore character.
             */
            if (m_keywords[m_fullmatch].is_operator || (!isalnum(ch) && (ch != '_')))
                break;
            m_state = KeywordScannerState::NoMatch;
            break;
        default:
            fatal("Unreachable");
        }
        if (carry_on) {
            tokenizer.push();
        }
    }

    debug(lexer, "KeywordScanner::match returns '{s}' {d}", KeywordScannerState_name(m_state), (int) m_state);
    if ((m_state == KeywordScannerState::FullMatchLost) || (m_state == KeywordScannerState::FullMatch)) {
        tokenizer.accept(m_keywords[m_fullmatch].token.code());
    }
}

}
