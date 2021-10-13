/*
 * NativeFunction.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/NativeFunction.h>
#include <core/StringUtil.h>

namespace Obelix {

logging_category(function);

NativeFunction::NativeFunction(std::string name, void_t fnc, std::vector<std::string> params)
    : Object("native")
    , m_name(move(name))
    , m_fnc(fnc)
    , m_parameters(move(params))
{
}

NativeFunction::NativeFunction(std::string name, std::vector<std::string> params)
    : Object("native")
    , m_name(move(name))
    , m_parameters(move(params))
{
    resolve();
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
    oassert(other->type() == "native", "Can't compare NativeFunctions to objects of type '%s'", other->type().c_str());
    auto other_func = ptr_cast<NativeFunction>(other);
    return m_name.compare(other_func->m_name);
}

bool NativeFunction::resolve()
{
    if (m_fnc)
        return true;

    if (m_name.empty())
        return false;

    debug(function, "Resolving %s", m_name.c_str());
    auto name = split(m_name, ':');
    switch (name.size()) {
    case 2:
        if (!Resolver::get_resolver().open(name.front())) {
            error("Error loading library '%s'", name.front().c_str());
            return false;
        }
        /* fall through */
    case 1:
        m_fnc = (void_t) Resolver::get_resolver().resolve(name.back());
        if (!m_fnc) {
            error("Error resolving function '%s'", m_name.c_str());
            return false;
        }
        return true;
    default:
        error("Invalid function reference '%s'", m_name.c_str());
        return false;
    }
}

std::optional<Obj> NativeFunction::call(Ptr<Arguments> args) {
    return call(name(), std::move(args));
}

std::optional<Obj> NativeFunction::call(std::string const& name, Ptr<Arguments> args) {
    if (!m_fnc) {
        if (!resolve()) {
            return make_obj<Exception>(ErrorCode::FunctionUndefined, m_name, image_name());
        }
    }
    return *((native_t) m_fnc)(name.c_str(), &args);
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
