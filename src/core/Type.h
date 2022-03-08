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

#include <core/Error.h>
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

#define ENUMERATE_BINARY_OPERATORS(S) \
    S(Invalid, false, -1)             \
    S(Add, false, 11)                 \
    S(Subtract, false, 11)            \
    S(Multiply, false, 12)            \
    S(Divide, false, 12)              \
    S(Modulo, false, 12)              \
    S(Assign, true, 1)                \
    S(Equals, false, 8)               \
    S(NotEquals, false, 8)            \
    S(GreaterEquals, false, 9)        \
    S(LessEquals, false, 9)           \
    S(Greater, false, 9)              \
    S(Less, false, 9)                 \
    S(LogicalAnd, false, 4)           \
    S(LogicalOr, false, 3)            \
    S(BitwiseAnd, false, 7)           \
    S(BitwiseOr, false, 5)            \
    S(BitwiseXor, false, 6)           \
    S(BinaryIncrement, true, 1)       \
    S(BinaryDecrement, true, 1)       \
    S(Dereference, false, 14)         \
    S(BitShiftLeft, false, 10)        \
    S(BitShiftRight, false, 10)       \
    S(AssignShiftLeft, true, 1)       \
    S(AssignShiftRight, true, 1)      \
    S(AssignBitwiseAnd, true, 1)      \
    S(AssignBitwiseOr, true, 1)       \
    S(AssignBitwiseXor, true, 1)      \
    S(Range, false, 8)

#define ENUMERATE_UNARY_OPERATORS(S) \
    S(InvalidUnary)                  \
    S(Identity)                      \
    S(Negate)                        \
    S(UnaryIncrement)                \
    S(UnaryDecrement)                \
    S(LogicalInvert)                 \
    S(BitwiseInvert)

enum class Operator {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) op,
    ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) op,
        ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR
};

enum class BinaryOperator {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) op,
    ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
};

constexpr Operator to_operator(BinaryOperator op)
{
    switch (op) {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) \
    case BinaryOperator::op:        \
        return Operator::op;
        ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
    }
    return Operator::Invalid;
}

#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) constexpr char const* Binary_##op = #op;
ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR

constexpr char const* BinaryOperator_name(BinaryOperator op)
{
    switch (op) {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) \
    case BinaryOperator::op:        \
        return #op;
        ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
    }
}

constexpr bool BinaryOperator_is_assignment(BinaryOperator op)
{
    switch (op) {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, assignment_op, p) \
    case BinaryOperator::op:                    \
        return assignment_op;
        ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
    }
}

constexpr BinaryOperator BinaryOperator_for_assignment_operator(BinaryOperator op)
{
    switch (op) {
    case BinaryOperator::BinaryIncrement:
        return BinaryOperator::Add;
    case BinaryOperator::BinaryDecrement:
        return BinaryOperator::Subtract;
    case BinaryOperator::AssignShiftLeft:
        return BinaryOperator::BitShiftLeft;
    case BinaryOperator::AssignShiftRight:
        return BinaryOperator::BitShiftRight;
    case BinaryOperator::AssignBitwiseAnd:
        return BinaryOperator::BitwiseAnd;
    case BinaryOperator::AssignBitwiseOr:
        return BinaryOperator::BitwiseOr;
    case BinaryOperator::AssignBitwiseXor:
        return BinaryOperator::BitwiseXor;
    default:
        return op;
    }
}

constexpr int BinaryOperator_precedence(BinaryOperator op)
{
    switch (op) {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, precedence) \
    case BinaryOperator::op:                 \
        return precedence;
        ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
    }
}

template<>
struct Converter<BinaryOperator> {
    static std::string to_string(BinaryOperator val)
    {
        return BinaryOperator_name(val);
    }

    static double to_double(BinaryOperator val)
    {
        return static_cast<double>(val);
    }

    static long to_long(BinaryOperator val)
    {
        return static_cast<long>(val);
    }
};

enum class UnaryOperator {
#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) op,
    ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR
};

#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) constexpr char const* Unary_##op = #op;
ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR

constexpr Operator to_operator(UnaryOperator op)
{
    switch (op) {
#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) \
    case UnaryOperator::op:  \
        return Operator::op;
        ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR
    }
    return Operator::Invalid;
}

constexpr char const* UnaryOperator_name(UnaryOperator op)
{
    switch (op) {
#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) \
    case UnaryOperator::op:  \
        return #op;
        ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR
    }
}

template<>
struct Converter<UnaryOperator> {
    static std::string to_string(UnaryOperator val)
    {
        return UnaryOperator_name(val);
    }

    static double to_double(UnaryOperator val)
    {
        return static_cast<double>(val);
    }

    static long to_long(UnaryOperator val)
    {
        return static_cast<long>(val);
    }
};

class ObjectType;
using ObjectTypes = std::vector<std::shared_ptr<ObjectType>>;

struct MethodParameter {
    MethodParameter(char const* n, ObelixType);
    MethodParameter(char const* n, std::shared_ptr<ObjectType>);

    char const* name;
    std::shared_ptr<ObjectType> type;
};

typedef std::vector<MethodParameter> MethodParameters;

class MethodDescription {
public:
    MethodDescription(char const* name, std::shared_ptr<ObjectType> type)
        : m_name(name)
        , m_is_operator(false)
        , m_return_type(move(type))
        , m_varargs(false)
    {
    }

    MethodDescription(char const* name, ObelixType type);

    template<typename... Args>
    MethodDescription(char const* name, ObelixType type, Args&&... args)
        : MethodDescription(name, type)
    {
        add_parameters(std::forward<Args>(args)...);
    }

    MethodDescription(Operator op, std::shared_ptr<ObjectType> type)
        : m_operator(op)
        , m_is_operator(true)
        , m_return_type(move(type))
        , m_varargs(false)
    {
    }

    MethodDescription(Operator op, ObelixType type);

    template<typename... Args>
    MethodDescription(Operator op, ObelixType type, Args&&... args)
        : MethodDescription(op, type)
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
    [[nodiscard]] Operator op() const { return m_operator; }
    [[nodiscard]] std::shared_ptr<ObjectType> const& return_type() const { return m_return_type; }
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
    bool m_varargs;
    MethodParameters m_parameters;
};

typedef std::vector<MethodDescription> MethodDescriptions;

class ObjectType {
public:
    ObjectType(ObelixType type, const char* name) noexcept
        : m_type(type)
        , m_name(name)
    {
    }

    template<typename ObjectTypeBuilder>
    ObjectType(ObelixType type, ObjectTypeBuilder const& builder) noexcept
        : m_type(type)
    {
        m_name_str = std::string(ObelixType_name(type));
        builder(*this);
    }

    template<typename ObjectTypeBuilder>
    ObjectType(ObelixType type, char const* name, ObjectTypeBuilder const& builder) noexcept
        : m_type(type)
        , m_name(name)
    {
        if (m_name)
            m_name_str = std::string(m_name);
        else
            m_name_str = std::string(ObelixType_name(type));
        builder(*this);
    }

    [[nodiscard]] ObelixType type() const { return m_type; }
    [[nodiscard]] std::string const& name() const
    {
        return m_name_str;
    }

    [[nodiscard]] std::string to_string() const;

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

    void will_be_a(ObelixType type) { m_is_a.push_back(ObjectType::get(type)); }
    void has_template_parameter(std::string const& parameter) { m_template_parameters.push_back(parameter); }
    void has_size(size_t sz) { m_size = sz; }

    [[nodiscard]] bool is_a(ObjectType const* other) const;
    [[nodiscard]] bool is_a(std::shared_ptr<ObjectType> other) const { return is_a(other.get()); }
    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(std::string_view method_name, ObjectTypes const& argument_types = {}) const;

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

    [[nodiscard]] std::optional<std::shared_ptr<ObjectType>> return_type_of(Operator op, ObjectTypes const& argument_types = {}) const;

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

    [[nodiscard]] bool is_parameterized() const { return !m_template_parameters.empty(); }
    [[nodiscard]] size_t size() const { return m_size; }
    [[nodiscard]] std::vector<std::string> const& template_parameters() const { return m_template_parameters; }

    [[nodiscard]] bool is_template_instantiation() const { return m_instantiates_template != nullptr; }
    [[nodiscard]] std::shared_ptr<ObjectType> instatiates_template() const { return m_instantiates_template; }
    [[nodiscard]] ObjectTypes const& template_arguments() const { return m_template_arguments; }

    bool operator==(ObjectType const&) const;

    static std::shared_ptr<ObjectType> register_type(ObelixType type) noexcept
    {
        auto ptr = std::make_shared<ObjectType>(type, ObelixType_name(type));
        s_types_by_id[type] = ptr;
        s_types_by_name[ptr->name()] = ptr;
        return ptr;
    }

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

    static std::shared_ptr<ObjectType> const& get(ObelixType);
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
    bool is_compatible(MethodDescription const&, ObjectTypes const&) const;

    ObelixType m_type { TypeUnknown };
    char const* m_name { nullptr };
    std::string m_name_str;
    size_t m_size { 8 };
    MethodDescriptions m_methods {};
    std::vector<std::shared_ptr<ObjectType>> m_is_a;
    std::vector<std::string> m_template_parameters {};
    std::shared_ptr<ObjectType> m_instantiates_template { nullptr };
    std::vector<std::shared_ptr<ObjectType>> m_template_arguments {};
    static std::unordered_map<ObelixType, std::shared_ptr<ObjectType>> s_types_by_id;
    static std::unordered_map<std::string, std::shared_ptr<ObjectType>> s_types_by_name;
    static std::vector<std::shared_ptr<ObjectType>> s_template_instantiations;
};

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
