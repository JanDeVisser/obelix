//
// Created by Jan de Visser on 2021-10-07.
//

#pragma once

#include <lexer/Lexer.h>
#include <gtest/gtest.h>

namespace Obelix {

struct Lexa {
    explicit Lexa(char const* txt = nullptr, bool dbg = false)
        : text(txt)
    {
        if (dbg)
            Obelix::Logger::get_logger().enable("lexer");
        else
            Obelix::Logger::get_logger().disable("lexer");
    }

    void tokenize(char const*txt = nullptr)
    {
        if (txt)
            text = txt;
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
        for (auto ix = 0; (ix < count) && (ix < tokens.size()); ix++) {
            TokenCode code = va_arg(codes, TokenCode);
            EXPECT_EQ(tokens[ix].code_name(), TokenCode_name(code));
        }
    }

    size_t count_tokens_with_code(TokenCode code)
    {
        return tokens_by_code[code].size();
    }

    template<class ScannerClass, class... Args>
    std::shared_ptr<ScannerClass> add_scanner(Args&&... args)
    {
        auto ret = lexer.add_scanner<ScannerClass>(std::forward<Args>(args)...);
        return ret;
    }

    Lexer lexer;
    char const* text;
    std::vector<Token> tokens;
    std::unordered_map<TokenCode, std::vector<Token>> tokens_by_code {};
};

}

class LexerTestF : public ::testing::Test {
public:
    Obelix::Lexa lexa;

protected:
    void SetUp() override {
        if (debugOn()) {
            Obelix::Logger::get_logger().enable("lexer");
        }
    }

    void initialize()
    {
        lexa = Obelix::Lexa();
        lexa.add_scanner<Obelix::QStringScanner>();
        lexa.add_scanner<Obelix::IdentifierScanner>();
        lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
    }

    void TearDown() override {
    }

    virtual bool debugOn() {
        return false;
    }

};

