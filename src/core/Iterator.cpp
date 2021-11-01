/*
 * Iterator.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/Iterator.h>

namespace Obelix {

SimpleIteratorState::SimpleIteratorState(Object& container, IteratorWhere where)
    : IteratorState(container)
    , m_index((where == IteratorWhere::Begin) ? 0 : container.size())
{
}

SimpleIteratorState::SimpleIteratorState(SimpleIteratorState const& original, size_t index)
    : IteratorState(dynamic_cast<IteratorState const&>(original))
    , m_index(index)
{
}

void SimpleIteratorState::increment(ptrdiff_t delta)
{
    m_index += delta;
    m_current = make_null<Object>();
}

IteratorState* SimpleIteratorState::copy() const
{
    return new SimpleIteratorState(*this, m_index);
}

bool SimpleIteratorState::equals(IteratorState const* other) const
{
    auto other_casted = dynamic_cast<SimpleIteratorState const*>(other);
    return m_index == other_casted->m_index;
}

Ptr<Object> const& SimpleIteratorState::dereference()
{
    if (!m_current.has_nullptr())
        return m_current;
    if ((m_index >= 0) && (m_index < container().size()))
        return container().at(m_index);
    return m_current;
}

// --------------------------------------------------------------------------

ObjectIterator::ObjectIterator(ObjectIterator& other)
    : m_state(other.m_state->copy())
{
}

ObjectIterator::ObjectIterator(IteratorState* state)
    : m_state(state)
{
}

ObjectIterator ObjectIterator::begin(Object& container)
{
    return ObjectIterator(container.iterator_state(IteratorState::IteratorWhere::Begin));
}

ObjectIterator ObjectIterator::end(Object& container)
{
    return ObjectIterator(container.iterator_state(IteratorState::IteratorWhere::End));
}

bool ObjectIterator::operator==(ObjectIterator const& other) const
{
    return m_state->equals(other.m_state);
}

bool ObjectIterator::operator!=(ObjectIterator const& other) const
{
    return !m_state->equals(other.m_state);
}

ObjectIterator& ObjectIterator::operator++()
{
    m_state->increment(1);
    return *this;
}

ObjectIterator ObjectIterator::operator++(int)
{
    ObjectIterator tmp(*this);
    m_state->increment(1);
    return tmp;
}

Obj const& ObjectIterator::operator*()
{
    return const_cast<Ptr<Object> const&>(m_state->dereference());
}

}
