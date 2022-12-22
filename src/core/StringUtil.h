/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-09-20.
//

#pragma once

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Obelix {

int stricmp(const char *, const char *);
std::string to_upper(std::string const&);
std::string to_lower(std::string const&);
std::size_t replace_all(std::string&, std::string_view, std::string_view);

std::string c_escape(std::string const& s);
std::vector<std::string> split(std::string const& s, char sep);
std::string join(std::vector<std::string> const& collection, std::string const& sep);
std::string join(std::vector<std::string> const& collection, char sep);
std::string strip(std::string const& s);
std::string rstrip(std::string const& s);
std::string lstrip(std::string const& s);
std::vector<std::pair<std::string, std::string>> parse_pairs(std::string const& s, char pair_sep = ';', char name_value_sep = '=');
std::string to_string(double);
std::string to_string(bool);
std::optional<long> to_long(std::string const&);
std::optional<unsigned long> to_ulong(std::string const&);
std::optional<double> to_double(std::string const&);
std::optional<bool> to_bool(std::string const&);
long to_long_unconditional(std::string const&);
double to_double_unconditional(std::string const&);
bool to_bool_unconditional(std::string const&);

template<typename ElementType, typename ToString>
std::string join(std::vector<ElementType> const& collection, std::string const& sep, ToString const& tostring)
{
    std::string ret;
    auto first = true;
    for (auto& elem : collection) {
        if (!first) {
            ret += sep;
        }
        ret += tostring(elem);
        first = false;
    }
    return ret;
}

template<typename ElementType, typename ToString>
std::string join(std::vector<ElementType> const& collection, char sep, ToString const& tostring)
{
    std::string sep_str;
    sep_str += sep;
    return join(collection, sep_str, tostring);
}

template<std::signed_integral T>
inline std::string to_string(T value)
{
    char buf[80];
    snprintf(buf, 79, "%ld", static_cast<long>(value));
    return buf;

}

template<std::unsigned_integral T>
inline std::string to_string(T value)
{
    char buf[80];
    snprintf(buf, 79, "%lu", static_cast<unsigned long>(value));
    return buf;

}

template<std::unsigned_integral T>
inline std::string to_hex_string(T value)
{
    char buf[80];
    snprintf(buf, 79, "%lx", static_cast<unsigned long>(value));
    return buf;

}

}
