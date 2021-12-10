/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/NativeFunction.h>
#include <core/StringUtil.h>

namespace Obelix {

logging_category(function);

Resolver& NativeFunction::s_resolver = Resolver::get_resolver();

NativeFunction::NativeFunction(std::string name, void_t fnc, std::vector<std::string> params)
    : Object(TypeNativeFunction)
    , m_name(move(name))
    , m_fnc(fnc)
    , m_parameters(move(params))
{
}

NativeFunction::NativeFunction(std::string name, std::vector<std::string> params)
    : Object(TypeNativeFunction)
    , m_name(move(name))
    , m_parameters(move(params))
{
    resolve_function();
}

std::optional<NativeFunction> NativeFunction::parse(std::string const& str) {
    auto name_params = split(str, '(');
    if (name_params.empty() || name_params.size() > 2)
        return {};

    std::vector<std::string> params;
    if (name_params.size() == 2) {
        auto p = name_params.back();
        if (p.back() != ')')
            return {};
        p = p.substr(0, p.length()-1);
        params = split(p, ',');
    }
    NativeFunction ret(name_params.front(), params);
    return ret;
}

int NativeFunction::compare(Obj const& other) const
{
    oassert(other->type() == TypeNativeFunction, "Can't compare NativeFunctions to objects of type_name '%s'", other->type_name());
    auto other_func = ptr_cast<NativeFunction>(other);
    return m_name.compare(other_func->m_name);
}

bool NativeFunction::resolve_function()
{
    if (m_fnc)
        return true;

    if (m_name.empty())
        return false;

    debug(function, "Resolving {}", m_name);
    if (auto result = s_resolver.resolve(m_name); result.errorcode) {
        return false;
    } else {
        m_fnc = result.function;
        return true;
    }
}

Obj NativeFunction::call(Ptr<Arguments> args) {
    return call(name(), std::move(args));
}

Obj NativeFunction::call(std::string const& name, Ptr<Arguments> args) {
    if (!m_fnc) {
        if (!resolve_function()) {
            return make_obj<Exception>(ErrorCode::FunctionUndefined, m_name, image_name());
        }
    }
    Obj ret;
    ((native_t) m_fnc)(name.c_str(), &args, &ret);
    return ret;
}

std::string NativeFunction::image_name() const
{
    auto n = split(m_name, ':');
    if (n.size() > 1) {
        return n.back();
    } else {
        return {};
    }
}

}
