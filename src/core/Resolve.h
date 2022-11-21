/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
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
        ResolveResult m_my_result;
        std::unordered_map<std::string, ResolveResult> m_functions {};
        friend Resolver;
    };

    Resolver() = default;
    std::unordered_map<std::string, std::shared_ptr<Library>> m_images;
};

}
