/*
 * Functions.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/Arguments.h>
#include <core/Object.h>

namespace Obelix {

std::string format_arguments(Ptr<Arguments> args)
{
    std::string fmt;
    std::vector<Obelix::Obj> format_args;
    for (auto& a : args->arguments()) {
        if (fmt.empty()) {
            fmt = a->to_string();
        } else {
            format_args.push_back(a);
        }
    }
    return format(fmt, format_args);
}

extern "C" void oblfunc_print(const char* name, Obelix::Ptr<Obelix::Arguments>* args, Obj* ret)
{
    Ptr<Arguments> arguments = *args;
    printf("%s\n", format_arguments(arguments).c_str());
    *ret = Object::null();
}

extern "C" void oblfunc_format(const char* name, Obelix::Ptr<Obelix::Arguments>* args, Obj* ret)
{
    Ptr<Arguments> arguments = *args;
    *ret = make_obj<String>(format_arguments(arguments).c_str());
}

}
