//
// Created by Jan de Visser on 2021-09-20.
//

#include <cassert>
#include <core/StringUtil.h>

namespace Obelix {

std::string c_escape(std::string const& s)
{
    std::string ret;
    for (char c : s) {
        if (c == '"' || c == '\'' || c == '\\') {
            ret += "\\";
        }
        ret += c;
    }
    return ret;
}

std::vector<std::string> split(std::string const& s, char sep)
{
    auto start = 0u;
    auto ptr = 0u;
    std::vector<std::string> ret;
    do {
        start = ptr;
        for (; (ptr < s.length()) && (s[ptr] != sep); ptr++)
            ;
        ret.push_back(s.substr(start, ptr - start));
        start = ++ptr;
    } while (ptr < s.length());
    if (s[s.length() - 1] == sep) // This is ugly...
        ret.emplace_back("");
    return ret;
}

std::string join(std::vector<std::string> const& collection, std::string const& sep)
{
    std::string ret;
    auto first = true;
    for (auto& elem : collection) {
        if (!first) {
            ret += sep;
        }
        ret += elem;
        first = false;
    }
    return ret;
}

std::string join(std::vector<std::string> const& collection, char sep)
{
    std::string sep_str;
    sep_str += sep;
    return join(collection, sep_str);
}

std::string strip(std::string const& s)
{
    size_t start;
    for (start = 0; (start < s.length()) && std::isspace(s[start]); start++)
        ;
    if (start == s.length()) {
        return {};
    }
    size_t end;
    for (end = s.length() - 1; std::isspace(s[end]); end--)
        ;
    return s.substr(start, end - start + 1);
}

std::string rstrip(std::string const& s)
{
    size_t end;
    for (end = s.length() - 1; std::isspace(s[end]); end--)
        ;
    return s.substr(0, end + 1);
}

std::string lstrip(std::string const& s)
{
    size_t start;
    for (start = 0; (start < s.length()) && std::isspace(s[start]); start++)
        ;
    return s.substr(start, s.length() - start);
}

std::vector<std::pair<std::string, std::string>> parse_pairs(std::string const& s, char pair_sep, char name_value_sep)
{
    auto pairs = split(s, pair_sep);
    std::vector<std::pair<std::string, std::string>> ret;
    for (auto& pair : pairs) {
        auto nvp = split(strip(pair), name_value_sep);
        switch (nvp.size()) {
        case 0:
            break;
        case 1:
            if (!strip(nvp[0]).empty())
                ret.emplace_back(strip(nvp[0]), "");
            break;
        case 2:
            if (!strip(nvp[0]).empty())
                ret.emplace_back(strip(nvp[0]), strip(nvp[1]));
            break;
        default:
            if (!strip(nvp[0]).empty()) {
                std::vector<std::string> tail;
                for (auto ix = 1u; ix < nvp.size(); ix++) {
                    tail.push_back(nvp[ix]);
                }
                auto value = strip(join(tail, name_value_sep));
                ret.emplace_back(strip(nvp[0]), strip(value));
            }
        }
    }
    return ret;
}

std::string to_string(long value)
{
    char buf[80];
    snprintf(buf, 79, "%ld", value);
    return buf;
}

std::string to_hex_string(long value)
{
    char buf[80];
    snprintf(buf, 79, "%lx", value);
    return buf;
}

std::string to_string(double value)
{
    char buf[80];
    snprintf(buf, 79, "%f", value);
    return buf;
}

std::string to_string(bool value)
{
    return (value) ? "true" : "false";
}

bool check_zero(std::string const& str)
{
    int zeroes = 0;
    for (auto& ch : str) {
        switch (ch) {
        case 'x':
        case 'X':
            if (zeroes != 1)
                return false;
            break;
        case '0':
            zeroes += (ch == '0') ? 1 : 0;
            break;
        default:
            return false;
        }
    }
    return true;
}

std::optional<long> to_long(std::string const& str)
{
    char* end;
    errno = 0;
    auto ret = strtol(str.c_str(), &end, 0);
    if ((end != (str.c_str() + str.length())) || (errno != 0))
        return {};
    if ((ret == 0) && !check_zero(str))
        return {};
    return ret;
}

std::optional<unsigned long> to_ulong(std::string const& str)
{
    char* end;
    errno = 0;
    auto ret = strtoul(str.c_str(), &end, 0);
    if ((end != (str.c_str() + str.length())) || (errno != 0))
        return {};
    if ((ret == 0) && !check_zero(str))
        return {};
    return ret;
}

long to_long_unconditional(std::string const& str)
{
    auto ret = to_long(str);
    assert(ret.has_value());
    return ret.value();
}

std::optional<double> to_double(std::string const& str)
{
    char* end;
    errno = 0;
    auto ret = strtod(str.c_str(), &end);
    if ((end != (str.c_str() + str.length())) || (errno != 0))
        return {};
    if (ret == 0) {
        for (auto& ch : str) {
            if ((ch != '.') && (ch != '0'))
                return {};
        }
    }
    return ret;
}

double to_double_unconditional(std::string const& str)
{
    auto ret = to_double(str);
    assert(ret.has_value());
    return ret.value();
}

std::optional<bool> to_bool(std::string const& str)
{
    if (str == "true" || str == "True" || str == "TRUE")
        return true;
    if (str == "false" || str == "False" || str == "FALSE")
        return true;
    auto ret = to_long(str);
    if (!ret.has_value())
        return {};
    return (ret.value() != 0);
}

bool to_bool_unconditional(std::string const& str)
{
    auto ret = to_bool(str);
    assert(ret.has_value());
    return ret.value();
}

}