//
// Created by Jan de Visser on 2021-09-18.
//

#include <core/Format.h>
#include <core/Object.h>
#include <core/StringUtil.h>

namespace Obelix {

enum class FormatState {
    String,
    OpenBrace,
    Format
};

enum class FormatSpecifierType {
    String,
    Int,
    Float,
};

enum class FormatSpecifierAlignment {
    Left,
    Right,
};

class FormatSpecifier {
public:
    explicit FormatSpecifier(std::string const& specifier)
    {
        std::string work;
        if (!specifier.empty()) {
            switch (specifier[specifier.length() - 1]) {
            case 's':
                m_type = FormatSpecifierType::String;
                break;
            case 'x':
                m_base = 16;
                /* fall through */
            case 'd':
                m_type = FormatSpecifierType::Int;
                break;
            case 'f':
                m_type = FormatSpecifierType::Float;
                break;
            default:
                assert(false);
            }
        }
        work = specifier.substr(0, specifier.length() - 1);
        if (work.empty())
            return;
        if (work[0] == '-')
            m_alignment = FormatSpecifierAlignment::Right;
        work = work.substr(1, work.length() - 1);
        if (work.empty())
            return;
        auto parts = split(work, '.');
        char* end;
        switch (parts.size()) {
        case 1:
            m_width = strtol(parts[0].c_str(), &end, 10);
            assert(end == work.c_str());
            break;
        case 2:
            if (!parts[0].empty()) {
                m_width = strtol(parts[0].c_str(), &end, 10);
                assert(end == work.c_str());
            }
            m_precision = strtol(parts[1].c_str(), &end, 10);
            assert(end == work.c_str());
            break;
        default:
            assert(false);
        }
    };

    [[nodiscard]] FormatSpecifierType type() const { return m_type; }
    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int precision() const { return m_precision; }
    [[nodiscard]] int base() const { return m_base; }
    [[nodiscard]] FormatSpecifierAlignment alignment() const { return m_alignment; }

    [[nodiscard]] Obj box_next(va_list args)
    {
        switch (type()) {
        case FormatSpecifierType::String:
            return to_obj(make_typed<String>(va_arg(args, char *)));
        case FormatSpecifierType::Int:
            return make_obj<Integer>(va_arg(args, int));
        case FormatSpecifierType::Float:
            return make_obj<Float>(va_arg(args, double));
        }
    }

private:
    FormatSpecifierType m_type { FormatSpecifierType::String };
    int m_width { 0 };
    int m_precision { 0 };
    int m_base { 10 };
    FormatSpecifierAlignment m_alignment = { FormatSpecifierAlignment::Left };
};

std::string fformat(std::string msg, ...)
{
    va_list args;
    va_start(args, msg);
    auto ret = vformat(msg, args);
    va_end(args);
    return ret;
}

std::string vformat(std::string const& msg, va_list args)
{
    std::string ret;
    std::vector<Obj> arg_list;
    FormatState state = FormatState::String;
    std::string format_specifier;
    for (auto& ch : msg) {
        switch (state) {
        case FormatState::String:
            if (ch != '{')
                ret += ch;
            else
                state = FormatState::OpenBrace;
            break;
        case FormatState::OpenBrace:
            if (ch != '{') {
                state = FormatState::Format;
                format_specifier = "";
            } else {
                ret += ch;
                state = FormatState::String;
            }
        case FormatState::Format:
            if (ch == '}') {
                FormatSpecifier specifier(format_specifier);
                arg_list.push_back(specifier.box_next(args));
                state = FormatState::String;
            } else {
                format_specifier += ch;
            }
        }
    }
    assert(state == FormatState::String);
    return format(msg, arg_list);
}

std::string format(std::string const& msg, std::vector<Obj> const& args)
{
    std::string ret = msg;
    for (auto& arg : args) {
        auto pos = ret.find_first_of("{}");
        if (pos != std::string::npos) {
            ret = ret.replace(pos, pos + 2, arg->to_string());
        }
    }
    return ret;
}

}