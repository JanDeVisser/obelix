/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

//
// Created by Jan de Visser on 2021-09-18.
//

#include <core/Arguments.h>
#include <core/Iterator.h>
#include <core/Logging.h>
#include <core/Object.h>
#include <core/Range.h>

namespace Obelix {

#if 0
static std::unordered_map<std::string, ObjectType> s_types;

std::optional<ObjectType const&> const& ObjectType::get(std::string const& type_name)
{
    if (s_types.contains(type_name)) {
        auto ret = s_types.at(type_name);
        return ret;
    }
    return {};
}
#endif

logging_category(object);

Object::Object(ObelixType type)
    : m_type(type)
{
}

int Object::compare(Object const& other) const
{
    return compare(other.self());
}

Obj Object::copy() const
{
    fatal("Not implemented");
}

std::optional<Obj> Object::evaluate(std::string const& name, Ptr<Arguments> args)
{
    auto apply_and_assign = [this, args](std::string const& op) -> std::optional<Obj> {
        auto attribute = args[0]->to_string();
        auto current_val_maybe = resolve(attribute);
        if (!current_val_maybe.has_value())
            return make_obj<Exception>(ErrorCode::NameUnresolved, attribute);
        auto new_val_maybe = current_val_maybe.value()->evaluate(op, make_typed<Arguments>(args[1]));
        if (!new_val_maybe.has_value())
            return make_obj<Exception>(ErrorCode::OperatorUnresolved, op, current_val_maybe.value());
        return assign(attribute, new_val_maybe.value());
    };

    if (name == ".") {
        assert(args->size() == 1);
        auto ret = resolve(args->at(0)->to_string());
        return ret;
    } else if (name == "=") {
        assert(args->size() == 2);
        return assign(args->at(0)->to_string(), args->at(1));
    } else if (name == "<") {
        return make_obj<Boolean>(compare(args->get(0)) < 0);
    } else if (name == ">") {
        return make_obj<Boolean>(compare(args->get(0)) > 0);
    } else if (name == "<=") {
        return make_obj<Boolean>(compare(args->get(0)) <= 0);
    } else if (name == "=>") {
        return make_obj<Boolean>(compare(args->get(0)) >= 0);
    } else if (name == "==") {
        return make_obj<Boolean>(compare(args->get(0)) == 0);
    } else if (name == "!=") {
        return make_obj<Boolean>(compare(args->get(0)) != 0);
    } else if (name == "..") {
        return make_obj<Range>(self(), args[0]);
    } else if (name == ":") {
        return iterator();
    } else if (name.ends_with("=")) {
        return apply_and_assign(name.substr(0, name.length() - 1));
    } else if (name == "typename") {
        return make_obj<String>(type_name());
    } else if (name == "size") {
        return make_obj<Integer>(size());
    } else if (name == "empty") {
        return make_obj<Boolean>(empty());
    }
    return {};
}

std::optional<Obj> Object::evaluate(std::string const& name)
{
    return evaluate(name, make_typed<Arguments>());
}

std::optional<Ptr<Object>> Object::resolve(std::string const& name) const
{
    if (name == "type") {
        auto ret = make_obj<String>(type_name());
        return ret;
    }
    return {};
}

std::optional<Ptr<Object>> Object::assign(std::string const&, Obj const&)
{
    return {};
}

std::optional<Ptr<Object>> Object::iterator() const
{
    return make_obj<SimpleIterator>(*this);
}

std::optional<Ptr<Object>> Object::next()
{
    return {};
}

ObjectIterator Object::begin() const
{
    return ObjectIterator::begin(*this);
}

ObjectIterator Object::end() const
{
    return ObjectIterator::end(*this);
}

ObjectIterator Object::cbegin() const
{
    return ObjectIterator::begin(*this);
}

ObjectIterator Object::cend() const
{
    return ObjectIterator::end(*this);
}

[[nodiscard]] std::optional<long> Object::to_long() const
{
    return Obelix::to_long(to_string());
}

[[nodiscard]] std::optional<double> Object::to_double() const
{
    return Obelix::to_double(to_string());
}

std::optional<bool> Object::to_bool() const
{
    return true;
}

std::string Object::to_string() const
{
    return format("{}:{x}", type_name(), (unsigned long)this);
}

Obj const& Object::at(size_t ix)
{
    assert(ix == 0);
    return self();
}

Obj const& Object::at(size_t ix) const
{
    assert(ix == 0);
    return self();
}

Obj Object::call(Ptr<Arguments>)
{
    return make_obj<Exception>(ErrorCode::ObjectNotCallable, to_string());
}

Obj Object::operator()(Ptr<Arguments> args)
{
    return call(std::move(args));
}

Obj const& Object::null()
{
    static Obj s_null = ptr_cast<Object>(Null::null());
    return s_null;
}

void Object::set_self(Ptr<Object> self)
{
    m_self = self.pointer();
}

Obj const& Object::self() const
{
    assert(m_self.get());
    return *((Ptr<Object>*)(&m_self));
}

Ptr<Null> const& Null::null()
{
    static Ptr<Null> s_null = make_typed<Null>();
    return s_null;
}

std::optional<Obj> Exception::evaluate(std::string const&, Ptr<Arguments>)
{
    return {};
}

std::optional<Obj> Exception::resolve(std::string const& name) const
{
    return self();
}

std::optional<Obj> Exception::assign(std::string const&, Obj const&)
{
    return self();
}

Error const& Exception::error() const
{
    return m_error;
}

int Float::compare(Obj const& other) const
{
    auto double_maybe = other->to_double();
    if (!double_maybe.has_value())
        return 1;
    double diff = m_value - double_maybe.value();
    if (diff < std::numeric_limits<double>::epsilon())
        return 0;
    else
        return (diff < 0) ? -1 : 1;
}

int NVP::compare(Obj const& other) const
{
    auto nvp = ptr_cast<NVP>(other);
    if (m_pair.first == nvp->m_pair.first)
        return m_pair.second->compare(nvp->m_pair.second);
    else if (m_pair.first < nvp->m_pair.first)
        return -1;
    else
        return 1;
}

std::optional<Obj> NVP::resolve(std::string const& name) const
{
    if (name == "name")
        return make_obj<String>(m_pair.first);
    if (name == "value")
        return m_pair.second;
    return Object::resolve(name);
}

}
