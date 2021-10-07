//
// Created by Jan de Visser on 2021-10-07.
//

#pragma once

#include <lexer/Lexer.h>
#include <gtest/gtest.h>

namespace Obelix {

struct Lexa {
    explicit Lexa(std::string const& text, bool dbg = false)
    {
        if (dbg)
            Obelix::Logger::get_logger().enable("lexer");
        lexer = Lexer(text);
    }

    void tokenize()
    {
        tokens = lexer.tokenize();
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
        for (auto ix = 0; ix < count; ix++) {
            TokenCode code = va_arg(codes, TokenCode);
            if (tokens[ix].code() != code)
                fprintf(stderr, "Expected %s, got %s\n", TokenCode_name(code), tokens[ix].to_string().c_str());
            EXPECT_EQ(tokens[ix].code(), code);
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
    std::vector<Token> tokens;
    std::unordered_map<TokenCode, std::vector<Token>> tokens_by_code {};
};

}
