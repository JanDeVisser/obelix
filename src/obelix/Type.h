/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <core/Error.h>
#include <core/Format.h>
#include <obelix/Architecture.h>
#include <obelix/Intrinsics.h>
#include <obelix/Operator.h>

namespace Obelix {

#define ENUMERATE_PRIMITIVE_TYPES(S) \
    S(Unknown, -1)                   \
    S(Void, 0)                       \
    S(Null, 1)                       \
    S(Int, 2)                        \
    S(Unsigned, 3)                   \
    S(Byte, 4)                       \
    S(Char, 5)                       \
    S(Boolean, 6)                    \
    S(Float, 7)                      \
    S(String, 8)                     \
    S(Pointer, 9)                    \
    S(Range, 10)                     \
    S(Error, 9996)                   \
    S(Self, 9997)                    \
    S(Compatible, 9998)              \
    S(Argument, 9999)                \
    S(Any, 10000)                    \
    S(Comparable, 10001)             \
    S(Incrementable, 10002)          \
    S(IntegerNumber, 10003)          \
    S(SignedIntegerNumber, 10004)

enum class PrimitiveType {
#undef __ENUM_PRIMITIVE_TYPE
#define __ENUM_PRIMITIVE_TYPE(t, cardinal) t = cardinal,
    ENUMERATE_PRIMITIVE_TYPES(__ENUM_PRIMITIVE_TYPE)
#undef __ENUM_PRIMITIVE_TYPE
};

typedef std::vector<PrimitiveType> PrimitiveTypes;

constexpr const char* PrimitiveType_name(PrimitiveType t)
{
    switch (t) {
#undef __ENUM_PRIMITIVE_TYPE
#define __ENUM_PRIMITIVE_TYPE(t, cardinal) \
    case PrimitiveType::t:                 \
        return #t;
        ENUMERATE_PRIMITIVE_TYPES(__ENUM_PRIMITIVE_TYPE)
#undef __ENUM_PRIMITIVE_TYPE
    }
}

std::optional<PrimitiveType> PrimitiveType_by_name(std::string const&);

template<>
struct Converter<PrimitiveType> {
    static std::string to_string(PrimitiveType val)
    {
        return PrimitiveType_name(val);
    }

    static double to_double(PrimitiveType val)
    {
        return static_cast<double>(val);
    }

    static long to_long(PrimitiveType val)
    {
        return static_cast<long>(val);
    }
};

class ObjectType;
using ObjectTypes = std::vector<std::shared_ptr<ObjectType>>;

struct MethodImpl {
    bool is_intrinsic { false };
    IntrinsicType intrinsic { IntrinsicType::NotIntrinsic };
    std::string native_function;
};

struct MethodParameter {
    MethodParameter(char const* n, PrimitiveType);
    MethodParameter(char const* n, std::shared_ptr<ObjectType>);

    char const* name;
    std::shared_ptr<ObjectType> type;
};

typedef std::vector<MethodParameter> MethodParameters;

class MethodDescription {
public:
    MethodDescription(char const*, PrimitiveType, IntrinsicType = IntrinsicType::NotIntrinsic, MethodParameters = {});
    MethodDescription(char const*, std::shared_ptr<ObjectType>, IntrinsicType = IntrinsicType::NotIntrinsic, MethodParameters = {});
    MethodDescription(Operator, PrimitiveType, IntrinsicType = IntrinsicType::NotIntrinsic, MethodParameters = {});
    MethodDescription(Operator, std::shared_ptr<ObjectType>, IntrinsicType = IntrinsicType::NotIntrinsic, MethodParameters = {});

    [[nodiscard]] std::string_view name() const 
    {
        if (m_is_operator)
            return Operator_name(m_operator);
        return m_name;
    }

    [[nodiscard]] Operator op() const { return m_operator; }
    [[nodiscard]] std::shared_ptr<ObjectType> const& return_type() const { return m_return_type; }
    void set_return_type(std::shared_ptr<ObjectType> ret_type) { m_return_type = move(ret_type); }
    void set_default_implementation(std::string const&);
    void set_default_implementation(IntrinsicType);
    void set_implementation(Architecture, IntrinsicType);
    void set_implementation(Architecture, std::string const&);
    [[nodiscard]] MethodImpl const& implementation(Architecture) const;
    [[nodiscard]] bool varargs() const { return m_varargs; }
    [[nodiscard]] MethodParameters const& parameters() const { return m_parameters; }
    [[nodiscard]] bool is_operator() const { return m_is_operator; }

private:
    union {
        char const* m_name { nullptr };
        Operator m_operator;
    };
    bool m_is_operator;
    std::shared_ptr<ObjectType> m_return_type;
    bool m_varargs { false };
    MethodParameters m_parameters {};
    MethodImpl m_default_implementation { true, IntrinsicType::NotIntrinsic, "" };
    std::unordered_map<Architecture,MethodImpl> m_implementations {};
};

using MethodDescriptions = std::vector<MethodDescription>;
using ObjectTypeBuilder = std::function<void(ObjectType&)>;

class ObjectType {
public:
    ObjectType(PrimitiveType type, const char* name) noexcept
        : m_type(type)
        , m_name(name)
    {
    }

    ObjectType(PrimitiveType type, ObjectTypeBuilder const& builder) noexcept
        : m_type(type)
    {
        m_name_str = std::string(PrimitiveType_name(type));
        builder(*this);
    }

    ObjectType(PrimitiveType type, char const* name, ObjectTypeBuilder const& builder) noexcept
        : m_type(type)
        , m_name(name)
    {
        if (m_name)
            m_name_str = std::string(m_name);
        else
            m_name_str = std::string(PrimitiveType_name(type));
        builder(*this);
    }

    [[nodiscard]] PrimitiveType type() const { return m_type; }
    [[nodiscard]] std::string const& name() const { return m_name_str; }
    [[nodiscard]] std::string to_string() const;

    MethodDescription& add_method(MethodDescription const& md)
    {
        m_methods.push_back(md);
        return m_methods.back();
    }

    void will_be_a(PrimitiveType type) { m_is_a.push_back(ObjectType::get(type)); }
    void has_template_parameter(std::string const& parameter) { m_template_parameters.push_back(parameter); }
    void has_size(size_t sz) { m_size = sz; }
    void has_template_stamp(ObjectTypeBuilder const& stamp) { m_stamp = stamp; }

    [[nodiscard]] bool is_parameterized() const { return !m_template_parameters.empty(); }
    [[nodiscard]] size_t size() const { return m_size; }
    [[nodiscard]] std::vector<std::string> const& template_parameters() const { return m_template_parameters; }
    [[nodiscard]] bool is_template_instantiation() const { return m_instantiates_template != nullptr; }
    [[nodiscard]] std::shared_ptr<ObjectType> instantiates_template() const { return m_instantiates_template; }
    [[nodiscard]] ObjectTypes const& template_arguments() const { return m_template_arguments; }
    [[nodiscard]] bool is_a(ObjectType const* other) const;
    [[nodiscard]] bool is_a(std::shared_ptr<ObjectType> other) const { return is_a(other.get()); }
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(std::string_view method_name, ObjectTypes const& argument_types = {}) const;
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(Operator op, ObjectTypes const& argument_types = {}) const;

    [[nodiscard]] std::optional<MethodDescription> get_method(Operator op, ObjectTypes const& argument_types = {});

    bool operator==(ObjectType const&) const;

    template<typename... Args>
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(std::string_view method_name, ObjectTypes& arg_types, std::shared_ptr<ObjectType> arg_type, Args&&... args) const
    {
        arg_types.push_back(arg_type);
        return return_type_of(method_name, arg_types, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(std::string_view method_name, std::shared_ptr<ObjectType> arg_type, Args&&... args) const
    {
        ObjectTypes arg_types { move(arg_type) };
        return return_type_of(method_name, arg_types, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(Operator op, ObjectTypes& arg_types, std::shared_ptr<ObjectType> arg_type, Args&&... args) const
    {
        arg_types.push_back(arg_type);
        return return_type_of(op, arg_types, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(Operator op, std::shared_ptr<ObjectType> arg_type, Args&&... args) const
    {
        ObjectTypes arg_types { move(arg_type) };
        return return_type_of(op, arg_types, std::forward<Args>(args)...);
    }

    static std::shared_ptr<ObjectType> register_type(PrimitiveType type) noexcept
    {
        auto ptr = std::make_shared<ObjectType>(type, PrimitiveType_name(type));
        s_types_by_id[type] = ptr;
        s_types_by_name[ptr->name()] = ptr;
        return ptr;
    }

    template<typename ObjectTypeBuilder>
    static std::shared_ptr<ObjectType> register_type(PrimitiveType type, ObjectTypeBuilder const& builder) noexcept
    {
        auto ptr = std::make_shared<ObjectType>(type, PrimitiveType_name(type), builder);
        s_types_by_id[type] = ptr;
        s_types_by_name[ptr->name()] = ptr;
        return ptr;
    }

    template<typename ObjectTypeBuilder>
    static std::shared_ptr<ObjectType> register_type(PrimitiveType type, char const* name, ObjectTypeBuilder const& builder) noexcept
    {
        auto ptr = std::make_shared<ObjectType>(type, builder);
        s_types_by_id[type] = ptr;
        return ptr;
    }

    static std::shared_ptr<ObjectType> const& get(PrimitiveType);
    static std::shared_ptr<ObjectType> const& get(std::string const&);
    static std::shared_ptr<ObjectType> const& get(ObjectType const*);
    static ErrorOr<std::shared_ptr<ObjectType>> resolve(std::string const& type_name, ObjectTypes const& template_args);

    template<typename... Args>
    static std::shared_ptr<ObjectType> resolve(std::string const& type_name, std::vector<std::shared_ptr<ObjectType>> template_args, std::shared_ptr<ObjectType> template_arg, Args&&... args)
    {
        template_args.push_back(move(template_arg));
        return resolve(type_name, template_args, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static std::shared_ptr<ObjectType> resolve(std::string const& type_name, std::shared_ptr<ObjectType> template_arg, Args&&... args)
    {
        std::vector<std::shared_ptr<ObjectType>> template_args = { move(template_arg) };
        return resolve(type_name, template_args, std::forward<Args>(args)...);
    }

private:
    [[nodiscard]] bool is_compatible(MethodDescription const&, ObjectTypes const&) const;

    PrimitiveType m_type { PrimitiveType::Unknown };
    char const* m_name { nullptr };
    std::string m_name_str;
    size_t m_size { 8 };
    MethodDescriptions m_methods {};
    std::vector<std::shared_ptr<ObjectType>> m_is_a;
    std::vector<std::string> m_template_parameters {};
    std::shared_ptr<ObjectType> m_instantiates_template { nullptr };
    std::vector<std::shared_ptr<ObjectType>> m_template_arguments {};
    ObjectTypeBuilder m_stamp {};

    static std::unordered_map<PrimitiveType, std::shared_ptr<ObjectType>> s_types_by_id;
    static std::unordered_map<std::string, std::shared_ptr<ObjectType>> s_types_by_name;
    static std::vector<std::shared_ptr<ObjectType>> s_template_instantiations;
};

template <typename T>
inline std::shared_ptr<ObjectType> get_type()
{
    return nullptr;
}

template<>
inline std::shared_ptr<ObjectType> get_type<int>()
{
    return ObjectType::get(PrimitiveType::Int);
}

template<>
inline std::shared_ptr<ObjectType> get_type<long>()
{
    return ObjectType::get(PrimitiveType::Int);
}

template<>
inline std::shared_ptr<ObjectType> get_type<uint8_t>()
{
    return ObjectType::get(PrimitiveType::Char);
}

template<>
inline std::shared_ptr<ObjectType> get_type<int8_t>()
{
    return ObjectType::get(PrimitiveType::Byte);
}

template<>
inline std::shared_ptr<ObjectType> get_type<std::string>()
{
    return ObjectType::get(PrimitiveType::String);
}

template<>
inline std::shared_ptr<ObjectType> get_type<bool>()
{
    return ObjectType::get(PrimitiveType::Boolean);
}

template<>
inline std::shared_ptr<ObjectType> get_type<double>()
{
    return ObjectType::get(PrimitiveType::Float);
}

template<>
inline std::shared_ptr<ObjectType> get_type<float>()
{
    return ObjectType::get(PrimitiveType::Float);
}

template<>
inline std::shared_ptr<ObjectType> get_type<void*>()
{
    return ObjectType::get(PrimitiveType::Pointer);
}

template<>
struct Converter<ObjectType> {
    static std::string to_string(ObjectType const& val)
    {
        return val.to_string();
    }

    static double to_double(UnaryOperator const&)
    {
        return NAN;
    }

    static long to_long(UnaryOperator const&)
    {
        return 0;
    }
};

std::string type_name(std::shared_ptr<ObjectType> type);

}

template<>
struct std::hash<Obelix::ObjectType> {
    size_t operator()(Obelix::ObjectType const& type) const noexcept
    {
        size_t ret = std::hash<std::string> {}(type.name());
        for (auto& template_arg : type.template_arguments()) {
            ret ^= std::hash<Obelix::ObjectType> {}(*template_arg) << 1;
        }
        return ret;
    }
};
