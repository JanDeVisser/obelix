/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Logging.h>
#include <core/Range.h>

namespace Obelix {

logging_category(range);

class RangeIterator : public Object {
public:
    RangeIterator(Obj from, Obj to)
        : Object(TypeRangeIterator)
        , m_from(std::move(from))
        , m_to(std::move(to))
        , m_current_value(m_from->to_long().value())
    {
    }

    ~RangeIterator() override = default;

    std::optional<Ptr<Object>> next() override
    {
        if (m_current_value < m_to->to_long().value())
            return make_obj<Integer>(m_current_value++);
        return {};
    }

    [[nodiscard]] Ptr<Object> copy() const override
    {
        Ptr<RangeIterator> ret = make_typed<RangeIterator>(m_from, m_to);
        ret->m_current_value = m_current_value;
        return to_obj(ret);
    }

    std::optional<Obj> evaluate(std::string const& op, Ptr<Arguments> args) const override
    {
        if ((op == "*") || (op == "has_next")) {
            return make_obj<Boolean>(m_current_value < m_to->to_long().value());
        }
        //        if ((op == "@") || (op == "current")) {
        //            return make_obj<Integer>(m_current_value++);
        //        }
        return {};
    }

private:
    Obj m_from;
    Obj m_to;
    long m_current_value;
};

Range::Range(Obj const& low, Obj const& high)
    : Object(TypeRange)
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

std::optional<Obj> Range::evaluate(std::string const& op, Ptr<Arguments> args) const
{
    return Object::evaluate(op, args);
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
