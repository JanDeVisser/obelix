/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-09-20.
//

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <core/StringBuffer.h>
#include <functional>
#include <lexer/Token.h>

namespace Obelix {

#define SCANNER_CONFIG_SEPARATORS ",.;"
#define PARAM_PRIORITY "priority"
#define PARAM_CONFIGURATION "configuration"

extern LoggingCategory lexer_logger;

#define ENUMERATE_TOKENIZER_STATES(S) \
    S(NoState)                        \
    S(Fresh)                          \
    S(Init)                           \
    S(Success)                        \
    S(Done)                           \
    S(Stale)

enum class TokenizerState {
#undef _TOKENIZER_STATE
#define _TOKENIZER_STATE(state) state,
    ENUMERATE_TOKENIZER_STATES(_TOKENIZER_STATE)
#undef _TOKENIZER_STATE
};

constexpr char const* TokenizerState_name(TokenizerState state)
{
    switch (state) {
#undef _TOKENIZER_STATE
#define _TOKENIZER_STATE(value) \
    case TokenizerState::value: \
        return #value;
        ENUMERATE_TOKENIZER_STATES(_TOKENIZER_STATE)
#undef _TOKENIZER_STATE
    default:
        assert(false);
    }
}

class Tokenizer;
class Scanner;

class Scanner {
public:
    explicit Scanner(int priority = 10)
        : m_priority(priority)
    {
    }
    virtual ~Scanner() = default;

    [[nodiscard]] int priority() const { return m_priority; }

    [[nodiscard]] virtual char const* name() const = 0;
    virtual void match(Tokenizer&) { }

    bool operator<(Obelix::Scanner const& other) const
    {
        if (priority() != other.priority())
            return priority() < other.priority();
        return strcmp(name(), other.name()) < 0;
    }

private:
    int m_priority { 0 };
};

class Tokenizer {
public:
    Tokenizer() = default;
    explicit Tokenizer(std::string_view const&, std::string = {});
    explicit Tokenizer(StringBuffer, std::string = {});

    template<typename... Args>
    void filter_codes(TokenCode code, Args&&... args)
    {
        m_filtered_codes.insert(code);
        filter_codes(std::forward<Args>(args)...);
    }

    void filter_codes()
    {
    }

    void filter_codes(std::unordered_set<TokenCode> codes)
    {
        m_filtered_codes.merge(codes);
    }

    [[nodiscard]] StringBuffer const& buffer() const { return m_buffer; }

    std::vector<Token> const& tokenize(std::optional<std::string_view const> = {});

    int get_char();
    void discard();
    Token accept(TokenCode);
    Token accept_token(TokenCode, std::string);
    Token accept_token(Token&);
    void push();
    void push_as(int);
    void skip();
    void chop(size_t = 1);
    [[nodiscard]] std::string const& token() const;
    [[nodiscard]] TokenizerState state() const;
    [[nodiscard]] bool at_top() const;
    [[nodiscard]] bool at_end() const;
    void reset();
    void rewind();
    void partial_rewind(size_t);
    Token get_accept(TokenCode, int);

    template<class ScannerClass, class... Args>
    std::shared_ptr<ScannerClass> add_scanner(Args&&... args)
    {
        auto ret = std::make_shared<ScannerClass>(std::forward<Args>(args)...);
        m_scanners.insert(std::dynamic_pointer_cast<Scanner>(ret));
        return ret;
    }

    void add_scanners(std::set<std::shared_ptr<Scanner>> scanners)
    {
        m_scanners.merge(scanners);
    }

    std::shared_ptr<Scanner> get_scanner(std::string const& name)
    {
        for (auto& scanner : m_scanners) {
            if (scanner->name() == name) {
                return scanner;
            }
        }
        return nullptr;
    }

private:
    void match_token();

    std::unordered_set<TokenCode> m_filtered_codes {};

    struct ScannerCmp {
        bool operator()(std::shared_ptr<Obelix::Scanner> const& a, std::shared_ptr<Obelix::Scanner> const& b) const
        {
            return (*a < *b);
        }
    };

    std::set<std::shared_ptr<Scanner>, ScannerCmp> m_scanners {};
    StringBuffer m_buffer;
    std::string m_token {};
    TokenizerState m_state { TokenizerState::Fresh };
    std::vector<Token> m_tokens {};
    size_t m_scanned { 0 };
    size_t m_consumed { 0 };
    size_t m_total_count { 0 };
    bool m_prev_was_cr { false };
    int m_current { 0 };
    Span m_location { {}, 1, 1, 1, 1 };
    bool m_eof { false };
};

#define ENUMERATE_QSTR_STATES(S) \
    S(Init)                      \
    S(QString)                   \
    S(Escape)                    \
    S(Done)

enum class QStrState {
#undef _QSTR_STATE
#define _QSTR_STATE(value) value,
    ENUMERATE_QSTR_STATES(_QSTR_STATE)
#undef _QSTR_STATE
};

class QStringScanner : public Scanner {
public:
    explicit QStringScanner(std::string = "'`\"");
    [[nodiscard]] std::string quotes() const { return m_quotes; }
    void match(Tokenizer& tokenizer) override;
    [[nodiscard]] char const* name() const override { return "qstring"; }

private:
    std::string m_quotes;
    char m_quote {};
    QStrState m_state { QStrState::Init };
};

#define ENUMERATE_WS_STATES(S) \
    S(Init)                    \
    S(Whitespace)              \
    S(CR)                      \
    S(Done)

class WhitespaceScanner : public Scanner {
public:
    enum class WhitespaceState {
#undef _WS_STATE
#define _WS_STATE(value) value,
        ENUMERATE_WS_STATES(_WS_STATE)
#undef _WS_STATE
    };

    struct Config {
        bool ignore_newlines { true };
        bool ignore_spaces { true };
        bool newlines_are_spaces { true };
    };
    WhitespaceScanner();
    explicit WhitespaceScanner(Config const&);
    explicit WhitespaceScanner(bool);
    void match(Tokenizer&) override;
    [[nodiscard]] char const* name() const override { return "whitespace"; }

private:
    Config m_config {};
    WhitespaceState m_state { WhitespaceState::Init };
};

#define ENUMERATE_COMMENT_STATES(S) \
    S(None)                         \
    S(StartMarker)                  \
    S(Text)                         \
    S(EndMarker)                    \
    S(End)                          \
    S(Unterminated)

class CommentScanner : public Scanner {
public:
    enum class CommentState {
#undef _COMMENT_STATE
#define _COMMENT_STATE(value) value,
        ENUMERATE_COMMENT_STATES(_COMMENT_STATE)
#undef _COMMENT_STATE
    };

    struct CommentMarker {
        bool hashpling;
        bool eol;
        std::string start;
        std::string end;
        bool matched { true };

        std::string to_string()
        {
            return format("{}{}{}{}",
                (hashpling) ? "^" : "", start,
                (!end.empty()) ? " " : "",
                (!end.empty()) ? end : "");
        }
    };

    explicit CommentScanner()
        : Scanner()
    {
    }

    template<typename... Args>
    explicit CommentScanner(Args&&... args)
        : Scanner()
    {
        add_markers(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    void add_markers(T t, Args&&... args)
    {
        add_marker(t);
        add_markers(std::forward<Args>(args)...);
    }

    void add_markers() { }

    void add_marker(CommentMarker marker)
    {
        m_markers.push_back(std::move(marker));
    }

    void add_marker(std::string marker)
    {
        m_markers.push_back({ false, true, std::move(marker), "" });
    }

    void add_marker(char const* marker)
    {
        add_marker(std::string(marker));
    }

    void match(Tokenizer&) override;
    [[nodiscard]] char const* name() const override { return "comment"; }

private:
    void find_eol(Tokenizer&);
    void find_end_marker(Tokenizer&);

    std::vector<CommentMarker> m_markers {};
    CommentState m_state { CommentState::None };
    std::vector<bool> m_matched;
    size_t m_num_matches { 0 };
    CommentMarker* m_match { nullptr };
    std::string m_token {};
};

#define ENUMERATE_NUMBER_SCANNER_STATES(S) \
    S(None)                                \
    S(PlusMinus)                           \
    S(Zero)                                \
    S(Number)                              \
    S(LeadingPeriod)                       \
    S(Period)                              \
    S(Float)                               \
    S(SciFloat)                            \
    S(SciFloatExpSign)                     \
    S(SciFloatExp)                         \
    S(HexIntegerStart)                     \
    S(HexInteger)                          \
    S(Done)                                \
    S(Error)

class NumberScanner : public Scanner {
public:
    enum class NumberScannerState {
#undef _NUMBER_SCANNER_STATE
#define _NUMBER_SCANNER_STATE(value) value,
        ENUMERATE_NUMBER_SCANNER_STATES(_NUMBER_SCANNER_STATE)
#undef _NUMBER_SCANNER_STATE
    };

    struct Config {
        bool scientific { true };
        bool sign { true };
        bool hex { true };
        bool dollar_hex { false };
        bool fractions { true };
    };
    NumberScanner();
    explicit NumberScanner(Config const&);
    void match(Tokenizer&) override;
    [[nodiscard]] char const* name() const override { return "number"; }

private:
    TokenCode process(Tokenizer&, int);

    NumberScannerState m_state { NumberScannerState::None };
    Config m_config {};
};

#define ENUMERATE_IDENTIFIER_CHARACTER_CLASSES(S) \
    S(CaseSensitive, 'X')                         \
    S(FoldToLower, 'l')                           \
    S(OnlyLower, 'a')                             \
    S(FoldToUpper, 'U')                           \
    S(OnlyUpper, 'A')                             \
    S(NoAlpha, 'Q')                               \
    S(Digits, '9')

constexpr char const* ALL_IDENTIFIER_CHARACTER_CLASSES = "XlUAaQ";

class IdentifierScanner : public Scanner {
public:
    enum class IdentifierCharacterClass {
#undef _ID_CHAR_CLASS
#define _ID_CHAR_CLASS(value, picture) value = picture,
        ENUMERATE_IDENTIFIER_CHARACTER_CLASSES(_ID_CHAR_CLASS)
#undef _ID_CHAR_CLASS
    };

    struct Config {
        TokenCode code { TokenCode::Identifier };
        std::string filter { "X9_" };
        std::string starts_with { "X_" };
        IdentifierCharacterClass alpha = IdentifierCharacterClass::CaseSensitive;
        IdentifierCharacterClass startswith_alpha = IdentifierCharacterClass::CaseSensitive;
        bool digits { true };
        bool startswith_digits { false };
    };

    IdentifierScanner();
    explicit IdentifierScanner(Config);
    void match(Tokenizer&) override;
    [[nodiscard]] char const* name() const override { return "identifier"; }

private:
    bool filter_character(Tokenizer&, int) const;

    Config m_config {};
};

#define ENUMERATE_KEYWORD_SCANNER_STATES(S) \
    S(Init)                                 \
    S(PrefixMatched)                        \
    S(PrefixesMatched)                      \
    S(FullMatch)                            \
    S(FullMatchAndPrefixes)                 \
    S(FullMatchLost)                        \
    S(PrefixMatchLost)                      \
    S(NoMatch)

class KeywordScanner : public Scanner {
public:
    struct Keyword {
        Token token;
        bool is_operator { true };
    };

    enum class KeywordScannerState {
#undef _KEYWORD_SCANNER_STATE
#define _KEYWORD_SCANNER_STATE(state) state,
        ENUMERATE_KEYWORD_SCANNER_STATES(_KEYWORD_SCANNER_STATE)
#undef _KEYWORD_SCANNER_STATE
    };

    static char const* KeywordScannerState_name(KeywordScannerState state)
    {
        switch (state) {
#undef _KEYWORD_SCANNER_STATE
#define _KEYWORD_SCANNER_STATE(value) \
    case KeywordScannerState::value:  \
        return #value;
            ENUMERATE_KEYWORD_SCANNER_STATES(_KEYWORD_SCANNER_STATE)
#undef _KEYWORD_SCANNER_STATE
        default:
            fatal("Unreachable");
        }
    }

    template<typename... Args>
    explicit KeywordScanner(bool case_sensitive, Args&&... args)
        : Scanner()
        , m_case_sensitive(case_sensitive)
    {
        add_keywords(std::forward<Args>(args)...);
    }

    template<typename... Args>
    explicit KeywordScanner(Args&&... args)
        : Scanner()
    {
        add_keywords(std::forward<Args>(args)...);
    }

    void match(Tokenizer&) override;
    [[nodiscard]] char const* name() const override { return "keyword"; }

    template<typename T, typename... Args>
    void add_keywords(T t, Args&&... args)
    {
        add_keyword(t);
        add_keywords(std::forward<Args>(args)...);
    }

    void add_keywords() { }

    void add_keyword(Token const&);
    void add_keyword(TokenCode);

private:
    void reset();
    void match_character(int);
    static size_t s_next_identifier;

    std::vector<Keyword> m_keywords;
    KeywordScannerState m_state { KeywordScannerState::Init };
    size_t m_matchcount { 0 };
    size_t m_match_min { 0 };
    size_t m_match_max { 0 };
    int m_fullmatch = -1;
    std::string m_scanned {};

    bool m_case_sensitive { true };
};

#if 0
class CommentScanner : public ScannerConfig {
public:
    CommentScanner(LexerConfig const&, std::string const&);
};

class PositionScanner : public ScannerConfig {
public:
    PositionScanner(LexerConfig const&, std::string const&);
};

#endif

}
