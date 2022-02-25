/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstring>
#include <functional>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <core/Format.h>

namespace Obelix {

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
    S(NVP, 15)                    \
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
    S(Assignable, 10002)          \
    S(Incrementable, 10003)       \
    S(IntegerNumber, 10004)       \
    S(SignedIntegerNumber, 10005)

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

class ObjectType {
public:
    ObjectType() = default;

    template<typename ObjectTypeBuilder>
    ObjectType(ObelixType type, ObjectTypeBuilder const& builder) noexcept
        : m_type(type)
    {
        builder(*this);
    }

    template<typename ObjectTypeBuilder>
    ObjectType(ObelixType type, char const* name, ObjectTypeBuilder const& builder) noexcept
        : m_type(type)
        , m_name(name)
    {
        builder(*this);
    }

    [[nodiscard]] ObelixType type() const { return m_type; }
    [[nodiscard]] std::string name() const { return (m_name) ? m_name : ObelixType_name(type()); }
    [[nodiscard]] std::string to_string() const
    {
        std::string ret = name();
        auto glue = '<';
        for (auto& parameter : template_parameters()) {
            ret += glue;
            ret += parameter;
            glue = ',';
        }
        if (glue == ',')
            ret += '>';
        return ret;
    }

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
    void has_template_parameter(std::string const& parameter) { m_template_parameters.push_back(parameter); }
    void has_size(size_t sz) { m_size = sz; }

    [[nodiscard]] bool is_a(ObelixType other) const
    {
        if ((other == type()) || (other == TypeAny))
            return true;
        return m_is_a.contains(other);
    }

    [[nodiscard]] bool is_a(ObjectType const& other) const { return is_a(other.type()); }
    [[nodiscard]] ObelixType return_type_of(std::string_view method_name, ObelixTypes const& argument_types) const;
    [[nodiscard]] bool is_parameterized() const { return !m_template_parameters.empty(); }
    [[nodiscard]] size_t size() const { return m_size; }
    [[nodiscard]] std::vector<std::string> const& template_parameters() const { return m_template_parameters; }

    [[nodiscard]] bool is_template_instantiation() const { return m_instantiates_template != nullptr; }
    [[nodiscard]] std::shared_ptr<ObelixType> instatiates_template() const { return m_instantiates_template; }
    [[nodiscard]] std::vector<std::shared_ptr<ObelixType>> const& template_arguments() const { return m_template_arguments; }

    template<typename ObjectTypeBuilder>
    static std::shared_ptr<ObjectType> register_type(ObelixType type, ObjectTypeBuilder const& builder) noexcept
    {
        auto ptr = std::make_shared<ObjectType>(type, ObelixType_name(type), builder);
        s_types_by_id[type] = ptr;
        s_types_by_name[ptr->name()] = ptr;
        return ptr;
    }

    template<typename ObjectTypeBuilder>
    static std::shared_ptr<ObjectType> register_type(ObelixType type, char const* name, ObjectTypeBuilder const& builder) noexcept
    {
        auto ptr = std::make_shared<ObjectType>(type, builder);
        s_types_by_id[type] = ptr;
        return ptr;
    }

    static std::shared_ptr<ObjectType> get(ObelixType);
    static std::shared_ptr<ObjectType> get(std::string const&);
    //    static std::shared_ptr<ObjectType> instantiate_template(std::shared_ptr<ObjectType>, std::vector<std::shared_ptr<ObjectType>>);

private:
    ObelixType m_type { TypeUnknown };
    char const* m_name { nullptr };
    size_t m_size { 8 };
    MethodDescriptions m_methods {};
    std::unordered_set<ObelixType> m_is_a;
    std::vector<std::string> m_template_parameters {};
    std::shared_ptr<ObelixType> m_instantiates_template { nullptr };
    std::vector<std::shared_ptr<ObelixType>> m_template_arguments {};
    static std::unordered_map<ObelixType, std::shared_ptr<ObjectType>> s_types_by_id;
    static std::unordered_map<std::string, std::shared_ptr<ObjectType>> s_types_by_name;
};

using ObjectTypes = std::vector<std::shared_ptr<ObjectType>>;
std::string type_name(std::shared_ptr<ObjectType> type);

template<>
struct Converter<ObjectType*> {
    static std::string to_string(ObjectType const* val)
    {
        return val->to_string();
    }

    static double to_double(ObjectType const* val)
    {
        return NAN;
    }

    static unsigned long to_long(ObjectType const* val)
    {
        return 0;
    }
};

}

template<>
struct std::hash<Obelix::ObjectType> {
    auto operator()(Obelix::ObjectType const& type) const noexcept
    {
        return std::hash<int> {}(type.type());
    }
};
