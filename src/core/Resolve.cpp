/*
* /obelix/src/core/Resolve.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <cstdio>
#include <climits>
#include <dlfcn.h>
#include <mutex>
#include <cstring>

#include <core/Logging.h>
#include <core/Resolve.h>

namespace Obelix {

logging_category(resolve);
std::mutex g_resolve_mutex;

/* ------------------------------------------------------------------------ */

Resolver::ResolveResult::ResolveResult(void* res)
{
    if (!res) {
        std::string err = dlerror();
        debug(resolve, "dlerror(): {}", err);
        if (!err.empty() && err.find_first_of("undefined symbol")) {
            error = err;
            errorcode = -1;
        } else {
            error = "";
        }
    }
    result = (errorcode) ? nullptr : res;
    if (!error.empty()) {
        debug(resolve, "resolve_result has error '{s}' ({d})", error, errorcode);
    } else {
        debug(resolve, "resolve_result OK, result is {s}NULL", (result) ? "NOT " : "");
    }
}


Resolver::ResolveHandle::ResolveHandle(std::string img)
    : m_image(move(img))
{
    open();
}

Resolver::ResolveHandle::~ResolveHandle()
{
    dlclose(m_handle);
}

std::string Resolver::ResolveHandle::to_string()
{
    return (!m_image.empty()) ? m_image : "Main Program Image";
}

std::string Resolver::ResolveHandle::platform_image()
{
    if (m_image.empty()) {
        return "";
    }
    if (m_platform_image.empty()) {
        std::string canonical;
        auto len = m_image.length();
        for (auto ix = 0; ix < len; ix++) {
            m_platform_image[ix] = m_image[ix];
            if (m_image[ix] == '\\') {
                m_platform_image += '/';
            } else {
                m_platform_image += m_image[ix];
            }
        }
        auto dot = m_platform_image.find_last_of('.');
        if (dot >= 0) {
            m_platform_image = m_platform_image.substr(0, dot + 1);
        } else {
            m_platform_image += '.';
        }
#ifdef __APPLE__
        m_platform_image += "dylib";
#else
        m_platform_image += "so";
#endif
    }
    return m_platform_image;
}

void Resolver::ResolveHandle::try_open(std::string const& dir)
{
    auto image = platform_image();
    std::string path;
    if (!image.empty()) {
        if (!dir.empty()) {
            path = dir + "/" + image;
        } else {
            path = image;
        }
        debug(resolve, "Attempting to open library '{s}'", path);
    } else {
        debug(resolve, "Attempting to open main program module");
    }
    dlerror();
    auto libhandle = dlopen((!image.empty()) ? path.c_str() : nullptr, RTLD_NOW | RTLD_GLOBAL);
    ResolveResult res((void*)libhandle);
    m_handle = (lib_handle_t) res.result;
    if (m_handle) {
        debug(resolve, "Successfully opened '{s}'", (!image.empty()) ? path : "main program module");
    }
}

bool Resolver::ResolveHandle::open()
{
    auto image = platform_image();
    if (!image.empty()) {
        debug(resolve, "resolve_open('{s}') ~ '{s}'", m_image, image);
    } else {
        debug(resolve, "resolve_open('Main Program Image')");
    }
    m_handle = nullptr;
    if (!image.empty()) {
        std::string obldir = getenv("OBL_DIR");
        if (obldir.empty()) {
            obldir = "/Users/jan/projects/obelix2/cmake-build-debug"; //OBELIX_DIR;
        }
        try_open(obldir + "/lib");
        if (!m_handle) {
            try_open(obldir + "/bin");
        }
        if (!m_handle) {
            try_open(obldir);
        }
        if (!m_handle) {
            try_open(obldir + "/share/lib");
        }
        if (!m_handle) {
            try_open("lib");
        }
        if (!m_handle) {
            try_open("bin");
        }
        if (!m_handle) {
            try_open("share/lib");
        }
    } else {
        try_open("");
    }
    if (m_handle) {
        if (!image.empty()) {
            auto result = get_function(OBL_INIT);
            if (result.result) {
                debug(resolve, "resolve_open('{s}') Executing initializer", to_string());
                ((void_t)result.result)();
            } else if (!result.errorcode) {
                debug(resolve, "resolve_open('{s}') No initializer", to_string());
            } else {
                error("resolve_open('{s}') Error finding initializer: {s} ({d})",
                    to_string(), result.error, result.errorcode);
                return false;
            }
        }
        debug(resolve, "Library '{s}' opened successfully", to_string());
        return true;
    } else {
        error("resolve_open('{s}') FAILED", to_string());
        return false;
    }
}

Resolver::ResolveResult Resolver::ResolveHandle::get_function(std::string const& function_name)
{
   void_t function;

    debug(resolve, "dlsym('{s}', '{s}')", to_string(), function_name);
    dlerror();
    function = (void_t) dlsym(m_handle, function_name.c_str());
    return Resolver::ResolveResult((void*) function);
}

/* ------------------------------------------------------------------------ */

bool Resolver::open(std::string const& image)
{
    const std::lock_guard<std::mutex> lock(g_resolve_mutex);
    m_images.emplace_back(image);
    return true;
}

void_t Resolver::resolve(std::string const& func_name)
{
    const std::lock_guard<std::mutex> lock(g_resolve_mutex);
    auto s = func_name;
    auto paren = s.find_first_of('(');
    if (paren > 0) {
        s = s.substr(0, paren);
    }

    if (m_functions.contains(s)) {
        auto ret = m_functions[s];
        debug(resolve, "Function '{}' was cached", func_name);
        return ret;
    }

    debug(resolve, "dlsym('{}')", func_name);
    void_t ret;
    for (auto& img : m_images) {
        auto result = img.get_function(s);
        if (result.errorcode) {
            error("Error resolving function '{}' in library '{}': {} ({})",
                func_name, img.to_string(), result.error, result.errorcode);
            continue;
        }
        ret = (void_t) result.result;
        if (ret) {
            m_functions.insert_or_assign(s, ret);
            return ret;
        } else {
            error("Error resolving function '{}' in library '{}': got nullptr)", func_name, img.to_string());
        }
    }
    return nullptr;
}

Resolver::Resolver()
{
}

Resolver& Resolver::get_resolver()
{
    static Resolver *resolver = nullptr;
    const std::lock_guard<std::mutex> lock(g_resolve_mutex);
    if (!resolver)
        resolver = new Resolver();
    return *resolver;
}

}
