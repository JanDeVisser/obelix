/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <core/Resolve.h>

namespace Obelix {

typedef void (*native_t)(char const*, Ptr<Arguments> *, Obj *);

class NativeFunction : public Object {
public:
    NativeFunction(std::string name, void_t, std::vector<std::string> = {});
    explicit NativeFunction(std::string name, std::vector<std::string> = {});
    bool resolve_function();
    Obj call(Ptr<Arguments>) override;
    Obj call(std::string const&, Ptr<Arguments>);
    static std::optional<NativeFunction> parse(std::string const&);
    [[nodiscard]] int compare(Obj const&) const override;
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] std::string image_name() const;

private:
    static Resolver& s_resolver;
    std::string m_name;
    void_t m_fnc { nullptr };
    //    [[maybe_unused]] int m_min_params { 0 };
    //    [[maybe_unused]] int m_max_params { 0 };
    //    [[maybe_unused]] std::string m_return_type { "integer" };
    std::vector<std::string> m_parameters {};
};

}
