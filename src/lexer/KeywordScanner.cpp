//
// Created by Jan de Visser on 2021-10-06.
//

#include <lexer/Tokenizer.h>

namespace Obelix {

size_t KeywordScanner::s_next_identifier = 100;

void KeywordScanner::sort_keywords()
{
    std::sort(m_keywords.begin(), m_keywords.end(), [](Token const& a, Token const& b) {
        return a.value() < b.value();
    });
}


void KeywordScanner::match_character(int ch)
{
    if (m_state == KeywordScannerState::Init) {
        m_match_min = 0;
        m_match_max = m_keywords.size();
        m_scanned = "";
    }
    auto len = m_scanned.length();
    m_scanned += (char)ch;
//    debug(lexer, "Matching '%c' scanned '%s'", ch, m_scanned.c_str());

    for (auto ix = m_match_min; ix < m_match_max; ix++) {
        std::string kw = m_keywords[ix].value();
        auto cmp = kw.compare(m_scanned);
        if (cmp < 0) {
            m_match_min = ix + 1;
        } else if (cmp == 0) {
            m_token = m_keywords[ix];
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
    debug(lexer, "_kw_scanner_match: scanned: {s} matchcount: {d} match_min: {d}, match_max: {d}",
        m_scanned, m_matchcount, m_match_min, m_match_max);

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
        m_state = (m_token.code() != TokenCode::Unknown)
            ? KeywordScannerState::FullMatch
            : KeywordScannerState::PrefixMatched;
        break;

    default: /* m_matchcount > 1 */

        /*
         * More than one match. If one of them is a full match, i.e. the token
         * matches exactly the keyword, it's a full-and-prefix match, otherwise
         * it's a prefixes-match.
         */
        m_state = (m_token.code() != TokenCode::Unknown)
            ? KeywordScannerState::FullMatchAndPrefixes
            : KeywordScannerState::PrefixesMatched;
        break;
    }
}

void KeywordScanner::reset()
{
    m_state = KeywordScannerState::Init;
    m_matchcount = 0;
    m_token = Token();
}

void KeywordScanner::match()
{
    if (m_keywords.empty()) {
        debug(lexer, "No keywords...");
        return;
    }

    reset();
    bool carry_on { true };
    for (int ch = tokenizer().get_char();
         ch && carry_on;
         ch = tokenizer().get_char()) {
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
            break;
        default:
            fatal("Unreachable");
        }
        if (carry_on) {
            tokenizer().push();
        }
    };

    debug(lexer, "KeywordScanner::match returns '{s}' {d}", KeywordScannerState_name(m_state), m_state);
    if ((m_state == KeywordScannerState::FullMatchLost) || (m_state == KeywordScannerState::FullMatch)) {
        tokenizer().accept(m_token.code());
    }
}

}