/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

//
// Created by Jan de Visser on 2021-09-24.
//

#include <core/Arguments.h>
#include <core/List.h>

namespace Obelix {

std::string List::to_string() const
{
    std::vector<std::string> strings;
    for (auto& obj : m_list) {
        strings.push_back(obj->to_string());
    }
    auto str = join(strings, ", ");
    return "[ " + str + " ]";
}

std::optional<Obj> List::evaluate(std::string const& name, Ptr<Arguments> args)
{
    if ((name == "add") || (name == "+=")) {
        for (auto const& arg : *args) {
            m_list.push_back(arg);
        }
        return Obj::True();
    } else if (name == "+") {
        std::shared_ptr<List> aggregate = std::make_shared<List>();
        for (auto const& obj : *this) {
            aggregate->push_back(obj);
        }
        for (Ptr<Object> arg : *args) {
            for (auto const& obj : *arg) {
                aggregate->push_back(obj);
            }
        }
    }
    return {};
}

std::optional<Obj> List::resolve(std::string const& name) const
{
    auto num_maybe = Obelix::to_ulong(name);
    if (num_maybe.has_value()) {
        auto num = num_maybe.value();
        if ((num >= 0) && (num < m_list.size()))
            return ptr_cast<Object>(m_list[num]);
    }
    return {};
}

}
