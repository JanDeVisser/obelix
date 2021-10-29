//
// Created by Jan de Visser on 2021-09-18.
//

#include <core/Arguments.h>
#include <core/Logging.h>
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

std::optional<Obj> Object::evaluate(std::string const& name, Ptr<Arguments> args)
{
    if (name == ".") {
        assert(args->size() == 1);
        return resolve(args->at(0).to_string());
    } else if (name == "=") {
        assert(args->size() == 2);
        return assign(args->at(0).to_string(), args->at(1));
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
    sprintf(buf, "%s:%p", type().c_str(), this);
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

Exception::Exception(ErrorCode code, std::string const& message)
    : Object("exception")
    , m_code(code)
{
    m_message = ErrorCode_name(code) + ": " + message;
}

int Integer::compare(Obj const& other) const
{
    auto long_maybe = other->to_long();
    if (!long_maybe.has_value())
        return 1;
    return (int) m_value - long_maybe.value();
}


std::optional<Obj> Integer::evaluate(std::string const& op, Ptr<Arguments> args)
{
    if ((op == "+") || (op == "add")) {
        long ret = m_value;
        for (auto& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
            }
            ret += int_maybe.value();
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "!") || (op == "negate")) {
        if (!args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' only takes a single operand", op));
        }
        return make_obj<Boolean>(!(to_bool().value()));
    }
    if ((op == "~") || (op == "invert")) {
        if (!args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' only takes a single operand", op));
        }
        return make_obj<Integer>(~m_value);
    }
    if ((op == "-") || (op == "sub")) {
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
    }
    if ((op == "*") || (op == "mult")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Arithmetical operation '{}' requires at least 2 operands", op));
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
    }
    if ((op == "/") || (op == "div")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Arithmetical operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        for (auto& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
            }
            ret /= int_maybe.value();
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "%") || (op == "mod")) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Arithmetical operation '{}' requires exactly 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret %= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "<<") || (op == "shl")) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires exactly 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret <<= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "<<") || (op == "shr")) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires exactly 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret >>= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "|") || (op == "bitwise_or")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret |= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "&") || (op == "bitwise_and")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret &= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "^") || (op == "bitwise_xor")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret ^= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    return Object::evaluate(op, args);
}

int Boolean::compare(Obj const& other) const
{
    auto long_maybe = other->to_long();
    if (!long_maybe.has_value())
        return 1;
    return (int) to_long().value() - long_maybe.value();
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

std::optional<Obj> Boolean::evaluate(std::string const& op, Ptr<Arguments> args)
{
    if ((op == "!") || (op == "negate")) {
        if (!args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' only takes a single operand", op));
        }
        return make_obj<Boolean>(!m_value);
    }
    if ((op == "||") || (op == "or")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' requires at least 2 operands", op));
        }
        if (m_value)
            return to_obj(True());
        for (auto& arg : args->arguments()) {
            auto bool_maybe = arg->to_bool();
            if (!bool_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "bool", arg->type());
            }
            if (bool_maybe.value())
                return to_obj(True());
        }
        return to_obj(False());
    }
    if ((op == "&&") || (op == "and")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' requires at least 2 operands", op));
        }
        if (!m_value)
            return to_obj(False());
        for (auto& arg : args->arguments()) {
            auto bool_maybe = arg->to_bool();
            if (!bool_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "bool", arg->type());
            }
            if (!bool_maybe.value())
                return to_obj(False());
        }
        return to_obj(True());
    }
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

int String::compare(Obj const& other) const
{
    return to_string().compare(other->to_string());
}

std::optional<Obj> String::evaluate(std::string const& op, Ptr<Arguments> args)
{
    if (op == "+") {
        auto ret = m_value;
        for (auto& arg : args->arguments()) {
            ret += arg->to_string();
        }
        return make_obj<String>(ret);
    }
    if ((op == "*") || (op == "repeat")) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("String operation '{}' requires exactly 2 operands", op));
        }
        std::string ret;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        if (int_maybe.value() < 0) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Repeat count of string operation '{}' cannot be negative", op));
        }
        for (auto ix = 0u; ix < int_maybe.value(); ix++) {
            ret += m_value;
        }
        return make_obj<String>(ret);
    }
    return Object::evaluate(op, args);
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
