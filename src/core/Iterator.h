/*
 * Iterator.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

class SimpleIteratorState : public IteratorState {
public:
    explicit SimpleIteratorState(Object&, IteratorWhere = IteratorWhere::Begin);
    ~SimpleIteratorState() override = default;

    void increment(ptrdiff_t delta) override;
    Ptr<Object> const& dereference() override;
    [[nodiscard]] IteratorState* copy() const override;
    [[nodiscard]] bool equals(IteratorState const* other) const override;

private:
    SimpleIteratorState(SimpleIteratorState const& original) = default;
    SimpleIteratorState(SimpleIteratorState const& original, size_t index);

    size_t m_index { 0 };
    Ptr<Object> m_current {};
};

}
