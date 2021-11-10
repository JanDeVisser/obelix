/*
 * Exception.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/Object.h>

namespace Obelix {

std::string ErrorCode_name(ErrorCode code)
{
    switch (code) {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) \
    case ErrorCode::code:                 \
        return #code;
        ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
    default:
        fatal("Unreachable");
    }
}

std::string ErrorCode_message(ErrorCode code)
{
    switch (code) {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) \
    case ErrorCode::code:                 \
        return msg;
        ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
    default:
        fatal("Unreachable");
    }
}

std::optional<Obj> Exception::evaluate(std::string const&, Ptr<Arguments>)
{
    return self();
}

std::optional<Obj> Exception::resolve(std::string const& name) const
{
    return self();
}

std::optional<Obj> Exception::assign(std::string const&, std::string const&, Obj const&)
{
    return self();
}

}
