/*
 * NativeFunction.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#pragma once

#include <core/Object.h>
#include <core/Resolve.h>

namespace Obelix {

typedef Obj * (*native_t)(char const*, Ptr<Arguments> *);

class NativeFunction : public Object {
public:
    NativeFunction(std::string name, void_t, std::vector<std::string> = {});
    explicit NativeFunction(std::string name, std::vector<std::string> = {});
    bool resolve();
    std::optional<Obj> call(Ptr<Arguments>) override;
    std::optional<Obj> call(std::string const&, Ptr<Arguments>);
    static std::optional<NativeFunction> parse(std::string const&);
    [[nodiscard]] int compare(Obj const&) const override;
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] std::string image_name() const;

private:
    std::string m_name;
    void_t m_fnc { nullptr };
    int m_min_params { 0 };
    int m_max_params { 0 };
    std::string m_return_type { "integer" };
    std::vector<std::string> m_parameters {};
};

}
