/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <string>

#include <lexer/Token.h>
#include <oblasm/Assembler.h>
#include <oblasm/AssemblyTypes.h>
#include <oblasm/Directive.h>
#include <oblasm/Image.h>
#include <oblasm/Instruction.h>

namespace Obelix::Assembler {

Assembler::Assembler(Image& image)
    : BasicParser()
    , m_image(image)
{
    lexer().add_scanner<QStringScanner>();
    lexer().add_scanner<IdentifierScanner>();
    lexer().add_scanner<NumberScanner>(Obelix::NumberScanner::Config { false, false, true, true, false });
    lexer().add_scanner<WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, true, false });
    lexer().add_scanner<CommentScanner>(
        CommentScanner::CommentMarker { false, false, "/*", "*/" },
        CommentScanner::CommentMarker { false, true, "//", "" },
        CommentScanner::CommentMarker { false, true, ";", "" });
    lexer().filter_codes(TokenCode::Whitespace, TokenCode::Comment);
    lexer().add_scanner<KeywordScanner>(false,
#undef __Mnemonic
#define __Mnemonic(value, arg_template, num) \
    Token(Keyword##value, #value),
            __ENUM_Mnemonic(__Mnemonic)
#undef __Mnemonic
        Token(KeywordA, "a"),
        Token(KeywordB, "b"),
        Token(KeywordC, "c"),
        Token(KeywordD, "d"),
        Token(KeywordAB, "ab"),
        Token(KeywordCD, "cd"),
        Token(KeywordSi, "si"),
        Token(KeywordDi, "di"),
        Token(KeywordSP, "sp"),
        Token(KeywordSegment, "segment"),
        Token(KeywordDefine, "define"),
        Token(KeywordInclude, "include"),
        Token(KeywordAlign, "align"));
}

void Assembler::parse(std::string const& text)
{
    lexer().assign(text);
    do {
        auto token = peek();
        switch (token.code()) {
        case TokenCode::EndOfFile:
            lex();
            return;
        case TokenCode::NewLine:
            lex();
            break;
        case TokenCode::Period:
            lex();
            parse_directive();
            break;
        case TokenCode::Identifier: {
            auto const& identifier = token.value();
            lex();
            parse_label(identifier);
        }
        default:
            parse_mnemonic();
            break;
        }
    } while (true);
}

void Assembler::parse_label(std::string const& label)
{
    if (!expect(TokenCode::Colon))
        return;
    auto lbl = std::make_shared<Label>(label, m_image.current_address());
    m_image.add(lbl);
}

void Assembler::parse_directive()
{
    switch(current_code()) {
    case KeywordSegment: {
        lex();
        // FIXME also match decimal (and binary) numbers
        auto start_address = match(TokenCode::HexNumber);
        if (!start_address)
            return;
        auto segment = std::make_shared<Segment>(start_address.value().value());
        m_image.add(segment);
        break;
    }
    case KeywordInclude: {
        lex();
        auto include_file = match(TokenCode::DoubleQuotedString);
        auto assembler = Assembler(m_image);
        assembler.parse(OblBuffer(include_file->value()).buffer().str());
        break;
    }
    case KeywordAlign: {
        lex();
        auto boundary = match(TokenCode::Integer);
        if (!boundary)
           return;
        auto align = std::make_shared<Align>(boundary.value().value());
        m_image.add(align);
        break;
    }
    case KeywordDefine: {
        lex();
        auto label = match(TokenCode::Identifier);
        if (!label)
            return;
        auto value = match(TokenCode::HexNumber);
        if (!value)
            return;
        auto define = std::make_shared<Define>(label.value().value(), value.value().value()); // lol
        m_image.add(std::dynamic_pointer_cast<Label>(define)); // ugly
        break;
    }
    default:
        add_error(peek(), format("Unexpected directive '{}'", peek().value()));
        break;
    }
}

void Assembler::parse_mnemonic()
{
    auto m_maybe = get_mnemonic(current_code());
    if (!m_maybe.has_value()) {
        add_error(peek(), "Expected mnemonic");
        return;
    }
    auto m = m_maybe.value();
    auto token = lex();

    std::shared_ptr<Entry> entry = nullptr;
    switch (m) {
    case Mnemonic::DB:
    case Mnemonic::DW:
    case Mnemonic::DDW:
    case Mnemonic::DLW: {
        std::vector<std::string> data;
        while ((current_code() != TokenCode::NewLine) && (current_code() != TokenCode::EndOfFile)) {
            auto data_token = lex();
            data.push_back(data_token.value());
        }
        entry = std::make_shared<Bytes>(m, join(data, ' '));
        break;
    }
    case Mnemonic::BUFFER:
        if (auto buffer_size = match(TokenCode::HexNumber); buffer_size.has_value())
            entry = std::make_shared<Buffer>(m, buffer_size.value().value());
        break;
    case Mnemonic::ASCIZ:
    case Mnemonic::STR:
        if (current_code() == TokenCode::DoubleQuotedString || current_code() == TokenCode::SingleQuotedString) {
            entry = std::make_shared<String>(m, lex().value());
        }
        break;
    default: {
        Argument dest, source;
        auto dest_maybe = parse_argument();
        if (dest_maybe.is_error()) {
            return;
        }
        dest = dest_maybe.value();
        if (dest.valid() && (current_code() == TokenCode::Comma)) {
            lex();
            auto source_maybe = parse_argument();
            if (source_maybe.is_error()) {
                return;
            }
            source = source_maybe.value();
            if (!source.valid()) {
                add_error(peek(), "Could not parse source argument");
                return;
            }
        }
        entry = std::make_shared<Instruction>(m, dest, source);
        break;
    }
    }
    m_image.add(entry);
    for (auto& e : entry->errors()) {
        add_error(token, e);
    }
}

ErrorOr<Argument> Assembler::parse_argument()
{
    Argument ret;
    if (current_code() == TokenCode::Asterisk) {
        lex();
        ret.indirect = true;
        if (auto reg_maybe = get_register(peek().value()); reg_maybe.has_value()) {
            auto token = lex();
            if (reg_maybe.value().bits != 16) {
                add_error(token, format("Only 16-bit registers can be used in indirect addressing"));
                return Error { ErrorCode::SyntaxError, "Only 16-bit registers can be used in indirect addressing" };
            }
            ret.type = Argument::ArgumentType::Register;
            ret.reg = reg_maybe.value().reg;
            return ret;
        }
        if ((current_code() == TokenCode::Integer) || (current_code() == TokenCode::HexNumber) || (current_code() == TokenCode::BinaryNumber)) {
            auto token = lex();
            ret.type = Argument::ArgumentType::Constant;
            ret.constant = to_long_unconditional(token.value());
            return ret;
        }
        if (current_code() == TokenCode::Percent) {
            lex();
            auto token_maybe = match(TokenCode::Identifier);
            if (!token_maybe.has_value()) {
                return Error { ErrorCode::SyntaxError, "Expected label name after '%'" };
            }
            ret.type = Argument::ArgumentType::Label;
            ret.label = token_maybe.value().value();
            return ret;
        }
    }
    if (current_code() == TokenCode::Pound) {
        lex();
        ret.indirect = false;
        if ((current_code() == TokenCode::Integer) || (current_code() == TokenCode::HexNumber) || (current_code() == TokenCode::BinaryNumber)) {
            auto token = lex();
            ret.type = Argument::ArgumentType::Constant;
            ret.constant = to_long_unconditional(token.value());
            return ret;
        }
        if (current_code() == TokenCode::Percent) {
            lex();
            auto token_maybe = match(TokenCode::Identifier);
            if (!token_maybe.has_value()) {
                return Error { ErrorCode::SyntaxError, "Expected label name after '%'" };
            }
            ret.type = Argument::ArgumentType::Label;
            ret.label = token_maybe.value().value();
            return ret;
        }
    }
    if (auto reg_maybe = get_register(peek().value()); reg_maybe.has_value()) {
        auto reg = reg_maybe.value();
        auto token = lex();
        ret.type = Argument::ArgumentType::Register;
        ret.reg = reg.reg;
        return ret;
    }
    return ret;
}

}
