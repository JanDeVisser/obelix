/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <core/Error.h>
#include <core/Format.h>
#include <core/Logging.h>
#include <core/StringUtil.h>

namespace Obelix {

class Object;
class ObjectIterator;

class Null;
class Arguments;

template<typename T>
class Ptr;

typedef Ptr<Object> Obj;

#define ENUMERATE_OBELIX_TYPES(S) \
    S(Unknown, -1)                \
    S(Null, 0)                    \
    S(Int, 1)                     \
    S(Unsigned, 2)                \
    S(Byte, 3)                    \
    S(Char, 4)                    \
    S(Boolean, 5)                 \
    S(Float, 6)                   \
    S(String, 7)                  \
    S(Pointer, 8)                 \
    S(MinUserType, 9)             \
    S(Object, 10)                 \
    S(List, 11)                   \
    S(Regex, 12)                  \
    S(Range, 13)                  \
    S(Exception, 14)              \
    S(Arguments, 16)              \
    S(Iterator, 17)               \
    S(NativeFunction, 18)         \
    S(RangeIterator, 19)          \
    S(BoundFunction, 20)          \
    S(Scope, 21)                  \
    S(MapIterator, 22)            \
    S(Error, 9996)                \
    S(Self, 9997)                 \
    S(Compatible, 9998)           \
    S(Argument, 9999)             \
    S(Any, 10000)                 \
    S(Comparable, 10001)          \
    S(Incrementable, 10002)       \
    S(IntegerNumber, 10003)       \
    S(SignedIntegerNumber, 10004)

enum ObelixType {
#undef __ENUM_OBELIX_TYPE
#define __ENUM_OBELIX_TYPE(t, cardinal) Type##t = cardinal,
    ENUMERATE_OBELIX_TYPES(__ENUM_OBELIX_TYPE)
#undef __ENUM_OBELIX_TYPE
};

typedef std::vector<ObelixType> ObelixTypes;

constexpr const char* ObelixType_name(ObelixType t)
{
    switch (t) {
#undef __ENUM_OBELIX_TYPE
#define __ENUM_OBELIX_TYPE(t, cardinal) \
    case Type##t:                       \
        return #t;
        ENUMERATE_OBELIX_TYPES(__ENUM_OBELIX_TYPE)
#undef __ENUM_OBELIX_TYPE
    default:
        fatal("Unknown ObelixType '{}'", (int) t);
    }
}

std::optional<ObelixType> ObelixType_by_name(std::string const& t);

template<>
struct Converter<ObelixType> {
    static std::string to_string(ObelixType val)
    {
        return ObelixType_name(val);
    }

    static double to_double(ObelixType val)
    {
        return static_cast<double>(val);
    }

    static long to_long(ObelixType val)
    {
        return val;
    }
};

class Object {
public:
    template<class ObjCls, class... Args>
    friend Ptr<ObjCls> make_typed(Args&&... args);

    virtual ~Object() = default;
    [[nodiscard]] ObelixType type() const { return m_type; }
    [[nodiscard]] char const* type_name() const { return ObelixType_name(type()); }
    [[nodiscard]] virtual std::optional<Obj> evaluate(std::string const&, Ptr<Arguments>) const;
    [[nodiscard]] std::optional<Obj> evaluate(std::string const&) const;
    [[nodiscard]] virtual Obj copy() const;
    [[nodiscard]] virtual std::optional<Obj> resolve(std::string const& name) const;
    [[nodiscard]] virtual std::optional<Obj> assign(std::string const&, Obj const&);
    [[nodiscard]] virtual std::optional<Obj> iterator() const;
    [[nodiscard]] virtual std::optional<Obj> next();
    [[nodiscard]] virtual std::optional<long> to_long() const;
    [[nodiscard]] virtual std::optional<double> to_double() const;
    [[nodiscard]] virtual std::optional<bool> to_bool() const;
    [[nodiscard]] virtual std::string to_string() const;
    [[nodiscard]] virtual bool is_exception() const { return false; }
    [[nodiscard]] virtual size_t size() const { return 1; }
    [[nodiscard]] virtual bool empty() const { return size() == 0; }

    [[nodiscard]] virtual Obj const& at(size_t);
    [[nodiscard]] virtual Obj const& at(size_t) const;

    Obj const& operator[](size_t ix) { return at(ix); }
    Obj const& operator[](size_t ix) const { return at(ix); }

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

    static Obj const& null();
    [[nodiscard]] Obj const& self() const;
    virtual void construct() { }

    [[nodiscard]] ObjectIterator begin() const;
    [[nodiscard]] ObjectIterator end() const;
    [[nodiscard]] ObjectIterator cbegin() const;
    [[nodiscard]] ObjectIterator cend() const;

protected:
    explicit Object(ObelixType type);
    void set_self(Ptr<Object>);

    ObelixType m_type;
    std::shared_ptr<Object> m_self { nullptr };
};

class ObjectIterator {
public:
    friend class Iterable;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Ptr<Object>;
    using pointer = Ptr<Object>*;
    using reference = Ptr<Object>&;

    bool operator==(ObjectIterator const& other) const;
    bool operator!=(ObjectIterator const& other) const;
    ObjectIterator& operator++();
    ObjectIterator operator++(int);
    Ptr<Object> operator*();

private:
    friend Object;
    static ObjectIterator begin(Object const& container);
    static ObjectIterator end(Object const& container);
    ObjectIterator(ObjectIterator& other);
    explicit ObjectIterator(Obj const& state);
    void dereference();

    std::shared_ptr<Object> m_state;
    std::shared_ptr<Object> m_current {};
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

    [[nodiscard]] ObelixType type() const { return (m_ptr) ? m_ptr->type() : TypeUnknown; }
    [[nodiscard]] char const* type_name() const { return (m_ptr) ? m_ptr->type_name() : "nullptr!"; }
    Obj const& operator[](size_t ix) const { return m_ptr->at(ix); }
    std::optional<Obj> operator()(Ptr<Arguments> args) { return m_ptr->operator()(std::move(args)); }

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

    [[nodiscard]] ObjectIterator begin() const { return m_ptr->begin(); }
    [[nodiscard]] ObjectIterator end() const { return m_ptr->end(); }
    [[nodiscard]] ObjectIterator cbegin() const { return m_ptr->cbegin(); }
    [[nodiscard]] ObjectIterator cend() const { return m_ptr->cend(); }

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
    friend class ObjectIterator;

public:
    [[nodiscard]] std::optional<Obj> evaluate(std::string const& name, Ptr<Arguments> const& args) const
    {
        return m_ptr->evaluate(name, args);
    }

    [[nodiscard]] std::optional<Obj> evaluate(std::string const& name) const
    {
        return m_ptr->evaluate(name);
    }

    template<typename... Args>
    std::optional<Obj> evaluate(std::string const& name, Args&&... args) const
    {
        return m_ptr->evaluate(name, make_typed<Arguments>(std::forward<Args>(args)...));
    }
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
    ptr->construct();
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
    std::string remainder = fmt;
    std::string ret;
    for (auto& arg : args) {
        auto first_formatted = format_one<Obj>(remainder, arg);
        ret += first_formatted.first;
        remainder = first_formatted.second;
    }
    return ret;
}

}
