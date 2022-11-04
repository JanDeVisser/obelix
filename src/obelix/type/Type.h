/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <core/Error.h>
#include <core/Format.h>
#include <core/Logging.h>
#include <core/StringUtil.h>
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
    S(Enum, "enum", 12)                      \
    S(Type, "type", 13)                      \
    S(Module, "module", 14)                  \
    S(List, "list", 15)                      \
    S(Function, "function", 16)              \
    S(Compilation, "compilation", 17)        \
    S(Conditional, "conditional", 18)        \
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
#undef ENUM_PRIMITIVE_TYPE
#define ENUM_PRIMITIVE_TYPE(t, str, cardinal) t = cardinal,
    ENUMERATE_PRIMITIVE_TYPES(ENUM_PRIMITIVE_TYPE)
#undef ENUM_PRIMITIVE_TYPE
};

typedef std::vector<PrimitiveType> PrimitiveTypes;

const char* PrimitiveType_name(PrimitiveType t);
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
    bool is_pure { false };
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
    MethodDescription() = default;
    MethodDescription(char const*, PrimitiveType, IntrinsicType = IntrinsicType::NotIntrinsic, MethodParameters = {}, bool pure = false);
    MethodDescription(char const*, std::shared_ptr<ObjectType>, IntrinsicType = IntrinsicType::NotIntrinsic, MethodParameters = {}, bool pure = false);
    MethodDescription(Operator, PrimitiveType, IntrinsicType = IntrinsicType::NotIntrinsic, MethodParameters = {}, bool pure = false);
    MethodDescription(Operator, std::shared_ptr<ObjectType>, IntrinsicType = IntrinsicType::NotIntrinsic, MethodParameters = {}, bool pure = false);

    [[nodiscard]] std::string_view name() const;
    [[nodiscard]] Operator op() const;
    [[nodiscard]] std::shared_ptr<ObjectType> const& return_type() const;
    void set_return_type(std::shared_ptr<ObjectType> ret_type);
    void set_default_implementation(std::string const&);
    void set_default_implementation(IntrinsicType);
    void set_implementation(Architecture, IntrinsicType);
    void set_implementation(Architecture, std::string const&);
    [[nodiscard]] MethodImpl const& implementation(Architecture) const;
    [[nodiscard]] MethodImpl const& implementation() const;
    [[nodiscard]] bool varargs() const;
    [[nodiscard]] MethodParameters const& parameters() const;
    [[nodiscard]] bool is_operator() const;
    [[nodiscard]] bool is_pure() const;
    [[nodiscard]] std::shared_ptr<class BoundIntrinsicDecl> declaration() const;
    [[nodiscard]] std::shared_ptr<ObjectType> const& method_of() const;

protected:
    void set_method_of(std::shared_ptr<ObjectType> method_of);
    friend class ObjectType;

private:
    union {
        char const* m_name { nullptr };
        Operator m_operator;
    };
    bool m_is_operator;
    bool m_is_pure { false };
    std::shared_ptr<ObjectType> m_return_type;
    bool m_varargs { false };
    MethodParameters m_parameters {};
    MethodImpl m_default_implementation { true, false, IntrinsicType::NotIntrinsic, "" };
    std::unordered_map<Architecture, MethodImpl> m_implementations {};
    std::shared_ptr<ObjectType> m_method_of;
};

using MethodDescriptions = std::vector<MethodDescription>;

struct FieldDef {
    FieldDef(std::string, PrimitiveType);
    FieldDef(std::string, std::shared_ptr<ObjectType>);

    std::string name;
    std::shared_ptr<ObjectType> type;
    [[nodiscard]] bool operator==(FieldDef const&) const;
};

using FieldDefs = std::vector<FieldDef>;

#define ENUMERATE_TEMPLATE_PARAMETER_TYPES(S) \
    S(Unknown)                                \
    S(Type)                                   \
    S(String)                                 \
    S(Integer)                                \
    S(Boolean)                                \
    S(NameValue)

enum class TemplateParameterType {
#undef ENUM_TEMPLATE_PARAMETER_TYPE
#define ENUM_TEMPLATE_PARAMETER_TYPE(t) t,
    ENUMERATE_TEMPLATE_PARAMETER_TYPES(ENUM_TEMPLATE_PARAMETER_TYPE)
#undef ENUM_TEMPLATE_PARAMETER_TYPE
};

const char* TemplateParameterType_name(TemplateParameterType t);

template<>
struct Converter<TemplateParameterType> {
    static std::string to_string(TemplateParameterType val)
    {
        return TemplateParameterType_name(val);
    }

    static double to_double(TemplateParameterType val)
    {
        return static_cast<double>(val);
    }

    static long to_long(TemplateParameterType val)
    {
        return static_cast<long>(val);
    }
};

#define ENUMERATE_TEMPLATE_PARAMETER_MULTIPLICITIES(S) \
    S(Optional /* 0-1 */)                              \
    S(Required /* 1 */)                                \
    S(Multiple /* 1- */)

enum class TemplateParameterMultiplicity {
#undef ENUM_TEMPLATE_PARAMETER_MULTIPLICITY
#define ENUM_TEMPLATE_PARAMETER_MULTIPLICITY(t) t,
    ENUMERATE_TEMPLATE_PARAMETER_MULTIPLICITIES(ENUM_TEMPLATE_PARAMETER_MULTIPLICITY)
#undef ENUM_TEMPLATE_PARAMETER_MULTIPLICITY
};

const char* TemplateParameterMultiplicity_name(TemplateParameterMultiplicity t);

template<>
struct Converter<TemplateParameterMultiplicity> {
    static std::string to_string(TemplateParameterMultiplicity val)
    {
        return TemplateParameterMultiplicity_name(val);
    }

    static double to_double(TemplateParameterMultiplicity val)
    {
        return static_cast<double>(val);
    }

    static long to_long(TemplateParameterMultiplicity val)
    {
        return static_cast<long>(val);
    }
};

using NVP = std::pair<std::string, long>;
using NVPs = std::vector<NVP>;

using TemplateArgumentValue = std::variant<std::shared_ptr<ObjectType>, long, std::string, bool, NVP>;
using TemplateArgumentValues = std::vector<TemplateArgumentValue>;

struct TemplateArgument {
    TemplateArgument() = default;
    explicit TemplateArgument(TemplateParameterType);
    TemplateArgument(std::shared_ptr<ObjectType>);
    TemplateArgument(long);
    TemplateArgument(int);
    TemplateArgument(std::string);
    TemplateArgument(bool);
    TemplateArgument(std::string, long);
    TemplateArgument(NVP);
    TemplateArgument(TemplateParameterType type, TemplateArgumentValues arguments);

    TemplateParameterType parameter_type { TemplateParameterType::Unknown };
    TemplateParameterMultiplicity multiplicity { TemplateParameterMultiplicity::Required };
    TemplateArgumentValues value {};

    [[nodiscard]] size_t hash() const;
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] bool operator==(TemplateArgument const&) const;

    [[nodiscard]] std::shared_ptr<ObjectType> as_type() const;
    [[nodiscard]] long as_integer() const;
    [[nodiscard]] std::string const& as_string() const;
    [[nodiscard]] bool as_bool() const;
    [[nodiscard]] NVP const& as_nvp() const;
    [[nodiscard]] TemplateArgumentValues const& as_values() const;

    template<typename ArgType>
    [[nodiscard]] ArgType get() const
    {
        assert(!value.empty());
        assert(std::holds_alternative<ArgType>(value[0]));
        return std::get<ArgType>(value[0]);
    }

    template<>
    [[nodiscard]] NVPs get() const
    {
        assert(parameter_type == TemplateParameterType::NameValue);
        assert(multiplicity == TemplateParameterMultiplicity::Multiple);

        NVPs ret;
        for (auto const& v : value) {
            ret.push_back(std::get<NVP>(v));
        }
        return ret;
    }
};

class TemplateArguments : public std::map<std::string, TemplateArgument> {
public:
    TemplateArguments() = default;
    TemplateArguments(std::initializer_list<std::map<std::string, Obelix::TemplateArgument>::value_type> init)
        : std::map<std::string, TemplateArgument>(init)
    {
    }

    [[nodiscard]] bool operator==(TemplateArguments const& other) const
    {
        if (size() != other.size())
            return false;
        return std::all_of(cbegin(), cend(), [&other](auto const& arg_item) {
            return other.contains(arg_item.first) && other.at(arg_item.first) == arg_item.second;
        });
    }
};

struct TemplateParameter {
    std::string name {};
    TemplateParameterType type { TemplateParameterType::Unknown };
    TemplateParameterMultiplicity multiplicity { TemplateParameterMultiplicity::Required };
    TemplateArgument default_value {};

    [[nodiscard]] std::string to_string() const;
};

using TemplateParameters = std::map<std::string, TemplateParameter>;

size_t hash(TemplateArgumentValue const&);
std::string to_string(TemplateArgumentValue const&);
bool compare(TemplateArgumentValue const&, TemplateArgumentValue const&, bool = false);

template<>
struct Converter<TemplateArguments> {
    static std::string to_string(TemplateArguments const& arguments)
    {
        std::string ret = "<";
        auto first = true;
        for (auto const& arg : arguments) {
            if (!first)
                ret += ',';
            first = false;
            ret += format("{}", arg.second);
        }
        return ret + ">";
    }

    static double to_double(TemplateArguments const&)
    {
        return NAN;
    }

    static long to_long(TemplateArguments const&)
    {
        return 0;
    }
};

enum class CanCast {
    Never,
    Sometimes,
    Always,
};

using ObjectTypeBuilder = std::function<void(std::shared_ptr<ObjectType> const&)>;
using CanCastTo = std::function<CanCast(std::shared_ptr<const ObjectType> const&, std::shared_ptr<const ObjectType> const&)>;

class ObjectType : public std::enable_shared_from_this<ObjectType> {
public:
    ObjectType(PrimitiveType, const char*) noexcept;
    ObjectType(PrimitiveType, std::string) noexcept;

    [[nodiscard]] PrimitiveType type() const { return m_type; }
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] bool is_custom() const { return m_custom; }
    void will_be_custom(bool custom) { m_custom = custom; }

    MethodDescription& add_method(MethodDescription);
    void will_be_a(std::shared_ptr<ObjectType>);
    void has_template_parameter(TemplateParameter const&);
    void has_size(size_t);
    void has_template_stamp(ObjectTypeBuilder const&);
    void has_can_cast_to(CanCastTo const&);
    void has_alias(std::string const& alias);
    [[nodiscard]] bool is_parameterized() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] std::vector<std::string> const& aliases() const;
    [[nodiscard]] ssize_t offset_of(std::string const&) const;
    [[nodiscard]] ssize_t offset_of(size_t) const;
    [[nodiscard]] TemplateParameters const& template_parameters() const;
    [[nodiscard]] TemplateParameter const& template_parameter(std::string const&) const;
    [[nodiscard]] TemplateParameter const& template_parameter(size_t) const;
    [[nodiscard]] bool is_template_specialization() const;
    [[nodiscard]] std::shared_ptr<ObjectType> specializes_template() const;
    [[nodiscard]] TemplateArguments const& template_arguments() const;
    [[nodiscard]] bool has_template_argument(std::string const&) const;

    template<typename ArgType>
    [[nodiscard]] ArgType template_argument(std::string const& arg_name) const
    {
        assert(is_template_specialization());
        if (has_template_argument(arg_name))
            return template_arguments().at(arg_name).get<ArgType>();
        if (specializes_template()->m_template_parameters.contains(arg_name)) {
            auto default_value = template_parameters().at(arg_name).default_value;
            if (default_value.parameter_type == TemplateParameterType::Unknown)
                fatal("Type '{}' instantiates template '{}' but did not specify template parameter '{}'",
                    name(), specializes_template()->name(), arg_name);
            return default_value.get<ArgType>();
        }
        fatal("Type '{}' instantiates template '{}' which has no template parameter named '{}'",
            name(), specializes_template()->name(), arg_name);
    }

    template<typename ArgType>
    [[nodiscard]] std::vector<ArgType> template_argument_values(std::string const& arg_name) const
    {
        assert(is_template_specialization());
        if (!specializes_template()->template_parameters().contains(arg_name)) {
            fatal("Type '{}' instantiates template '{}' which has no template parameter named '{}'",
                name(), specializes_template()->name(), arg_name);
        }
        auto arg_value = specializes_template()->template_parameters().at(arg_name).default_value;
        if (m_template_arguments.contains(arg_name))
            arg_value = m_template_arguments.at(arg_name);
        std::vector<ArgType> ret;
        for (auto const& v : arg_value.value) {
            ret.push_back(std::get<ArgType>(v));
        }
        return ret;
    }

    [[nodiscard]] bool is_a(ObjectType const*) const;
    [[nodiscard]] bool is_a(std::shared_ptr<ObjectType> const&) const;
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(std::string_view method_name, ObjectTypes const& argument_types = {}) const;
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(Operator op, ObjectTypes const& argument_types = {}) const;

    [[nodiscard]] std::optional<MethodDescription> get_method(Operator op) const;
    [[nodiscard]] std::optional<MethodDescription> get_method(Operator op, ObjectTypes const& argument_types) const;
    [[nodiscard]] FieldDefs const& fields() const;
    [[nodiscard]] FieldDef const& field(std::string const&) const;
    ErrorOr<void> extend_enum_type(NVPs const&);

    bool operator==(ObjectType const&) const;
    [[nodiscard]] bool is_assignable_to(std::shared_ptr<ObjectType> const&) const;
    [[nodiscard]] bool is_assignable_to(ObjectType const&) const;
    [[nodiscard]] bool is_compatible_with(std::shared_ptr<ObjectType> const&) const;
    [[nodiscard]] bool is_compatible_with(ObjectType const&) const;

    [[nodiscard]] CanCast can_cast_to(std::shared_ptr<ObjectType> const&) const;
    [[nodiscard]] CanCast can_cast_to(ObjectType const&) const;

    template<typename... Args>
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(std::string_view method_name, ObjectTypes& arg_types, std::shared_ptr<ObjectType> arg_type, Args&&... args) const
    {
        arg_types.push_back(std::move(arg_type));
        return return_type_of(method_name, arg_types, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(std::string_view method_name, std::shared_ptr<ObjectType> arg_type, Args&&... args) const
    {
        ObjectTypes arg_types { std::move(arg_type) };
        return return_type_of(method_name, arg_types, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(Operator op, ObjectTypes& arg_types, std::shared_ptr<ObjectType> arg_type, Args&&... args) const
    {
        arg_types.push_back(std::move(arg_type));
        return return_type_of(op, arg_types, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(Operator op, std::shared_ptr<ObjectType> arg_type, Args&&... args) const
    {
        ObjectTypes arg_types { std::move(arg_type) };
        return return_type_of(op, arg_types, std::forward<Args>(args)...);
    }

    static std::shared_ptr<ObjectType> register_type(PrimitiveType, ObjectTypeBuilder const& = nullptr) noexcept;
    static std::shared_ptr<ObjectType> register_type(PrimitiveType, std::string const&, ObjectTypeBuilder const&) noexcept;
    static std::shared_ptr<ObjectType> register_type(std::string, std::shared_ptr<ObjectType> const&, TemplateArguments const&, ObjectTypeBuilder const& = nullptr) noexcept;

    static std::shared_ptr<ObjectType> const& get(PrimitiveType);
    static std::shared_ptr<ObjectType> const& get(std::string const&);
    static std::shared_ptr<ObjectType> const& get(ObjectType const*);
    static ErrorOr<std::shared_ptr<ObjectType>> specialize(std::shared_ptr<ObjectType> const&, TemplateArguments const&);
    static ErrorOr<std::shared_ptr<ObjectType>> specialize(std::string const&, TemplateArguments const&);

    static ErrorOr<std::shared_ptr<ObjectType>> make_struct_type(std::string, FieldDefs, ObjectTypeBuilder const& = nullptr);
    static std::shared_ptr<ObjectType> register_struct_type(std::string const&, FieldDefs, ObjectTypeBuilder const& = nullptr);
    static std::shared_ptr<ObjectType> make_enum_type(std::string const&, NVPs const&);

    [[nodiscard]] ErrorOr<std::shared_ptr<ObjectType>> smallest_compatible_type() const;

    static void dump();

private:
    [[nodiscard]] bool is_compatible(MethodDescription const&, ObjectTypes const&) const;
    static void register_type_in_caches(std::shared_ptr<ObjectType> const&);

    PrimitiveType m_type { PrimitiveType::Unknown };
    bool m_custom { false };
    std::string m_name;
    size_t m_size { 8 };
    std::vector<std::string> m_aliases {};
    MethodDescriptions m_methods {};
    FieldDefs m_fields {};
    std::vector<std::shared_ptr<ObjectType>> m_is_a;
    TemplateParameters m_template_parameters {};
    std::vector<std::string> m_template_parameters_by_index {};
    std::shared_ptr<ObjectType> m_specializes_template { nullptr };
    TemplateArguments m_template_arguments {};
    ObjectTypeBuilder m_stamp {};
    CanCastTo m_can_cast_to {};

    static std::unordered_map<PrimitiveType, std::shared_ptr<ObjectType>> s_types_by_id;
    static std::unordered_map<std::string, std::shared_ptr<ObjectType>> s_types_by_name;
    static std::vector<std::shared_ptr<ObjectType>> s_template_specializations;
};

using pObjectType = std::shared_ptr<ObjectType>;

template<typename T>
struct TypeGetter {
    std::shared_ptr<ObjectType> operator()()
    {
        if (std::is_integral<T>()) {
            std::string prefix = "u";
            if (std::is_signed<T>())
                prefix = "s";
            auto t = format("{}{}", prefix, 8 * sizeof(T));
            return ObjectType::get(t);
        }
        if (std::is_floating_point<T>()) {
            return ObjectType::get("float");
        }
        fatal("Sorry can't do this yet");
    }
};

template<>
struct TypeGetter<bool> {
    std::shared_ptr<ObjectType> operator()()
    {
        return ObjectType::get(PrimitiveType::Boolean);
    }
};

template<>
struct TypeGetter<std::string> {
    std::shared_ptr<ObjectType> operator()()
    {
        return ObjectType::get("string");
    }
};

template<>
struct TypeGetter<std::shared_ptr<ObjectType>> {
    std::shared_ptr<ObjectType> operator()()
    {
        return ObjectType::get("type");
    }
};

template<>
struct TypeGetter<ObjectType> {
    std::shared_ptr<ObjectType> operator()()
    {
        return ObjectType::get("type");
    }
};

template<typename T>
struct TypeGetter<T*> {
    std::shared_ptr<ObjectType> operator()()
    {
        auto ret_or_error = ObjectType::specialize(ObjectType::get("pointer"),
            { { "target", TemplateArgument(TypeGetter<T> {}()) } });
        if (ret_or_error.is_error())
            fatal("Could not specialize pointer: {}", ret_or_error.error());
        return ret_or_error.value();
    }
};

template<>
struct TypeGetter<void*> {
    std::shared_ptr<ObjectType> operator()()
    {
        return ObjectType::get("pointer");
    }
};

template<typename T>
std::shared_ptr<ObjectType> get_type()
{
    return TypeGetter<T> {}();
}

template<>
struct Converter<ObjectType> {
    static std::string to_string(ObjectType const& val)
    {
        return val.to_string();
    }

    static double to_double(ObjectType const&)
    {
        return NAN;
    }

    static long to_long(ObjectType const&)
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
            ret ^= (std::hash<std::string> {}(template_arg.first) ^ std::hash<Obelix::TemplateArgument> {}(template_arg.second)) << 1;
        }
        return ret;
    }
};
