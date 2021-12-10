/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Obelix {

#define ENUMERATE_OBELIX_TYPES(S) \
    S(Unknown, -1)                \
    S(Null, 0)                    \
    S(Int, 1)                     \
    S(Unsigned, 2)                \
    S(Boolean, 3)                 \
    S(Float, 4)                   \
    S(String, 5)                  \
    S(Object, 6)                  \
    S(List, 7)                    \
    S(Regex, 8)                   \
    S(Range, 9)                   \
    S(Exception, 10)              \
    S(NVP, 11)                    \
    S(Arguments, 12)              \
    S(Iterator, 13)               \
    S(NativeFunction, 14)         \
    S(RangeIterator, 15)          \
    S(BoundFunction, 16)          \
    S(Scope, 17)                  \
    S(MapIterator, 18)            \
    S(Error, 9996)                \
    S(Self, 9997)                 \
    S(Compatible, 9998)           \
    S(Argument, 9999)             \
    S(Any, 10000)                 \
    S(Comparable, 10001)          \
    S(Assignable, 10002)

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
    }
}

struct MethodParameter {
    MethodParameter(char const* n, ObelixType t)
        : name(n)
        , type(t)
    {
    }

    char const* name;
    ObelixType type;
};

typedef std::vector<MethodParameter> MethodParameters;

class MethodDescription {
public:
    MethodDescription(char const* name, ObelixType type)
        : m_name(name)
        , m_return_type(type)
        , m_varargs(false)
    {
    }

    template<typename... Args>
    MethodDescription(char const* name, ObelixType type, Args&&... args)
        : MethodDescription(name, type)
    {
        add_parameters(std::forward<Args>(args)...);
    }

    void add_parameter(MethodParameter const& parameter) { m_parameters.emplace_back(parameter); }
    void add_parameters(MethodParameter const& parameter) { add_parameter(parameter); }
    void add_parameters() { }

    template<typename... Args>
    void add_parameters(MethodParameter const& parameter, Args&&... args)
    {
        m_parameters.emplace_back(parameter);
        add_parameters(std::forward<Args>(args)...);
    }

    [[nodiscard]] std::string_view name() const { return m_name; }
    [[nodiscard]] ObelixType return_type() const { return m_return_type; }
    [[nodiscard]] bool varargs() const { return m_varargs; }
    [[nodiscard]] MethodParameters const& parameters() const { return m_parameters; }

private:
    char const* m_name;
    ObelixType m_return_type;
    bool m_varargs;
    MethodParameters m_parameters;
};

typedef std::vector<MethodDescription> MethodDescriptions;
typedef std::function<void(class ObjectType&)> ObjectTypeBuilder;

class ObjectType {
public:
    ObjectType() = default;
    ObjectType(ObelixType type, ObjectTypeBuilder const& builder) noexcept
        : m_type(type)
    {
        builder(*this);
    }

    ObjectType(ObelixType type, char const* name, ObjectTypeBuilder const& builder) noexcept
        : m_type(type)
        , m_name(name)
    {
        builder(*this);
    }

    [[nodiscard]] ObelixType type() const { return m_type; }
    [[nodiscard]] std::string_view name() const { return (m_name) ? m_name : ObelixType_name(type()); }
    MethodDescription& add_method(MethodDescription const& md)
    {
        m_methods.push_back(md);
        return m_methods.back();
    }

    template<typename... Args>
    MethodDescription& add_method(MethodDescription const& md, Args&&... args)
    {
        auto& added_md = add_method(md);
        added_md.add_parameters(std::forward<Args>(args)...);
        return added_md;
    }

    void will_be_a(ObelixType type) { m_is_a.insert(type); }

    [[nodiscard]] bool is_a(ObelixType other) const
    {
        if ((other == type()) || (other == TypeAny))
            return true;
        return m_is_a.contains(other);
    }

    [[nodiscard]] bool is_a(ObjectType const& other) const { return is_a(other.type()); }

    [[nodiscard]] ObelixType return_type_of(std::string_view method_name, ObelixTypes const& argument_types) const;

    static std::shared_ptr<ObjectType> register_type(ObelixType type, ObjectTypeBuilder const& builder) noexcept
    {
        auto ptr = std::make_shared<ObjectType>(type, builder);
        s_types[type] = ptr;
        return ptr;
    }

    static std::optional<std::shared_ptr<ObjectType>> get(ObelixType);

private:
    ObelixType m_type { TypeUnknown };
    char const* m_name { nullptr };
    MethodDescriptions m_methods {};
    std::unordered_set<ObelixType> m_is_a;
    static std::unordered_map<ObelixType, std::shared_ptr<ObjectType>> s_types;
};

}