/*
 * Range.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

namespace Obelix {

class Range : public Object {
public:
    Range(Obj const&, Obj const&);

    [[nodiscard]] IteratorState* iterator_state(IteratorState::IteratorWhere where) override;
    [[nodiscard]] std::string to_string() const override;
    std::optional<Obj> evaluate(std::string const& name, Ptr<Arguments>) override;
    [[nodiscard]] std::optional<Obj> resolve(std::string const& name) const override;

private:
    Obj m_low;
    Obj m_high;
};

}
