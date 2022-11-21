/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <unordered_map>

#include <config.h>
#include <core/FileBuffer.h>
#include <obelix/Architecture.h>

namespace Obelix {

struct Config {
public:
    Config() = default;
    Config(int argc, char const** argv);

    std::string filename { "" };
    bool help { false };
    bool show_tree { false };
    bool import_root { true };
    bool lex { true };
    bool bind { true };
    bool lower { true };
    bool fold_constants { true };
    bool materialize { true };
    bool compile { true };
    bool run { false };

    template <typename T>
    T cmdline_flag(std::string const& flag, T const& default_value = T()) const
    {
        if (m_cmdline_flags.contains(flag)) {
            auto val = m_cmdline_flags.at(flag);
            assert(std::holds_alternative<T>(val));
            return get<T>(val);
        }
        return default_value;
    }
    Architecture target { Architecture::C_TRANSPILER };

    [[nodiscard]] std::string obelix_directory() const;
    [[nodiscard]] std::string base_directory() const;
    [[nodiscard]] std::string main() const;

private:
    std::string obelix_dir {};
    std::unordered_map<std::string, std::variant<std::string,bool>> m_cmdline_flags;
};

class ObelixBufferLocator : public BufferLocator {
public:
    ObelixBufferLocator(Config const& config)
        : m_config(config)
    {
    }

    [[nodiscard]] ErrorOr<std::string> locate(std::string const&) const override;

private:
    static ErrorOr<std::string> check_in_dir(std::string const&, std::string const&);
    Config const& m_config;
};

}
