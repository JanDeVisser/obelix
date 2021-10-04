//
// Created by Jan de Visser on 2021-09-20.
//

#pragma once

#include "Token.h"
#include "core/Arguments.h"
#include "core/Object.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define SCANNER_CONFIG_SEPARATORS ",.;"
#define PARAM_PRIORITY            "priority"
#define PARAM_CONFIGURATION       "configuration"

#define ENUMERATE_LEXER_WHERE(S) \
    S(LexerWhereBegin)           \
    S(LexerWhereMiddle)          \
    S(LexerWhereEnd)

enum class LexerWhere {
#undef _LEXER_WHERE
#define _LEXER_WHERE(w) w,
    ENUMERATE_LEXER_WHERE(_LEXER_WHERE)
#undef _LEXER_WHERE
};

#define ENUMERATE_LEXER_STATE(S) \
    S(LexerStateNoState)         \
    S(LexerStateFresh)           \
    S(LexerStateInit)            \
    S(LexerStateSuccess)         \
    S(LexerStateDone)            \
    S(LexerStateStale)

enum class LexerState {
#undef _LEXER_STATE
#define _LEXER_STATE(state) state,
    ENUMERATE_LEXER_STATE(_LEXER_STATE)
#undef _LEXER_STATE
};

class Lexer;
class LexerConfig;
class ScannerConfig;
class Scanner;

typedef std::function<ScannerConfig*(LexerConfig const&, std::string const&)> ScannerConfigFactory;

class ScannerConfig : public Object {
public:
    static void register_config(std::string const&, ScannerConfigFactory const&);
    static ScannerConfig * create(std::string const&, LexerConfig const&);
    explicit ScannerConfig(LexerConfig const&, std::string const& = "");
    ~ScannerConfig() = default;

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<Object> resolve(std::string const&) const override;
    void set(std::string const&, std::shared_ptr<Object> value);
    void dump() const;

    std::shared_ptr<Object> operator()(std::shared_ptr<Arguments>) override;

protected:
    virtual std::optional<Token> match(Scanner&) { return {}; }
    virtual std::optional<Token> match_2nd_pass(Scanner&) { return {}; };
    virtual void dump_config() const { }
    [[nodiscard]] virtual std::unordered_map<std::string, std::shared_ptr<Object>> get_config() const { return {}; }
    void set_string(std::string const&);
    void set_value(std::string const&, std::shared_ptr<Object> const&);
    void configure(Object const&);

private:
    [[nodiscard]] std::string config_to_string() const;

    static std::unordered_map<std::string, ScannerConfigFactory> s_configs;
    int m_priority { 0 };
    LexerConfig const& m_lexer_config;
    std::unordered_map<std::string, std::shared_ptr<Object>> m_config { };
};

class Scanner : public Object {
public:
    Scanner(ScannerConfig const&, Lexer&);
    [[nodiscard]] std::string to_string() const;
private:
    ScannerConfig const& m_config;
    Lexer& m_lexer;
    int m_state;
    void* m_data;
};

class LexerConfig {
public:
private:
    std::vector<std::shared_ptr<ScannerConfig>> m_scanners;
    size_t      m_bufsize;
//    char       *build_func;
//    data_t     *data;
};

class Lexer {
public:
private:
    std::shared_ptr<LexerConfig> m_config;
    std::vector<std::shared_ptr<Scanner>> m_scanners;
    std::string m_buffer;
    std::string m_token;
    LexerState m_state;
    LexerWhere m_where;
    Token m_last_token;
    int m_scanned;
    int m_count;
    int m_scan_count;
    int m_current;
    int m_prev_char;
    int m_line;
    int m_column;
    void *data;
};

class CommentScanner : public ScannerConfig {
public:
    CommentScanner(LexerConfig const&, std::string const&);
};

class IdentifierScanner : public ScannerConfig {
public:
    IdentifierScanner(LexerConfig const&, std::string const&);
};

class KeywordScanner : public ScannerConfig {
public:
    KeywordScanner(LexerConfig const&, std::string const&);
};

class NumberScanner : public ScannerConfig {
public:
    NumberScanner(LexerConfig const&, std::string const&);
};

class PositionScanner : public ScannerConfig {
public:
    PositionScanner(LexerConfig const&, std::string const&);
};

class QStringScanner : public ScannerConfig {
public:
    QStringScanner(LexerConfig const&, std::string const&);
};

class WhitespaceScanner : public ScannerConfig {
public:
    WhitespaceScanner(LexerConfig const&, std::string const&);
};



std::string LexerState_name(LexerState);
