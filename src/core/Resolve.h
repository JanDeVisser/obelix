/*
 * /obelix/include/resolve.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Obelix {

constexpr static char const* OBL_DIR = "OBL_DIR";
constexpr static char const* OBL_INIT = "_obl_init";

typedef void (*void_t)();

typedef void* lib_handle_t;

struct ResolveResult {
    ResolveResult() = default;
    explicit ResolveResult(void* res, std::optional<std::string> message = {});

    union {
        void* result { nullptr };
        lib_handle_t handle;
        void_t function;
    };

    int errorcode { 0 };
    std::string message {};
};

class Resolver {
public:
    ~Resolver() = default;
    static Resolver& get_resolver() noexcept;
    ResolveResult open(std::string const&);
    ResolveResult resolve(std::string const&);

private:
    class Library {
    public:
        explicit Library(std::string img);
        ~Library();
        std::string to_string();
        static std::string platform_image(std::string const&);
        [[nodiscard]] bool is_valid() const { return m_my_result.errorcode == 0; }
        [[nodiscard]] ResolveResult const& result() const { return m_my_result; }
        ResolveResult get_function(std::string const&);

    private:
        ResolveResult open();
        ResolveResult try_open(std::string const&);

        lib_handle_t m_handle { nullptr };
        std::string m_image {};
        std::string m_platform_image {};
        ResolveResult m_my_result;
        std::unordered_map<std::string, ResolveResult> m_functions {};
        friend Resolver;
    };

    Resolver() = default;
    std::unordered_map<std::string, std::shared_ptr<Library>> m_images;
};

}