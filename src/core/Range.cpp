/*
 * Range.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/Logging.h>
#include <core/Range.h>

namespace Obelix {

logging_category(range);

class RangeIteratorState : public IteratorState {
public:
    RangeIteratorState(Object& container, Obj from, Obj to, IteratorWhere where)
        : IteratorState(container)
        , m_from(from)
        , m_to(to)
        , m_current_value((where == IteratorWhere::Begin) ? from->to_long().value() : to->to_long().value() + 1)
    {
    }

    ~RangeIteratorState() override = default;

    void increment(ptrdiff_t delta) override
    {
        assert(delta >= 0);
        m_current_value += delta;
        m_current = {};
    }

    Ptr<Object> const& dereference() override
    {
        if (!m_current.has_nullptr())
            return m_current;
        if (m_current_value <= m_to->to_long().value()) {
            m_current = make_obj<Integer>(m_current_value);
        }
        return m_current;
    }

    [[nodiscard]] IteratorState* copy() const override
    {
        return new RangeIteratorState(*this);
    }

    [[nodiscard]] bool equals(IteratorState const* other) const override
    {
        auto other_casted = dynamic_cast<RangeIteratorState const*>(other);
        return m_current_value == other_casted->m_current_value;
    }

private:
    RangeIteratorState(RangeIteratorState const& original) = default;
    Obj m_from;
    Obj m_to;
    long m_current_value;
    Ptr<Object> m_current {};
};

Range::Range(Obj const& low, Obj const& high)
    : Object("range")
    , m_low(low)
    , m_high(high)
{
    assert(!m_low.has_nullptr());
    assert(!m_high.has_nullptr());
    debug(range, "Creating range {} .. {}", low.type(), high.type());
    assert(low->type() == high->type());
}

std::string Range::to_string() const
{
    return format("{}..{}", m_low->to_string(), m_high->to_string());
}

IteratorState* Range::iterator_state(IteratorState::IteratorWhere where)
{
    return new RangeIteratorState(*this, m_low, m_high, where);
}

std::optional<Obj> Range::evaluate(std::string const& name, Ptr<Arguments> args)
{
    return Object::evaluate(name, args);
}

[[nodiscard]] std::optional<Obj> Range::resolve(std::string const& name) const
{
    if (name == "high") {
        return m_high;
    } else if (name == "low") {
        return m_low;
    } else {
        return Object::resolve(name);
    }
}

}
