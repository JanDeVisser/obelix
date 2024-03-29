/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Iterator.h>

namespace Obelix {

SimpleIterator::SimpleIterator(Object const& container, size_t index)
    : Object(TypeIterator)
    , m_container(container)
    , m_index(index)
{
}

Ptr<Object> SimpleIterator::copy() const
{
    return make_obj<SimpleIterator>(m_container, m_index);
}

std::optional<Ptr<Object>> SimpleIterator::next()
{
    if ((m_index >= 0) && (m_index < m_container.size())) {
        return m_container.at(m_index++);
    }
    return {};
}

// --------------------------------------------------------------------------

ObjectIterator::ObjectIterator(ObjectIterator& other)
    : m_state(other.m_state->copy().pointer())
{
}

ObjectIterator::ObjectIterator(Obj const& state)
    : m_state(state.pointer())
{
    if (m_state)
        dereference();
}

void ObjectIterator::dereference()
{
    auto next = m_state->next();
    if (next.has_value())
        m_current = next->pointer();
    else
        m_current = nullptr;
}

ObjectIterator ObjectIterator::begin(Object const& container)
{
    auto iter_maybe = container.iterator();
    assert(iter_maybe.has_value());
    return ObjectIterator(iter_maybe.value());
}

ObjectIterator ObjectIterator::end(Object const& container)
{
    return ObjectIterator(make_null<Object>());
}

bool ObjectIterator::operator==(ObjectIterator const& other) const
{
    if ((m_current == nullptr) != (other.m_current == nullptr))
        return false;
    if (m_current == nullptr)
        return true;
    return m_current->compare(*other.m_current) == 0;
}

bool ObjectIterator::operator!=(ObjectIterator const& other) const
{
    return !operator==(other);
}

ObjectIterator& ObjectIterator::operator++()
{
    dereference();
    return *this;
}

ObjectIterator ObjectIterator::operator++(int)
{
    ObjectIterator tmp(*this);
    dereference();
    return tmp;
}

Obj ObjectIterator::operator*()
{
    assert(m_current != nullptr);
    return make_from_shared<Object>(m_current);
}

}
