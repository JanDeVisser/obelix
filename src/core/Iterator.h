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

class SimpleIterator : public Object {
public:
    explicit SimpleIterator(Object const&, size_t = 0);
    ~SimpleIterator() override = default;

    [[nodiscard]] Ptr<Object> copy() const override;
    std::optional<Ptr<Object>> next() override;

private:
    Object const& m_container;
    size_t m_index { 0 };
};

}
