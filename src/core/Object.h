//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include <core/Format.h>
#include <core/Logging.h>
#include <core/StringUtil.h>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Obelix {

class Object;
class ObjectIterator;
class Null;
class Arguments;

template<typename T>
class Ptr;
typedef Ptr<Object> Obj;

class IteratorState {
public:
    enum class IteratorWhere {
        Begin,
        End
    };
    virtual ~IteratorState() = default;
    virtual void increment(ptrdiff_t delta) = 0;
    virtual std::shared_ptr<Object> dereference() = 0;
    [[nodiscard]] virtual IteratorState* copy() const = 0;
    [[nodiscard]] virtual bool equals(IteratorState const* other) const = 0;

    [[nodiscard]] Object& container() const
    {
        return m_container;
    }

    [[nodiscard]] virtual bool equals(std::unique_ptr<IteratorState> const& other) const
    {
        return equals(other.get());
    }

protected:
    explicit IteratorState(Object& container)
        : m_container(container)
    {
    }

    IteratorState(IteratorState const& original) = default;

    Object& m_container;
};

class SimpleIteratorState : public IteratorState {
public:
    explicit SimpleIteratorState(Object&, IteratorWhere = IteratorWhere::Begin);
    ~SimpleIteratorState() override = default;

    void increment(ptrdiff_t delta) override
    {
        m_index += delta;
    }

    std::shared_ptr<Object> dereference() override;

    [[nodiscard]] IteratorState* copy() const override
    {
        return new SimpleIteratorState(*this, m_index);
    }

    [[nodiscard]] bool equals(IteratorState const* other) const override
    {
        auto other_casted = dynamic_cast<SimpleIteratorState const*>(other);
        //        assert(container() == other_casted.container());
        return m_index == other_casted->m_index;
    }

private:
    SimpleIteratorState(SimpleIteratorState const& original) = default;
    SimpleIteratorState(SimpleIteratorState const& original, size_t index)
        : IteratorState(dynamic_cast<IteratorState const&>(original))
        , m_index(index)
    {
    }

    size_t m_index { 0 };
};

class Object {
public:
    [[nodiscard]] std::string const& type() const { return m_type; }
    virtual std::optional<Obj> evaluate(std::string const&, Ptr<Arguments>);
    [[nodiscard]] virtual std::optional<Obj> resolve(std::string const& name) const;
    [[nodiscard]] virtual std::optional<long> to_long() const;
    [[nodiscard]] virtual std::optional<double> to_double() const;
    [[nodiscard]] virtual std::optional<bool> to_bool() const;
    [[nodiscard]] virtual std::string to_string() const;
    [[nodiscard]] virtual bool is_exception() const { return false; }
    [[nodiscard]] virtual size_t size() const { return 1; }
    [[nodiscard]] virtual bool empty() const { return size() == 0; }

    [[nodiscard]] virtual IteratorState* iterator_state(IteratorState::IteratorWhere where)
    {
        return new SimpleIteratorState(*this, where);
    }

    [[nodiscard]] virtual Obj const& at(size_t);

    Obj const& operator[](size_t ix)
    {
        return at(ix);
    }

    [[nodiscard]] virtual int compare(Obj const& other) const { return -1; }

    bool operator==(Object const& other) const { return compare(other.self()) == 0; }
    bool operator!=(Object const& other) const { return compare(other.self()) != 0; }
    bool operator<(Object const& other) const { return compare(other.self()) < 0; }
    bool operator>(Object const& other) const { return compare(other.self()) > 0; }
    bool operator<=(Object const& other) const { return compare(other.self()) <= 0; }
    bool operator>=(Object const& other) const { return compare(other.self()) >= 0; }

    bool operator==(Obj const& other) const { return compare(other) == 0; }
    bool operator!=(Obj const& other) const { return compare(other) != 0; }
    bool operator<(Obj const& other) const { return compare(other) < 0; }
    bool operator>(Obj const& other) const { return compare(other) > 0; }
    bool operator<=(Obj const& other) const { return compare(other) <= 0; }
    bool operator>=(Obj const& other) const { return compare(other) >= 0; }

    [[nodiscard]] virtual unsigned long hash() const
    {
        return std::hash<std::string> {}(to_string());
    }

    virtual Obj call(Ptr<Arguments>);
    Obj operator()(Ptr<Arguments> args);

    virtual ObjectIterator begin();
    virtual ObjectIterator end();
    virtual ObjectIterator cbegin();
    virtual ObjectIterator cend();

    static Obj const& null();
    void set_self(Ptr<Object>);
    [[nodiscard]] Obj const& self() const;

protected:
    explicit Object(std::string type);

    std::string m_type;
    Ptr<Object> const* m_self { nullptr };
};

class Null : public Object {
public:
    Null()
        : Object("null")
    {
    }

    static Ptr<Null> const& null();
    [[nodiscard]] std::optional<long> to_long() const override { return {}; }
    [[nodiscard]] std::optional<bool> to_bool() const override { return false; }
    [[nodiscard]] std::string to_string() const override { return "(null)"; }
    [[nodiscard]] int compare(Obj const& other) const override
    {
        return 1;
    }
};

class Integer : public Object {
public:
    explicit Integer(int value = 0)
        : Object("integer")
        , m_value(value)
    {
    }

    std::optional<Obj> evaluate(std::string const&, Ptr<Arguments>) override;
    [[nodiscard]] std::optional<long> to_long() const override { return m_value; }
    [[nodiscard]] std::optional<double> to_double() const override { return m_value; }
    [[nodiscard]] std::optional<bool> to_bool() const override { return m_value != 0; }
    [[nodiscard]] std::string to_string() const override { return std::to_string(m_value); }
    [[nodiscard]] int compare(Obj const& other) const override;

private:
    int m_value { 0 };
};

class Boolean : public Object {
public:
    explicit Boolean(bool value = false)
        : Object("boolean")
        , m_value(value)
    {
    }

    static Ptr<Boolean> const& True();
    static Ptr<Boolean> const& False();

    std::optional<Obj> evaluate(std::string const&, Ptr<Arguments>) override;
    [[nodiscard]] std::optional<long> to_long() const override { return (m_value) ? 1 : 0; }
    [[nodiscard]] std::optional<bool> to_bool() const override { return m_value; }
    [[nodiscard]] std::string to_string() const override { return Obelix::to_string(m_value); }
    [[nodiscard]] int compare(Obj const& other) const override;

private:
    bool m_value { true };
};

class Float : public Object {
public:
    explicit Float(double value = 0)
        : Object("float")
        , m_value(value)
    {
    }

    //    std::optional<Obj> evaluate(std::string const&, Ptr<Arguments>) override;
    [[nodiscard]] std::optional<long> to_long() const override { return m_value; }
    [[nodiscard]] std::optional<double> to_double() const override { return m_value; }
    [[nodiscard]] std::optional<bool> to_bool() const override { return m_value != 0; }
    [[nodiscard]] std::string to_string() const override { return std::to_string(m_value); }
    [[nodiscard]] int compare(Obj const& other) const override;

private:
    double m_value { 0 };
};

class ObjectIterator {
public:
    friend Object;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Ptr<Object>;
    using pointer = Ptr<Object>*;
    using reference = Ptr<Object>&;

    bool operator==(ObjectIterator const& other) const
    {
        return m_state->equals(other.m_state);
    }

    bool operator!=(ObjectIterator const& other) const
    {
        return !m_state->equals(other.m_state);
    }

    ObjectIterator& operator++()
    {
        m_state->increment(1);
        return *this;
    }

    ObjectIterator operator++(int)
    {
        ObjectIterator tmp(*this);
        m_state->increment(1);
        return tmp;
    }

    Ptr<Object> const& operator*();

private:
    static ObjectIterator begin(Object& container)
    {
        return ObjectIterator(container.iterator_state(IteratorState::IteratorWhere::Begin));
    }

    static ObjectIterator end(Object& container)
    {
        return ObjectIterator(container.iterator_state(IteratorState::IteratorWhere::End));
    }

    ObjectIterator(ObjectIterator& other)
        : m_state(other.m_state->copy())
    {
    }

    explicit ObjectIterator(IteratorState* state)
        : m_state(state)
    {
    }

    std::unique_ptr<IteratorState> m_state;
    std::shared_ptr<Object const> m_current { nullptr };
};

template<typename ObjClass>
class Ptr {
public:
    Ptr() = default;
    [[nodiscard]] std::string const& type() const { return m_ptr->type(); }
    std::optional<Obj> evaluate(std::string const& name, Ptr<Arguments> args)
    {
        return m_ptr->evaluate(name, std::move(args));
    }

    [[nodiscard]] std::optional<Obj> resolve(std::string const& name) const
    {
        return m_ptr->resolve(name);
    }

    [[nodiscard]] std::optional<long> to_long() const
    {
        return m_ptr->to_long();
    }

    [[nodiscard]] std::optional<double> to_double() const
    {
        return m_ptr->to_double();
    }

    [[nodiscard]] std::optional<bool> to_bool() const
    {
        return m_ptr->to_bool();
    }

    [[nodiscard]] std::string to_string() const
    {
        return m_ptr->to_string();
    }

    [[nodiscard]] size_t size() const { return m_ptr->size(); }
    [[nodiscard]] bool empty() const { return m_ptr->empty(); }

    [[nodiscard]] Obj const& at(size_t ix) const
    {
        return m_ptr->at(ix);
    }

    Obj const& operator[](size_t ix) const { return at(ix); }

    std::optional<Obj> operator()(Ptr<Arguments> args)
    {
        return m_ptr->operator()(std::move(args));
    }

    static Obj null()
    {
        return Object::null();
    }

    static Obj True()
    {
        return ptr_cast<Object>(Boolean::True());
    }

    static Obj False()
    {
        return ptr_cast<Object>(Boolean::False());
    }

    [[nodiscard]] std::shared_ptr<Object> pointer() const { return m_ptr; }
    [[nodiscard]] explicit operator ObjClass*() const { return m_ptr.get(); }
    [[nodiscard]] explicit operator ObjClass*() { return m_ptr.get(); }
    [[nodiscard]] ObjClass const& operator*() const { return *(std::dynamic_pointer_cast<ObjClass>(m_ptr)); }
    [[nodiscard]] ObjClass& operator*() { return *(std::dynamic_pointer_cast<ObjClass>(m_ptr)); }
    [[nodiscard]] ObjClass* operator->() { return dynamic_cast<ObjClass*>(&*m_ptr); }
    [[nodiscard]] ObjClass const* operator->() const { return dynamic_cast<ObjClass const*>(&*m_ptr); }
#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-explicit-constructor"
    [[nodiscard]] operator bool() const
    {
        auto b = m_ptr->to_bool();
        assert(b.has_value());
        return b.value();
    }
#pragma clang diagnostic pop
    [[nodiscard]] bool operator!() const
    {
        return !((bool)(*this));
    }

    bool operator==(Object const& other) const { return m_ptr->compare(other.self()) == 0; }
    bool operator!=(Object const& other) const { return m_ptr->compare(other.self()) != 0; }
    bool operator<(Object const& other) const { return m_ptr->compare(other.self()) < 0; }
    bool operator>(Object const& other) const { return m_ptr->compare(other.self()) > 0; }
    bool operator<=(Object const& other) const { return m_ptr->compare(other.self()) <= 0; }
    bool operator>=(Object const& other) const { return m_ptr->compare(other.self()) >= 0; }

    bool operator==(Obj const& other) const { return m_ptr->compare(other) == 0; }
    bool operator!=(Obj const& other) const { return m_ptr->compare(other) != 0; }
    bool operator<(Obj const& other) const { return m_ptr->compare(other) < 0; }
    bool operator>(Obj const& other) const { return m_ptr->compare(other) > 0; }
    bool operator<=(Obj const& other) const { return m_ptr->compare(other) <= 0; }
    bool operator>=(Obj const& other) const { return m_ptr->compare(other) >= 0; }

private:
    std::shared_ptr<Object> m_ptr { std::make_shared<Null>() };

    template<class ObjCls, class... Args>
    friend Ptr<ObjCls> make_typed(Args&&... args);
    template<class ObjCls, class... Args>
    friend Ptr<Object> make_obj(Args&&... args);
    template<class ObjCls, class OtherObjClass>
    friend Ptr<ObjCls> ptr_cast(Ptr<OtherObjClass> const&);
    template<class ObjCls>
    friend Ptr<ObjCls> make_from_shared(std::shared_ptr<ObjCls>);
    template<class ObjCls>
    friend Ptr<Object> to_obj(Ptr<ObjCls> const&);
    template<class OtherObjClass>
    friend class Ptr;
    friend class Object;

    template<class OtherObjClass>
    explicit Ptr(Ptr<OtherObjClass> const& other)
        : m_ptr(other.m_ptr)
    {
    }

    explicit Ptr(std::shared_ptr<ObjClass> ptr)
        : m_ptr(std::dynamic_pointer_cast<Object>(ptr))
    {
    }

public:
    [[nodiscard]] ObjectIterator begin() const { return m_ptr->begin(); }
    [[nodiscard]] ObjectIterator end() const { return m_ptr->end(); }
    [[nodiscard]] ObjectIterator cbegin() const { return m_ptr->cbegin(); }
    [[nodiscard]] ObjectIterator cend() const { return m_ptr->cend(); }
};

template<class ObjCls>
Ptr<ObjCls> make_from_shared(std::shared_ptr<ObjCls> ptr)
{
    return Ptr<ObjCls>(ptr);
}

template<class ObjCls, class... Args>
Ptr<ObjCls> make_typed(Args&&... args)
{
    auto ptr = std::make_shared<ObjCls>(std::forward<Args>(args)...);
    Ptr<ObjCls> ret(ptr);
    ptr->set_self(to_obj((ret)));
    return ret;
}

template<class ObjCls>
Ptr<Object> to_obj(Ptr<ObjCls> const& from)
{
    return Ptr<Object>(from);
}

template<class ObjCls, class... Args>
Ptr<Object> make_obj(Args&&... args)
{
    return to_obj(make_typed<ObjCls>(std::forward<Args>(args)...));
}

template<class ObjCls, class OtherObjCls>
Ptr<ObjCls> ptr_cast(Ptr<OtherObjCls> const& from)
{
    return Ptr<ObjCls>(from);
}

#define ENUMERATE_ERROR_CODES(S)                                                \
    S(NoError, "There is no error")                                             \
    S(SyntaxError, "Syntax error")                                              \
    S(ObjectNotCallable, "Object is not callable")                              \
    S(FunctionUndefined, "Function '{}' in image '{}' is undefined")            \
    S(RegexpSyntaxError, "Regular expression syntax error")                     \
    S(TypeMismatch, "Type mismatch in operation '{}'. Expected '{}', got '{}'") \
    S(CouldNotResolveNode, "Could not resolve node")                            \
    S(CantUseAsUnaryOp, "Cannot use '{}' as a unary operation")

enum class ErrorCode {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) code,
    ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
};

std::string ErrorCode_name(ErrorCode);
std::string ErrorCode_message(ErrorCode);

class Exception : public Object {
public:
    Exception(ErrorCode code, std::string const&);

    template<typename... Args>
    explicit Exception(ErrorCode code, Args&&... args)
        : Object("exception")
        , m_code(code)
    {
        m_message = ErrorCode_name(code) + ": " + format(std::string(ErrorCode_message(code)), std::forward<Args>(args)...);
    }

    [[nodiscard]] ErrorCode code() const { return m_code; }
    [[nodiscard]] std::optional<long> to_long() const override { return {}; }
    [[nodiscard]] std::optional<bool> to_bool() const override { return {}; }
    [[nodiscard]] std::string to_string() const override { return m_message; }
    [[nodiscard]] bool is_exception() const override { return true; }

private:
    ErrorCode m_code { ErrorCode::NoError };
    std::string m_message {};
};

class String : public Object {
public:
    explicit String(std::string value = "")
        : Object("string")
        , m_value(move(value))
    {
    }

    std::optional<Obj> evaluate(std::string const&, Ptr<Arguments>) override;
    [[nodiscard]] std::string to_string() const override { return m_value; }
    [[nodiscard]] int compare(Obj const& other) const override;

private:
    std::string m_value {};
};

class NVP : public Object {
public:
    NVP(std::string name, Obj value)
        : Object("nvp")
        , m_pair(std::move(name), std::move(value))
    {
    }

    [[nodiscard]] std::string const& name() const { return m_pair.first; }
    [[nodiscard]] Obj value() const { return m_pair.second; }
    [[nodiscard]] std::string to_string() const override { return "(" + name() + "," + value().to_string() + ")"; }
    [[nodiscard]] int compare(Obj const& other) const override;

private:
    std::pair<std::string, Obj> m_pair;
};

class ObjectType {
public:
    ObjectType() = default;
    ObjectType(std::string name, std::function<Obj(std::vector<Obj> const&)> factory) noexcept
        : m_name(move(name))
        , m_factory(move(factory))
    {
        s_types[name] = *this;
    }

    //    static std::optional<ObjectType const&> const& get(std::string const&);
private:
    Obj make(std::vector<Obj> const& params)
    {
        return m_factory(params);
    }

    template<class ObjClass, typename... Args>
    Obj make(std::vector<Obj> params, Ptr<ObjClass> value, Args... args)
    {
        params.emplace_back(value);
        return make(params, args...);
    }

    std::string m_name {};
    std::function<Obj(std::vector<Obj> const&)> m_factory {};
    static std::unordered_map<std::string, ObjectType> s_types;
};

static inline std::string format(std::string const& fmt, std::vector<Obj> args)
{
    std::string ret = fmt;
    for (auto& arg : args) {
        ret = format_one<Obj>(fmt, arg);
    }
    return ret;
}

}