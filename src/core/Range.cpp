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

class RangeIterator : public Object {
public:
    RangeIterator(Obj from, Obj to)
        : Object("rangeiterator")
        , m_from(std::move(from))
        , m_to(std::move(to))
        , m_current_value(m_from->to_long().value())
    {
    }

    ~RangeIterator() override = default;

    std::optional<Ptr<Object>> next() override
    {
        if (m_current_value <= m_to->to_long().value())
            return make_obj<Integer>(m_current_value++);
        return {};
    }

    [[nodiscard]] Ptr<Object> copy() const override
    {
        Ptr<RangeIterator> ret = make_typed<RangeIterator>(m_from, m_to);
        ret->m_current_value = m_current_value;
        return to_obj(ret);
    }

private:
    Obj m_from;
    Obj m_to;
    long m_current_value;
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

std::optional<Ptr<Object>> Range::iterator() const
{
    return make_obj<RangeIterator>(m_low, m_high);
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
