/*
 * Type.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/Object.h>

namespace Obelix {

#define ENUMERATE_OBELIX_TYPES(S) \
    S(Unknown)                    \
    S(Null)                       \
    S(Int)                        \
    S(Unsigned)                   \
    S(Float)                      \
    S(String)                     \
    S(Object)                     \
    S(List)                       \
    S(Regex)

enum class ObelixType {
#undef __ENUM_OBELIX_TYPE
#define __ENUM_OBELIX_TYPE(t) t,
    ENUMERATE_OBELIX_TYPES(__ENUM_OBELIX_TYPE)
#undef __ENUM_OBELIX_TYPE
};

constexpr const char* ObelixType_name(ObelixType t)
{
    switch (t) {
#undef __ENUM_OBELIX_TYPE
#define __ENUM_OBELIX_TYPE(t) \
    case ObelixType::t:       \
        return #t;
        ENUMERATE_OBELIX_TYPES(__ENUM_OBELIX_TYPE)
#undef __ENUM_OBELIX_TYPE
    }
}

ObelixType ObelixType_of(Obj const&);

}