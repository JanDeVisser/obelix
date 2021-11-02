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

Object::Object(std::string type)
    : m_type(move(type))
{
}

int Object::compare(Object const& other) const
{
    return compare(other.self());
}

IteratorState* Object::iterator_state(IteratorState::IteratorWhere where)
{
    return new SimpleIteratorState(*this, where);
}

std::optional<Obj> Object::evaluate(std::string const& name, Ptr<Arguments> args)
{
    if (name == ".") {
        assert(args->size() == 1);
        auto ret = resolve(args->at(0)->to_string());
        debug(object, "operator '.': ret.has_value): {}", ret.has_value());
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
        return make_obj<Boolean>(compare(args->get(0)) == 0);
    } else if (name == "..") {
        debug(object, "evaluate:.. {}", type());
        debug(object, "evaluate:.. {}", m_self->type());
        return make_obj<Range>(self(), args[0]);
    } else if (name == "typename") {
        return make_obj<String>(type());
    } else if (name == "size") {
        return make_obj<Integer>(size());
    } else if (name == "empty") {
        return make_obj<Boolean>(empty());
    }
    return {};
}

std::optional<Ptr<Object>> Object::resolve(std::string const& name) const
{
    if (name == "type") {
        auto ret = make_obj<String>(type());
        return ret;
    }
    return {};
}

std::optional<Ptr<Object>> Object::assign(std::string const&, Obj const&)
{
    return {};
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
    char buf[80];
    return format("{}:{x}", type(), (unsigned long)this);
    return buf;
}

Obj const& Object::at(size_t ix)
{
    assert(ix == 0);
    return self();
}

Obj Object::call(Ptr<Arguments>)
{
    return make_obj<Exception>(ErrorCode::ObjectNotCallable);
}

Obj Object::operator()(Ptr<Arguments> args)
{
    return call(std::move(args));
}

ObjectIterator Object::begin()
{
    return ObjectIterator::begin(*this);
}

ObjectIterator Object::end()
{
    return ObjectIterator::end(*this);
}

ObjectIterator Object::cbegin()
{
    return ObjectIterator::begin(*this);
}

ObjectIterator Object::cend()
{
    return ObjectIterator::end(*this);
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

Obj Object::self() const
{
    assert(m_self.get());
    return make_from_shared(m_self);
}

Ptr<Null> const& Null::null()
{
    static Ptr<Null> s_null = make_typed<Null>();
    return s_null;
}

std::string ErrorCode_name(ErrorCode code)
{
    switch (code) {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) \
    case ErrorCode::code:          \
        return #code;
        ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
    default:
        fatal("Unreachable");
    }
}

std::string ErrorCode_message(ErrorCode code)
{
    switch (code) {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) \
    case ErrorCode::code:          \
        return msg;
        ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
    default:
        fatal("Unreachable");
    }
}

Exception::Exception(ErrorCode code, std::string const& message)
    : Object("exception")
    , m_code(code)
{
    m_message = ErrorCode_name(code) + ": " + message;
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


std::unordered_map<std::string, ObjectType> ObjectType::s_types;
[[maybe_unused]] ObjectType s_integer("integer", [](std::vector<Obj> const& args) {
   if (args.size() == 1) {
       return make_obj<Integer>(args[0]->to_long().value());
   } else {
       return make_obj<Integer>();
   }
});

}
