/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-10-05.
//

#include "lexer/Token.h"
#include <lexer/Tokenizer.h>

namespace Obelix {

NumberScanner::NumberScanner()
    : Scanner()
{
}

NumberScanner::NumberScanner(Config const& config)
    : Scanner()
    , m_config(config)
{
}

TokenCode NumberScanner::process(Tokenizer& tokenizer, int ch)
{
    TokenCode code = TokenCode::Unknown;

    switch (m_state) {
    case NumberScannerState::None:
        if (m_config.sign && ((ch == '-') || (ch == '+'))) {
            m_state = NumberScannerState::PlusMinus;
        } else if (ch == '0') {
            m_state = NumberScannerState::Zero;
        } else if (isdigit(ch)) {
            m_state = NumberScannerState::Number;
        } else if (m_config.fractions && (ch == '.')) {
            m_state = NumberScannerState::LeadingPeriod;
        } else if (m_config.dollar_hex && (ch == '$')) {
            m_state = NumberScannerState::HexIntegerStart;
        } else {
            m_state = NumberScannerState::Done;
            code = TokenCode::Unknown;
        }
        break;

    case NumberScannerState::PlusMinus:
        if (ch == '0') {
            m_state = NumberScannerState::Zero;
        } else if (m_config.fractions && (ch == '.')) {
            m_state = NumberScannerState::Period;
        } else if (isdigit(ch)) {
            m_state = NumberScannerState::Number;
        } else {
            m_state = NumberScannerState::Done;
            code = TokenCode::Unknown;
        }
        break;

    case NumberScannerState::LeadingPeriod:
        if (isdigit(ch)) {
            m_state = NumberScannerState::Float;
        } else {
            m_state = NumberScannerState::Done;
            code = TokenCode::Unknown;
        }
        break;

    case NumberScannerState::Period:
        if (isdigit(ch)) {
            m_state = NumberScannerState::Float;
        } else if (m_config.scientific && (ch == 'e') && (tokenizer.token().length() > 1)) {
            m_state = NumberScannerState::SciFloat;
        } else {
            tokenizer.partial_rewind(1);
            m_state = NumberScannerState::Done;
            code = TokenCode::Integer;
        }
        break;

    case NumberScannerState::Zero:
        if (ch == '0') {
            /*
             * Chop the previous zero and keep the state. This zero will be chopped
             * next time around.
             */
            tokenizer.chop();
        } else if (isdigit(ch)) {
            /*
             * We don't want octal numbers. Therefore we strip
             * leading zeroes.
             */
            tokenizer.chop();
            m_state = NumberScannerState::Number;
        } else if (m_config.fractions && (ch == '.')) {
            m_state = NumberScannerState::Period;
        } else if (m_config.hex && (ch == 'x')) {
            /*
             * Hexadecimals are returned including the leading
             * 0x. This allows us to send both base-10 and hex ints
             * to strtol and friends.
             */
            m_state = NumberScannerState::HexIntegerStart;
        } else {
            m_state = NumberScannerState::Done;
            code = TokenCode::Integer;
        }
        break;

    case NumberScannerState::Number:
        if (m_config.fractions && (ch == '.')) {
            m_state = NumberScannerState::Period;
        } else if (m_config.scientific && (ch == 'e')) {
            m_state = NumberScannerState::SciFloat;
        } else if (!isdigit(ch)) {
            m_state = NumberScannerState::Done;
            code = TokenCode::Integer;
        }
        break;

    case NumberScannerState::Float:
        if (m_config.scientific &&  (ch == 'e')) {
            m_state = NumberScannerState::SciFloat;
        } else if (!isdigit(ch)) {
            m_state = NumberScannerState::Done;
            code = TokenCode::Float;
        }
        break;

    case NumberScannerState::SciFloat:
        if ((ch == '+') || (ch == '-')) {
            m_state = NumberScannerState::SciFloatExpSign;
        } else if (isdigit(ch)) {
            m_state = NumberScannerState::SciFloatExp;
        } else {
            m_state = NumberScannerState::Error;
        }
        break;

    case NumberScannerState::SciFloatExp:
        if (!isdigit(ch)) {
            m_state = NumberScannerState::Done;
            code = TokenCode::Float;
        }
        break;

    case NumberScannerState::SciFloatExpSign:
        if (isdigit(ch)) {
            m_state = NumberScannerState::SciFloatExp;
        } else {
            m_state = NumberScannerState::Error;
        }
        break;

    case NumberScannerState::HexIntegerStart:
        if (!isxdigit(ch)) {
            m_state = NumberScannerState::Error;
        } else {
            m_state = NumberScannerState::HexInteger;
        }
        break;

    case NumberScannerState::HexInteger:
        if (!isxdigit(ch)) {
            m_state = NumberScannerState::Done;
            code = TokenCode::HexNumber;
        }
        break;

    default:
        oassert(false, "Unreachable");
    }
    if ((m_state != NumberScannerState::Done) && (m_state != NumberScannerState::Error)) {
        tokenizer.push();
    }
    return code;
}

void NumberScanner::match(Tokenizer& tokenizer)
{
    int ch;
    TokenCode code;
    Token ret;

    for (m_state = NumberScannerState::None;
         (m_state != NumberScannerState::Done) && (m_state != NumberScannerState::Error);) {
        ch = tolower(tokenizer.get_char());
        code = process(tokenizer, ch);
    }
    if (m_state == NumberScannerState::Error) {
        tokenizer.accept_token(TokenCode::Error, "Malformed number");
    } else if (code != TokenCode::Unknown) {
        tokenizer.accept(code);
    }
}

}
