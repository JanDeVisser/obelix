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

#include <string>
#include <unordered_map>
#include <vector>

#define HAVE_DLFCN_H

namespace Obelix {

constexpr static char const* OBL_DIR = "OBL_DIR";
constexpr static char const* OBL_INIT = "_obl_init";

typedef void (*void_t)();

typedef void* lib_handle_t;
typedef int resolve_error_t;

class Resolver {
public:
    static Resolver& get_resolver();
    bool open(std::string const&);
    void_t resolve(std::string const&);

private:
    struct ResolveResult {
        explicit ResolveResult(void* res);

        void* result;
        int errorcode { 0 };
        std::string error {};
    };

    class ResolveHandle {
    public:
        explicit ResolveHandle(std::string img);
        ~ResolveHandle();
        std::string to_string();
        std::string platform_image();
        bool open();
        ResolveResult get_function(std::string const&);
    private:
        void try_open(std::string const&);

        lib_handle_t m_handle { nullptr };
        std::string m_image;
        std::string m_platform_image { nullptr };
    };

    Resolver() = default;
    std::vector<ResolveHandle> m_images;
    std::unordered_map<std::string, void_t> m_functions;
};

}