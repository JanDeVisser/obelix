//
// Created by Jan de Visser on 2021-09-18.
//

#include <core/Arguments.h>
#include <core/Object.h>

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

Object::Object(std::string type)
    : m_type(move(type))
{
}

std::optional<Obj> Object::evaluate(std::string const&, Ptr<Arguments>)
{
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
    return Obelix::to_bool(to_string());
}

std::string Object::to_string() const
{
    char buf[80];
    sprintf(buf, "%s:%p", type().c_str(), this);
    return buf;
}

Obj const& Object::at(size_t ix)
{
    assert(ix == 0);
    return self();
}

int Object::compare_skel(Obj const& other) const
{
    return compare_skel(*other);
}

std::optional<Obj> Object::operator()(Ptr<Arguments>)
{
    return {};
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
    m_self = &self;
}

Obj const& Object::self() const
{
    assert(m_self);
    return *m_self;
}

SimpleIteratorState::SimpleIteratorState(Object& container, IteratorWhere where)
    : IteratorState(container)
    , m_index((where == IteratorWhere::Begin) ? 0 : container.size())
{
}

std::shared_ptr<Object> SimpleIteratorState::dereference()
{
    if ((m_index >= 0) && (m_index < container().size()))
        return container().at(m_index).pointer();
    return nullptr;
}

Obj const& ObjectIterator::operator*() {
    m_current = m_state->dereference();
    if (m_current != nullptr)
        return reinterpret_cast<Ptr<Object> const&>(m_current);
    return Object::null();
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
        assert(false);
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
        assert(false);
    }
}

Exception::Exception(ErrorCode code, ...)
    : Object("exception")
    , m_code(code)
{
    va_list args;
    va_start(args, code);
    m_message = ErrorCode_name(code) + ": " + vformat(ErrorCode_message(code), args);
    va_end(args);
}

std::optional<Obj> Integer::evaluate(std::string const& op, Ptr<Arguments> args)
{
    if (op == "+") {
        long ret = m_value;
        for (auto& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op.c_str(), "int", arg->type().c_str());
            }
            ret += int_maybe.value();
        }
        return make_obj<Integer>(ret);
    } else if (op == "-") {
        long ret = m_value;
        if (args->arguments().empty()) {
            ret = -ret;
        } else {
            for (auto& arg : args->arguments()) {
                auto int_maybe = arg->to_long();
                if (!int_maybe.has_value()) {
                    return make_obj<Exception>(ErrorCode::TypeMismatch, op.c_str(), "int", arg->type().c_str());
                }
                ret -= int_maybe.value();
            }
        }
        return make_obj<Integer>(ret);
    } else if (op == "*") {
        if (args->arguments().empty()) {
            return make_obj<Exception>(ErrorCode::CantUseAsUnaryOp, op.c_str(), "12");
        }
        long ret = m_value;
        for (auto& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op.c_str(), "int", arg->type().c_str());
            }
            ret *= int_maybe.value();
        }
        return make_obj<Integer>(ret);
    } else if (op == "/") {
        if (args->arguments().empty()) {
            return make_obj<Exception>(ErrorCode::CantUseAsUnaryOp, op.c_str(), "!@");
        }
        long ret = m_value;
        for (auto& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op.c_str(), "int", arg->type().c_str());
            }
            ret /= int_maybe.value();
        }
        return make_obj<Integer>(ret);
    } else {
        return Object::evaluate(op, args);
    }
}

std::optional<Obj> Float::evaluate(std::string const& op, Ptr<Arguments> args)
{
    return Object::evaluate(op, args);
}

Ptr<Boolean> const& Boolean::True()
{
    static Ptr<Boolean> s_true = make_typed<Boolean>(true);
    return s_true;
}

Ptr<Boolean> const& Boolean::False()
{
    static Ptr<Boolean> s_false = make_typed<Boolean>(false);
    return s_false;
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