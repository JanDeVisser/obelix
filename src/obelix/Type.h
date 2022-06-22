/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <core/Error.h>
#include <core/Format.h>
#include <obelix/Architecture.h>
#include <obelix/Intrinsics.h>
#include <obelix/Operator.h>

namespace Obelix {

#define ENUMERATE_PRIMITIVE_TYPES(S)         \
    S(Unknown, "unknown", -1)                \
    S(Void, "void", 0)                       \
    S(Null, "null", 1)                       \
    S(Int, "int", 2)                         \
    S(Boolean, "boolean", 6)                 \
    S(Float, "float", 7)                     \
    S(Pointer, "pointer", 8)                 \
    S(Struct, "struct", 9)                   \
    S(Range, "range", 10)                    \
    S(Array, "array", 11)                    \
    S(Error, "error", 9995)                  \
    S(Self, "self", 9996)                    \
    S(Compatible, "compatible", 9997)        \
    S(AssignableTo, "assignable_to", 9998)   \
    S(Argument, "argument", 9999)            \
    S(Any, "any", 10000)                     \
    S(Comparable, "comparable", 10001)       \
    S(Incrementable, "incementable", 10002)  \
    S(IntegerNumber, "integernumber", 10003) \
    S(SignedIntegerNumber, "signedintegernumber", 10004)

enum class PrimitiveType {
#undef __ENUM_PRIMITIVE_TYPE
#define __ENUM_PRIMITIVE_TYPE(t, str, cardinal) t = cardinal,
    ENUMERATE_PRIMITIVE_TYPES(__ENUM_PRIMITIVE_TYPE)
#undef __ENUM_PRIMITIVE_TYPE
};

typedef std::vector<PrimitiveType> PrimitiveTypes;

constexpr const char* PrimitiveType_name(PrimitiveType t)
{
    switch (t) {
#undef __ENUM_PRIMITIVE_TYPE
#define __ENUM_PRIMITIVE_TYPE(t, str, cardinal) \
    case PrimitiveType::t:                      \
        return str;
        ENUMERATE_PRIMITIVE_TYPES(__ENUM_PRIMITIVE_TYPE)
#undef __ENUM_PRIMITIVE_TYPE
    default:
        fatal("Unknowm PrimitiveType '{}'", (int) t);
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

using MethodParameters = std::vector<MethodParameter>;

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
    [[nodiscard]] std::shared_ptr<class BoundIntrinsicDecl> declaration() const;
    [[nodiscard]] std::shared_ptr<ObjectType> const& method_of() const { return m_method_of; }

protected:
    void set_method_of(std::shared_ptr<ObjectType> method_of)
    {
        m_method_of = move(method_of);
    }
    friend class ObjectType;

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
    std::shared_ptr<ObjectType> m_method_of;
};

using MethodDescriptions = std::vector<MethodDescription>;

struct FieldDef {
    FieldDef(std::string, PrimitiveType);
    FieldDef(std::string, std::shared_ptr<ObjectType>);

    std::string name;
    std::shared_ptr<ObjectType> type;
};

using FieldDefs = std::vector<FieldDef>;

using ObjectTypeBuilder = std::function<void(std::shared_ptr<ObjectType>)>;

enum class TemplateParameterType {
    Type,
    String,
    Integer,
    Boolean,
};

struct TemplateParameter {
    std::string name;
    TemplateParameterType type;
};

using TemplateParameters = std::vector<TemplateParameter>;

struct TemplateArgument {
    TemplateArgument(std::shared_ptr<ObjectType> type)
        : parameter_type(TemplateParameterType::Type)
        , value(type)
    {
    }

    TemplateArgument(long integer)
        : parameter_type(TemplateParameterType::Integer)
        , value(integer)
    {
    }

    TemplateArgument(int integer)
        : parameter_type(TemplateParameterType::Integer)
        , value(integer)
    {
    }

    TemplateArgument(std::string string)
        : parameter_type(TemplateParameterType::String)
        , value(string)
    {
    }

    TemplateArgument(bool boolean)
        : parameter_type(TemplateParameterType::Boolean)
        , value(boolean)
    {
    }

    TemplateParameterType parameter_type;
    std::variant<std::shared_ptr<ObjectType>, long, std::string, bool> value;

    [[nodiscard]] size_t hash() const;
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] bool operator==(TemplateArgument const&) const;

    [[nodiscard]] std::shared_ptr<ObjectType> as_type() const
    {
        assert(std::holds_alternative<std::shared_ptr<ObjectType>>(value));
        return std::get<std::shared_ptr<ObjectType>>(value);
    }

    [[nodiscard]] long as_integer() const
    {
        assert(std::holds_alternative<long>(value));
        return std::get<long>(value);
    }

    [[nodiscard]] std::string const& as_string() const
    {
        assert(std::holds_alternative<std::string>(value));
        return std::get<std::string>(value);
    }

    [[nodiscard]] bool as_bool() const
    {
        assert(std::holds_alternative<bool>(value));
        return std::get<bool>(value);
    }

    template <typename ArgType>
    [[nodiscard]] ArgType const& get() const
    {
        assert(std::holds_alternative<ArgType>(value));
        return std::get<ArgType>(value);
    }
};

using TemplateArguments = std::vector<TemplateArgument>;

class ObjectType {
public:
    ObjectType(PrimitiveType type, const char* name) noexcept
        : m_type(type)
        , m_name(name)
    {
        if (m_name)
            m_name_str = std::string(m_name);
        else
            m_name_str = std::string(PrimitiveType_name(type));
    }

    ObjectType(PrimitiveType type, std::string name) noexcept
        : m_type(type)
        , m_name_str(move(name))
    {
    }

    [[nodiscard]] PrimitiveType type() const { return m_type; }
    [[nodiscard]] std::string const& name() const { return m_name_str; }
    [[nodiscard]] std::string to_string() const;

    MethodDescription& add_method(MethodDescription);
    void will_be_a(std::shared_ptr<ObjectType> type) { m_is_a.push_back(type); }
    void has_template_parameter(TemplateParameter const& parameter) { m_template_parameters.push_back(parameter); }
    void has_size(size_t sz) { m_size = sz; }
    void has_template_stamp(ObjectTypeBuilder const& stamp) { m_stamp = stamp; }
    void has_alias(std::string const& alias)
    {
        m_aliases.emplace_back(alias);
        s_types_by_name[alias] = ObjectType::get(this);
    }

    [[nodiscard]] bool is_parameterized() const { return !m_template_parameters.empty(); }
    [[nodiscard]] size_t size() const;
    [[nodiscard]] std::vector<std::string> const& aliases() const { return m_aliases; }
    [[nodiscard]] ssize_t offset_of(std::string const&) const;
    [[nodiscard]] ssize_t offset_of(size_t) const;
    [[nodiscard]] TemplateParameters const& template_parameters() const { return m_template_parameters; }
    [[nodiscard]] bool is_template_specialization() const { return m_specializes_template != nullptr; }
    [[nodiscard]] std::shared_ptr<ObjectType> specializes_template() const { return m_specializes_template; }
    [[nodiscard]] TemplateArguments const& template_arguments() const { return m_template_arguments; }
    [[nodiscard]] bool has_template_argument(std::string const&) const;

    template <typename ArgType>
    [[nodiscard]] ArgType const& template_argument(std::string const& arg_name) const
    {
        assert(is_template_specialization());
        for (auto ix = 0u; ix < specializes_template()->template_parameters().size(); ++ix) {
            auto const& param = specializes_template()->template_parameters()[ix];
            if (param.name == arg_name) {
                return template_arguments()[ix].get<ArgType>();
            }
        }
        fatal("Type '{}' instantiates template '{}' which has no template parameter named '{}'",
            name(), specializes_template()->name(), arg_name);
    }

    [[nodiscard]] bool is_a(ObjectType const* other) const;
    [[nodiscard]] bool is_a(std::shared_ptr<ObjectType> other) const { return is_a(other.get()); }
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(std::string_view method_name, ObjectTypes const& argument_types = {}) const;
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(Operator op, ObjectTypes const& argument_types = {}) const;

    [[nodiscard]] std::optional<MethodDescription> get_method(Operator op) const;
    [[nodiscard]] std::optional<MethodDescription> get_method(Operator op, ObjectTypes const& argument_types) const;
    [[nodiscard]] FieldDefs const& fields() const { return m_fields; }
    [[nodiscard]] FieldDef const& field(std::string const&) const;

    bool operator==(ObjectType const&) const;
    [[nodiscard]] bool is_assignable_to(std::shared_ptr<ObjectType> const& other) const { return is_assignable_to(*other); }
    [[nodiscard]] bool is_assignable_to(ObjectType const&) const;
    [[nodiscard]] bool can_assign(std::shared_ptr<ObjectType> const& other) const { return can_assign(*other); }
    [[nodiscard]] bool can_assign(ObjectType const&) const;

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

    static std::shared_ptr<ObjectType> register_type(PrimitiveType, ObjectTypeBuilder const& = nullptr) noexcept;
    static std::shared_ptr<ObjectType> register_type(PrimitiveType, char const*, ObjectTypeBuilder const&) noexcept;
    static std::shared_ptr<ObjectType> register_type(char const*, std::shared_ptr<ObjectType>, TemplateArguments const&, ObjectTypeBuilder const& = nullptr) noexcept;

    static std::shared_ptr<ObjectType> const& get(PrimitiveType);
    static std::shared_ptr<ObjectType> const& get(std::string const&);
    static std::shared_ptr<ObjectType> const& get(ObjectType const*);
    static ErrorOr<std::shared_ptr<ObjectType>> specialize(std::shared_ptr<ObjectType> const&, TemplateArguments const&);
    static ErrorOr<std::shared_ptr<ObjectType>> specialize(std::string const&, TemplateArguments const&);

    template<typename... Args>
    static std::shared_ptr<ObjectType> specialize(std::string const& type_name, std::string const& base_name, TemplateArguments& template_args, TemplateArgument template_arg, Args&&... args)
    {
        template_args.push_back(std::move(template_arg));
        return specialize(type_name, base_name, template_args, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static std::shared_ptr<ObjectType> specialize(std::string const& type_name, std::string const& base_name, TemplateArgument template_arg, Args&&... args)
    {
        TemplateArguments template_args = { std::move(template_arg) };
        return specialize(type_name, base_name, template_args, std::forward<Args>(args)...);
    }

    static ErrorOr<std::shared_ptr<ObjectType>> make_struct_type(std::string, FieldDefs, ObjectTypeBuilder const& = nullptr);
    static std::shared_ptr<ObjectType> register_struct_type(std::string const&, FieldDefs, ObjectTypeBuilder const& = nullptr);
    static void dump();

private:
    [[nodiscard]] bool is_compatible(MethodDescription const&, ObjectTypes const&) const;
    static void register_type_in_caches(std::shared_ptr<ObjectType> const&);

    PrimitiveType m_type { PrimitiveType::Unknown };
    char const* m_name { nullptr };
    std::string m_name_str;
    size_t m_size { 8 };
    std::vector<std::string> m_aliases {};
    MethodDescriptions m_methods {};
    FieldDefs m_fields {};
    std::vector<std::shared_ptr<ObjectType>> m_is_a;
    TemplateParameters m_template_parameters {};
    std::shared_ptr<ObjectType> m_specializes_template { nullptr };
    TemplateArguments m_template_arguments {};
    ObjectTypeBuilder m_stamp {};

    static std::unordered_map<PrimitiveType, std::shared_ptr<ObjectType>> s_types_by_id;
    static std::unordered_map<std::string, std::shared_ptr<ObjectType>> s_types_by_name;
    static std::vector<std::shared_ptr<ObjectType>> s_template_specializations;
};

template <typename T>
inline std::shared_ptr<ObjectType> get_type()
{
    return nullptr;
}

template<>
inline std::shared_ptr<ObjectType> get_type<int>()
{
    return ObjectType::get("s32");
}

template<>
inline std::shared_ptr<ObjectType> get_type<long>()
{
    return ObjectType::get("s64");
}

template<>
inline std::shared_ptr<ObjectType> get_type<uint8_t>()
{
    return ObjectType::get("u8");
}

template<>
inline std::shared_ptr<ObjectType> get_type<int8_t>()
{
    return ObjectType::get("s8");
}

template<>
inline std::shared_ptr<ObjectType> get_type<std::string>()
{
    return ObjectType::get("string");
}

template<>
inline std::shared_ptr<ObjectType> get_type<bool>()
{
    return ObjectType::get("bool");
}

template<>
inline std::shared_ptr<ObjectType> get_type<double>()
{
    return ObjectType::get("float");
}

template<>
inline std::shared_ptr<ObjectType> get_type<float>()
{
    return ObjectType::get("float");
}

template<>
inline std::shared_ptr<ObjectType> get_type<void*>()
{
    return ObjectType::get("pointer");
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
struct std::hash<Obelix::ObjectType>;

template<>
struct std::hash<Obelix::TemplateArgument> {
    size_t operator()(Obelix::TemplateArgument const& arg) const noexcept { return arg.hash(); }
};

template<>
struct std::hash<Obelix::ObjectType> {
    size_t operator()(Obelix::ObjectType const& type) const noexcept
    {
        size_t ret = std::hash<std::string> {}(type.name());
        for (auto& template_arg : type.template_arguments()) {
            ret ^= std::hash<Obelix::TemplateArgument> {}(template_arg) << 1;
        }
        return ret;
    }
};
