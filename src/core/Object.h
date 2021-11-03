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
    virtual Ptr<Object> const& dereference() = 0;
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

class Object {
public:
    virtual ~Object() = default;
    [[nodiscard]] std::string const& type() const { return m_type; }
    virtual std::optional<Obj> evaluate(std::string const&, Ptr<Arguments>);
    [[nodiscard]] virtual std::optional<Obj> resolve(std::string const& name) const;
    [[nodiscard]] virtual std::optional<Obj> assign(std::string const&, Obj const&);
    [[nodiscard]] virtual std::optional<long> to_long() const;
    [[nodiscard]] virtual std::optional<double> to_double() const;
    [[nodiscard]] virtual std::optional<bool> to_bool() const;
    [[nodiscard]] virtual std::string to_string() const;
    [[nodiscard]] virtual bool is_exception() const { return false; }
    [[nodiscard]] virtual size_t size() const { return 1; }
    [[nodiscard]] virtual bool empty() const { return size() == 0; }

    [[nodiscard]] virtual IteratorState* iterator_state(IteratorState::IteratorWhere where);

    [[nodiscard]] virtual Obj const& at(size_t);

    Obj const& operator[](size_t ix) { return at(ix); }

    [[nodiscard]] virtual int compare(Obj const& other) const { return -1; }
    [[nodiscard]] int compare(Object const&) const;

    bool operator==(Object const& other) const { return compare(other) == 0; }
    bool operator!=(Object const& other) const { return compare(other) != 0; }
    bool operator<(Object const& other) const { return compare(other) < 0; }
    bool operator>(Object const& other) const { return compare(other) > 0; }
    bool operator<=(Object const& other) const { return compare(other) <= 0; }
    bool operator>=(Object const& other) const { return compare(other) >= 0; }

    bool operator==(Obj const& other) const { return compare(other) == 0; }
    bool operator!=(Obj const& other) const { return compare(other) != 0; }
    bool operator<(Obj const& other) const { return compare(other) < 0; }
    bool operator>(Obj const& other) const { return compare(other) > 0; }
    bool operator<=(Obj const& other) const { return compare(other) <= 0; }
    bool operator>=(Obj const& other) const { return compare(other) >= 0; }

    [[nodiscard]] virtual unsigned long hash() const { return std::hash<std::string> {}(to_string()); }

    virtual Obj call(Ptr<Arguments>);
    Obj operator()(Ptr<Arguments> args);

    virtual ObjectIterator begin();
    virtual ObjectIterator end();
    virtual ObjectIterator cbegin();
    virtual ObjectIterator cend();

    static Obj const& null();
    void set_self(Ptr<Object>);
    [[nodiscard]] Obj self() const;

protected:
    explicit Object(std::string type);

    std::string m_type;
    std::shared_ptr<Object> m_self { nullptr };
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

class ObjectIterator {
public:
    friend Object;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Ptr<Object>;
    using pointer = Ptr<Object>*;
    using reference = Ptr<Object>&;

    bool operator==(ObjectIterator const& other) const;
    bool operator!=(ObjectIterator const& other) const;
    ObjectIterator& operator++();
    ObjectIterator operator++(int);
    Ptr<Object> const& operator*();

private:
    static ObjectIterator begin(Object& container);
    static ObjectIterator end(Object& container);
    ObjectIterator(ObjectIterator& other);
    explicit ObjectIterator(IteratorState* state);

    std::unique_ptr<IteratorState> m_state;
};

class Integer : public Object {
public:
    explicit Integer(int = 0);

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
    explicit Boolean(bool = false);

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

template<typename ObjClass>
class Ptr {
public:
    Ptr() = default;

    [[nodiscard]] explicit operator ObjClass*() const { return m_ptr.get(); }
    [[nodiscard]] explicit operator ObjClass*() { return m_ptr.get(); }
    [[nodiscard]] ObjClass const& operator*() const { return *(std::dynamic_pointer_cast<ObjClass>(m_ptr)); }
    [[nodiscard]] ObjClass& operator*() { return *(std::dynamic_pointer_cast<ObjClass>(m_ptr)); }
    [[nodiscard]] ObjClass* operator->() { return dynamic_cast<ObjClass*>(&*m_ptr); }
    [[nodiscard]] ObjClass const* operator->() const { return dynamic_cast<ObjClass const*>(&*m_ptr); }

    [[nodiscard]] bool has_nullptr() const { return m_ptr == nullptr; }

    [[nodiscard]] char const* type() const { return (m_ptr) ? m_ptr->type().c_str() : "nullptr!"; }
    Obj const& operator[](size_t ix) const { return m_ptr->at(ix); }
    std::optional<Obj> operator()(Ptr<Arguments> args) { return m_ptr->operator()(std::move(args)); }

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

    [[nodiscard]] operator bool() const
    {
        if (has_nullptr())
            return false;
        auto b = m_ptr->to_bool();
        assert(b.has_value());
        return b.value();
    }

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
    template<class OtherObjClass>
    explicit Ptr(Ptr<OtherObjClass> const& other)
        : m_ptr(other.m_ptr)
    {
    }

    explicit Ptr(std::shared_ptr<ObjClass> ptr)
        : m_ptr(std::dynamic_pointer_cast<Object>(ptr))
    {
    }

    [[nodiscard]] std::shared_ptr<Object> pointer() const { return m_ptr; }

    std::shared_ptr<Object> m_ptr { nullptr };

    template<class ObjCls>
    friend Ptr<ObjCls> make_null() noexcept;
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

template<class ObjCls>
Ptr<ObjCls> make_null() noexcept
{
    std::shared_ptr<ObjCls> ptr = nullptr;
    Ptr<ObjCls> ret(ptr);
    return ret;
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
    S(SyntaxError, "{}")                                                        \
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
    explicit String(std::string = "");

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
    [[nodiscard]] std::string to_string() const override { return "(" + name() + "," + value()->to_string() + ")"; }
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

template<>
struct Converter<Object> {
    static std::string to_string(Object val)
    {
        return val.to_string();
    }

    static double to_double(Object val)
    {
        auto dbl = val.to_double();
        assert(dbl.has_value());
        return dbl.value();
    }

    static long to_long(Object val)
    {
        auto l = val.to_long();
        assert(l.has_value());
        return l.value();
    }
};

template<class ObjClass>
struct Converter<Ptr<ObjClass>> {
    static std::string to_string(Ptr<ObjClass> val)
    {
        return val->to_string();
    }

    static double to_double(Ptr<ObjClass> val)
    {
        auto dbl = val->to_double();
        assert(dbl.has_value());
        return dbl.value();
    }

    static long to_long(Ptr<ObjClass> val)
    {
        auto l = val->to_long();
        assert(l.has_value());
        return l.value();
    }
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