/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-10-07.
//

#pragma once

#include <cstdarg>
#include <gtest/gtest.h>
#include <lexer/Lexer.h>
#include <lexer/Tokenizer.h>

class LexerTest : public ::testing::Test {
public:
    Obelix::Lexer lexer;

protected:
    void SetUp() override {
        if (debugOn()) {
            Obelix::Logger::get_logger().enable("lexer");
        }
    }

    void initialize()
    {
        lexer = Obelix::Lexer();
        lexer.add_scanner<Obelix::QStringScanner>();
        lexer.add_scanner<Obelix::IdentifierScanner>();
        lexer.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
    }

    void tokenize(std::string const& s)
    {
        tokenize(s.c_str());
    }

    void tokenize(char const* text = nullptr)
    {
        tokens = lexer.tokenize(text);
        for (auto& token : tokens) {
            auto tokens_for = tokens_by_code[token.code()];
            tokens_for.push_back(token);
            tokens_by_code[token.code()] = tokens_for;
        }
    }

    void check_codes(size_t count, ...)
    {
        EXPECT_EQ(count, tokens.size());
        va_list codes;
        va_start(codes, count);
        for (auto ix = 0u; (ix < count) && (ix < tokens.size()); ix++) {
            Obelix::TokenCode code = va_arg(codes, Obelix::TokenCode);
            EXPECT_EQ(tokens[ix].code_name(), TokenCode_name(code));
        }
    }

    size_t count_tokens_with_code(Obelix::TokenCode code)
    {
        return tokens_by_code[code].size();
    }

    template<class ScannerClass, class... Args>
    std::shared_ptr<ScannerClass> add_scanner(Args&&... args)
    {
        auto ret = lexer.add_scanner<ScannerClass>(std::forward<Args>(args)...);
        return ret;
    }

    std::vector<Obelix::Token> tokens;
    std::unordered_map<Obelix::TokenCode, std::vector<Obelix::Token>> tokens_by_code {};

    void TearDown() override {
    }

    virtual bool debugOn() {
        return false;
    }

};
