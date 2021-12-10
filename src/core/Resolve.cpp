/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <dlfcn.h>
#include <mutex>

#include <config.h>
#include <core/Logging.h>
#include <core/Resolve.h>
#include <core/StringUtil.h>

namespace Obelix {

logging_category(resolve);
std::mutex g_resolve_mutex;

/* ------------------------------------------------------------------------ */

ResolveResult::ResolveResult(void* res, std::optional<std::string> msg)
    : result(res)
{
    if (!res) {
        if (msg.has_value()) {
            message = msg.value();
            errorcode = -1;
            return;
        }
        std::string err = dlerror();
        debug(resolve, "dlerror(): {}", err);

        // 'undefined symbol' is returned with an empty result pointer
#ifdef __APPLE__
        if (err.find("symbol not found") == std::string::npos) {
#else
        if (err.find("undefined symbol") == std::string::npos) {
#endif
            message = err;
            errorcode = -1;
        } else {
            message = "";
            errorcode = 0;
        }
    }
    if (!message.empty()) {
        debug(resolve, "resolve_result has error '{s}' ({d})", message, errorcode);
    } else {
        debug(resolve, "resolve_result OK, result is {s}NULL", (result) ? "NOT " : "");
    }
}

Resolver::Library::Library(std::string img)
    : m_image(move(img))
{
    open();
}

Resolver::Library::~Library()
{
    if (m_handle)
        dlclose(m_handle);
}

std::string Resolver::Library::to_string()
{
    return (!m_image.empty()) ? m_image : "Main Program Image";
}

std::string Resolver::Library::platform_image(std::string const& image)
{
    if (image.empty()) {
        return "";
    }
    auto platform_image = image;
    for (auto backslash = platform_image.find_first_of('\\');
         backslash != std::string::npos;
         backslash = platform_image.find_first_of('\\')) {
        platform_image[backslash] = '/';
    }
    auto dot = platform_image.find_last_of('.');
    if (dot != std::string::npos) {
        platform_image = platform_image.substr(0, dot + 1);
    } else {
        platform_image += '.';
    }
#ifdef __APPLE__
    platform_image += "dylib";
#else
    platform_image += "so";
#endif
    return platform_image;
}

ResolveResult Resolver::Library::try_open(std::string const& dir)
{
    auto image = platform_image(m_image);
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
    if (libhandle) {
        debug(resolve, "Successfully opened '{s}'", (!image.empty()) ? path : "main program module");
    }
    return ResolveResult((void*)libhandle);
}

ResolveResult Resolver::Library::open()
{
    auto image = platform_image(m_image);
    if (!image.empty()) {
        debug(resolve, "resolve_open('{s}') ~ '{s}'", m_image, image);
    } else {
        debug(resolve, "resolve_open('Main Program Image')");
    }
    ResolveResult ret;
    m_handle = nullptr;
    if (!image.empty()) {
        std::string obldir = getenv("OBL_DIR") ? getenv("OBL_DIR") : "";
        if (obldir.empty()) {
            obldir = OBELIX_DIR;
        }
        ret = try_open(obldir + "/lib");
        if (ret.errorcode) {
            ret = try_open(obldir + "/bin");
        }
        if (ret.errorcode) {
            ret = try_open(obldir);
        }
        if (ret.errorcode) {
            ret = try_open(obldir + "/share/lib");
        }
        if (ret.errorcode) {
            ret = try_open("./lib");
        }
        if (ret.errorcode) {
            ret = try_open("./bin");
        }
        if (ret.errorcode) {
            ret = try_open("./share/lib");
        }
        if (ret.errorcode) {
            ret = try_open("./");
        }
    } else {
        ret = try_open("");
    }
    if (!ret.errorcode) {
        m_handle = ret.handle;
        if (!image.empty()) {
            auto result = get_function(OBL_INIT);
            if (result.result) {
                debug(resolve, "resolve_open('{s}') Executing initializer", to_string());
                (result.function)();
            } else if (!result.errorcode) {
                debug(resolve, "resolve_open('{s}') No initializer", to_string());
            } else {
                log_error("resolve_open('{s}') Error finding initializer: {s} ({d})",
                    to_string(), result.message, result.errorcode);
                m_my_result = result;
                return result;
            }
        }
        debug(resolve, "Library '{s}' opened successfully", to_string());
        return ResolveResult(m_handle);
    } else {
        log_error("Resolver::Library::open('{s}') FAILED", to_string());
        m_my_result = ret;
        return ret;
    }
}

ResolveResult Resolver::Library::get_function(std::string const& function_name)
{
    if (m_my_result.errorcode)
        return m_my_result;
    dlerror();
    if (m_functions.contains(function_name))
        return m_functions[function_name];
    debug(resolve, "dlsym('{s}', '{s}')", to_string(), function_name);
    auto function = ResolveResult(dlsym(m_handle, function_name.c_str()));
    m_functions[function_name] = function;
    return function;
}

/* ------------------------------------------------------------------------ */

ResolveResult Resolver::open(std::string const& image)
{
    auto platform_image = Library::platform_image(image);
    if (!m_images.contains(platform_image)) {
        auto lib = std::make_shared<Library>(image);
        if (lib->is_valid()) {
            m_images[platform_image] = lib;
        } else {
            return lib->result();
        }
    }
    return m_images[platform_image]->result();
}

ResolveResult Resolver::resolve(std::string const& func_name)
{
    const std::lock_guard<std::mutex> lock(g_resolve_mutex);
    auto s = func_name;
    if (auto paren = s.find_first_of('('); paren != std::string::npos) {
        s.erase(paren);
        while (s[s.length() - 1] == ' ')
            s.erase(s.length() - 1);
    }
    if (auto space = s.find_first_of(' '); space != std::string::npos) {
        s.erase(0, space);
        while (s[0] == ' ')
            s.erase(0, 1);
    }

    std::string image;
    std::string function;
    auto name = split(s, ':');
    switch (name.size()) {
    case 2:
        image = name.front();
        /* fall through */
    case 1:
        function = name.back();
        break;
    default:
        return ResolveResult(nullptr, format("Invalid function reference '{}'", func_name));
    }

    if (auto result = open(image); result.errorcode) {
        return result;
    } else {
        auto lib = m_images[Library::platform_image(image)];
        return lib->get_function(function);
    }
}

Resolver& Resolver::get_resolver() noexcept
{
    static Resolver* resolver = nullptr;
    const std::lock_guard<std::mutex> lock(g_resolve_mutex);
    if (!resolver) {
        resolver = new Resolver();
    }
    return *resolver;
}

}
