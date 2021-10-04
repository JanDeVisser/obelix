//
// Created by Jan de Visser on 2021-09-20.
//

#include "Lexer.h"
#include "core/StringUtil.h"

std::unordered_map<std::string, ScannerConfigFactory> ScannerConfig::s_configs = {};

void ScannerConfig::register_config(std::string const& code, ScannerConfigFactory const& factory)
{
    s_configs.insert_or_assign(code, factory);
}

ScannerConfig * ScannerConfig::create(std::string const& config, LexerConfig const& lexer_config)
{
    if (s_configs.empty()) {
        register_config("comment", [](LexerConfig const& lexer_config, std::string const& config) { return new CommentScanner(lexer_config, config); });
        register_config("identifier", [](LexerConfig const& lexer_config, std::string const& config) { return new IdentifierScanner(lexer_config, config); });
        register_config("keyword", [](LexerConfig const& lexer_config, std::string const& config) { return new KeywordScanner(lexer_config, config); });
        register_config("number", [](LexerConfig const& lexer_config, std::string const& config) { return new NumberScanner(lexer_config, config); });
        register_config("position", [](LexerConfig const& lexer_config, std::string const& config) { return new PositionScanner(lexer_config, config); });
        register_config("qstring", [](LexerConfig const& lexer_config, std::string const& config) { return new QStringScanner(lexer_config, config); });
        register_config("whitespace", [](LexerConfig const& lexer_config, std::string const& config) { return new WhitespaceScanner(lexer_config, config); });
    }
    auto code_config = split(config, ':');
    assert(code_config.size() == 2);
    assert(s_configs.contains(code_config[0]));
    auto factory = s_configs[code_config[0]];
    return factory(lexer_config, code_config[1]);
}

ScannerConfig::ScannerConfig(LexerConfig const& lexer_config, std::string const& config)
    : Object("scannerconfig")
    , m_lexer_config(lexer_config)
{
    configure(String(config));
}

std::string ScannerConfig::config_to_string() const
{
    std::string ret;
    auto config = get_config();
    if (!config.empty()) {
        bool first = true;
        for (auto& cfg : config) {
            if (!first) {
                ret += ";";
            }
            ret += cfg.first + "=" + cfg.second->to_string();
            first = false;
        }
    } else if (!m_config.empty()) {
        bool first = true;
        for (auto& cfg : m_config) {
            if (!first) {
                ret += ";";
            }
            ret += cfg.first + "=" + cfg.second->to_string();
            first = false;
        }
    }
    return ret;
}

std::string ScannerConfig::to_string() const
{
    auto ret = type();
    auto config = config_to_string();
    if (!config.empty()) {
        ret += ": " + config;
    }
    return ret;
}

std::shared_ptr<Object> ScannerConfig::resolve(std::string const& name) const {
    std::shared_ptr<Object> ret;

    if (name == PARAM_CONFIGURATION) {
        return std::make_shared<String>(config_to_string());
    } else if (name == PARAM_PRIORITY) {
        return std::make_shared<Number>(m_priority);
    } else if (m_config.contains(name)) {
        return m_config[name];
    }
    return Object::resolve(name);
}

void ScannerConfig::set(std::string const& name, std::shared_ptr<Object> value)
{
    if (name == PARAM_CONFIGURATION) {
        configure(value);
    } else if (name == PARAM_PRIORITY) {
        {
            auto priority_maybe = value->to_int();
            if (priority_maybe.has_value()) {
                m_priority = priority_maybe.value();
            }
        }
    } else {
        m_config[name] = value;
    }
}

void ScannerConfig::set_string(std::string const& value)
{
    auto nv_pairs = parse_pairs(value);
    for (auto& nvp : nv_pairs) {
        if (nvp.first !="configuration") {
            set_value(nvp.first, std::make_shared<String>(nvp.second));
        }
    }
}

std::shared_ptr<Object> ScannerConfig::operator()(std::shared_ptr<Arguments> args)
{
    return std::make_shared<Scanner>(*this, *std::dynamic_pointer_cast<Lexer>(args->get(0)));
}

void ScannerConfig::set_value(std::string const& name, std::shared_ptr<Object> const& value)
{
    m_config[name] = value;
}

void ScannerConfig::configure(Object const& value) {
    if (value.type() == "nvp") {
        NVP const& nvp = dynamic_cast<NVP const&>(value);
        set_value(nvp.name(), nvp.value());
    } else if (value.type() != "null") {
        auto params = parse_pairs(value.to_string());
        for (auto &param : params) {
            set_value(param.first, std::make_shared<String>(param.second));
        }
    }
}

void ScannerConfig::dump() const {
    auto escaped = c_escape(to_string());
    printf("  scanner_config = lexer_config.add_scanner(\"%s\");\n", escaped.c_str());
    dump_config();
}
