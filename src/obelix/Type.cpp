/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

#include <core/Logging.h>
#include <core/StringUtil.h>
#include <obelix/Architecture.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Intrinsics.h>
#include <obelix/Type.h>
#include <variant>

namespace Obelix {

logging_category(type);

std::optional<PrimitiveType> PrimitiveType_by_name(std::string const& t)
{
    if (t == "int")
        return PrimitiveType::Int;
    if (t == "bool")
        return PrimitiveType::Boolean;
    if (t == "ptr")
        return PrimitiveType::Pointer;
    if (t == "array")
        return PrimitiveType::Array;
    return {};
}

size_t hash(TemplateArgumentValue const& arg)
{
    if (std::holds_alternative<long>(arg))
        return std::hash<long> {}(std::get<long>(arg));
    if (std::holds_alternative<std::string>(arg))
        return std::hash<std::string> {}(std::get<std::string>(arg));
    if (std::holds_alternative<std::shared_ptr<ObjectType>>(arg))
        return std::hash<std::shared_ptr<ObjectType>> {}(std::get<std::shared_ptr<ObjectType>>(arg));
    if (std::holds_alternative<bool>(arg))
        return std::hash<bool> {}(std::get<bool>(arg));
    assert(std::holds_alternative<NVP>(arg));
    auto nvp = std::get<NVP>(arg);
    return std::hash<std::string> {}(nvp.first) ^ std::hash<long> {}(nvp.second);
}

std::string to_string(TemplateArgumentValue const& arg)
{
    if (std::holds_alternative<long>(arg))
        return Obelix::to_string(std::get<long>(arg));
    if (std::holds_alternative<std::string>(arg))
        return std::get<std::string>(arg);
    if (std::holds_alternative<std::shared_ptr<ObjectType>>(arg))
        return std::get<std::shared_ptr<ObjectType>>(arg)->to_string();
    if (std::holds_alternative<bool>(arg))
        return Obelix::to_string(std::get<bool>(arg));
    assert(std::holds_alternative<NVP>(arg));
    auto nvp = std::get<NVP>(arg);
    return format("{}={}", nvp.first, nvp.second);
}

bool compare(TemplateArgumentValue const& arg1, TemplateArgumentValue const& arg2)
{
    if (arg1.index() != arg2.index())
        return false;
    if (arg1.valueless_by_exception() != arg2.valueless_by_exception())
        return false;
    if (std::holds_alternative<std::shared_ptr<ObjectType>>(arg1))
        return *std::get<std::shared_ptr<ObjectType>>(arg1) == *std::get<std::shared_ptr<ObjectType>>(arg2);
    return arg1 == arg2;
}

size_t TemplateArgument::hash() const
{
    size_t ret = std::hash<int> {}(static_cast<int>(parameter_type));
    for (auto const& arg : value) {
        ret ^= Obelix::hash(arg);
    }
    return ret;
}

std::string TemplateArgument::to_string() const
{
    if (multiplicity == TemplateParameterMultiplicity::Multiple) {
        std::string ret = "[ ";
        for (auto const& arg : value) {
            ret += Obelix::to_string(arg);
            ret += ' ';
        }
        return ret + "]";
    }
    if (!value.empty())
        return Obelix::to_string(value[0]);
    return "";
}

bool TemplateArgument::operator==(TemplateArgument const& other) const
{
    if (parameter_type != other.parameter_type || multiplicity != other.multiplicity)
        return false;
    if (value.size() != other.value.size())
        return false;
    for (auto ix = 0u; ix < value.size(); ++ix) {
        if (!compare(value[ix], other.value[ix]))
            return false;
    }
    return true;
}

MethodParameter::MethodParameter(char const* n, PrimitiveType t)
    : name(n)
    , type(ObjectType::get(t))
{
}

MethodParameter::MethodParameter(char const* n, std::shared_ptr<ObjectType> t)
    : name(n)
    , type(move(t))
{
}

MethodDescription::MethodDescription(char const* name, PrimitiveType type, IntrinsicType intrinsic, MethodParameters parameters, bool pure)
    : m_name(name)
    , m_is_operator(false)
    , m_is_pure(pure)
    , m_return_type(ObjectType::get(type))
    , m_varargs(false)
    , m_parameters(move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

MethodDescription::MethodDescription(char const* name, std::shared_ptr<ObjectType> type, IntrinsicType intrinsic, MethodParameters parameters, bool pure)
    : m_name(name)
    , m_is_operator(false)
    , m_is_pure(pure)
    , m_return_type(move(type))
    , m_varargs(false)
    , m_parameters(move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

MethodDescription::MethodDescription(Operator op, PrimitiveType type, IntrinsicType intrinsic, MethodParameters parameters, bool pure)
    : m_operator(op)
    , m_is_operator(true)
    , m_is_pure(pure)
    , m_return_type(ObjectType::get(type))
    , m_varargs(false)
    , m_parameters(move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

MethodDescription::MethodDescription(Operator op, std::shared_ptr<ObjectType> type, IntrinsicType intrinsic, MethodParameters parameters, bool pure)
    : m_operator(op)
    , m_is_operator(true)
    , m_is_pure(pure)
    , m_return_type(move(type))
    , m_varargs(false)
    , m_parameters(move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

void MethodDescription::set_default_implementation(std::string const& native_function)
{
    m_default_implementation.is_intrinsic = false;
    m_default_implementation.native_function = native_function;
    m_default_implementation.intrinsic = IntrinsicType::NotIntrinsic;
}

void MethodDescription::set_default_implementation(IntrinsicType intrinsic)
{
    assert(intrinsic > IntrinsicType::NotIntrinsic && intrinsic < IntrinsicType::count);
    m_default_implementation.is_intrinsic = true;
    m_default_implementation.intrinsic = intrinsic;
    m_default_implementation.native_function = "";
}

void MethodDescription::set_implementation(Architecture arch, IntrinsicType intrinsic)
{
    MethodImpl impl { .is_intrinsic = false, .intrinsic = intrinsic };
    m_implementations[arch] = impl;
}

void MethodDescription::set_implementation(Architecture arch, std::string const& native_function)
{
    MethodImpl impl { .is_intrinsic = false, .native_function = native_function };
    m_implementations[arch] = impl;
}

std::shared_ptr<BoundIntrinsicDecl> MethodDescription::declaration() const
{
    auto ident = make_node<BoundIdentifier>(Token {}, std::string(name()), return_type());
    BoundIdentifiers params;
    params.push_back(make_node<BoundIdentifier>(Token {}, std::string("this"), method_of()));
    for (auto const& p : parameters()) {
        params.push_back(make_node<BoundIdentifier>(Token {}, p.name, p.type));
    }
    return make_node<BoundIntrinsicDecl>(ident, params);;
}


MethodImpl const& MethodDescription::implementation(Architecture arch) const
{
    if (m_implementations.contains(arch))
        return m_implementations.at(arch);
    return m_default_implementation;
}

MethodImpl const& MethodDescription::implementation() const
{
    return m_default_implementation;
}

FieldDef::FieldDef(std::string n, PrimitiveType t)
    : name(move(n))
    , type(ObjectType::get(t))
{
}

FieldDef::FieldDef(std::string n, std::shared_ptr<ObjectType> t)
    : name(move(n))
    , type(move(t))
{
}

std::unordered_map<PrimitiveType, std::shared_ptr<ObjectType>> ObjectType::s_types_by_id {};
std::unordered_map<std::string, std::shared_ptr<ObjectType>> ObjectType::s_types_by_name {};
std::vector<std::shared_ptr<ObjectType>> ObjectType::s_template_specializations {};

[[maybe_unused]] std::shared_ptr<ObjectType> s_self;
[[maybe_unused]] std::shared_ptr<ObjectType> s_argument;
[[maybe_unused]] std::shared_ptr<ObjectType> s_compatible;
[[maybe_unused]] std::shared_ptr<ObjectType> s_assignable_to;
[[maybe_unused]] std::shared_ptr<ObjectType> s_unknown;
[[maybe_unused]] std::shared_ptr<ObjectType> s_incrementable;
[[maybe_unused]] std::shared_ptr<ObjectType> s_comparable;
[[maybe_unused]] std::shared_ptr<ObjectType> s_boolean;
[[maybe_unused]] std::shared_ptr<ObjectType> s_integer_number;
[[maybe_unused]] std::shared_ptr<ObjectType> s_signed_integer_number;
[[maybe_unused]] std::shared_ptr<ObjectType> s_integer;
[[maybe_unused]] std::shared_ptr<ObjectType> s_unsigned;
[[maybe_unused]] std::shared_ptr<ObjectType> s_long;
[[maybe_unused]] std::shared_ptr<ObjectType> s_ulong;
[[maybe_unused]] std::shared_ptr<ObjectType> s_word;
[[maybe_unused]] std::shared_ptr<ObjectType> s_uword;
[[maybe_unused]] std::shared_ptr<ObjectType> s_byte;
[[maybe_unused]] std::shared_ptr<ObjectType> s_char;
[[maybe_unused]] std::shared_ptr<ObjectType> s_float;
[[maybe_unused]] std::shared_ptr<ObjectType> s_null;
[[maybe_unused]] std::shared_ptr<ObjectType> s_pointer;
[[maybe_unused]] std::shared_ptr<ObjectType> s_array;
[[maybe_unused]] std::shared_ptr<ObjectType> s_string;
[[maybe_unused]] std::shared_ptr<ObjectType> s_enum;
[[maybe_unused]] std::shared_ptr<ObjectType> s_type;
[[maybe_unused]] std::shared_ptr<ObjectType> s_any;

static void initialize_types()
{
    static int s_initialized = 0;
    if (s_initialized > 0)
        return;
    s_initialized = 1;
    s_self = ObjectType::register_type(PrimitiveType::Self);
    s_argument = ObjectType::register_type(PrimitiveType::Argument);
    s_compatible = ObjectType::register_type(PrimitiveType::Compatible);
    s_assignable_to = ObjectType::register_type(PrimitiveType::AssignableTo);
    s_unknown = ObjectType::register_type(PrimitiveType::Unknown);
    s_type = ObjectType::register_type(PrimitiveType::Type);

    s_incrementable = ObjectType::register_type(PrimitiveType::Incrementable,
        [](std::shared_ptr<ObjectType> type) {
            type->add_method(MethodDescription { Operator::UnaryIncrement, PrimitiveType::Self });
            type->add_method(MethodDescription { Operator::UnaryDecrement, PrimitiveType::Self });
            type->add_method(MethodDescription { Operator::BinaryIncrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
            type->add_method(MethodDescription { Operator::BinaryDecrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
        });

    s_boolean = ObjectType::register_type(PrimitiveType::Boolean,
        [](std::shared_ptr<ObjectType> type) {
            type->add_method(MethodDescription { Operator::LogicalInvert, PrimitiveType::Self, IntrinsicType::invert_bool, {}, true });
            type->add_method(MethodDescription { Operator::LogicalAnd, PrimitiveType::Self, IntrinsicType::and_bool_bool, { { "other", PrimitiveType::Boolean } }, true });
            type->add_method(MethodDescription { Operator::LogicalOr, PrimitiveType::Self, IntrinsicType::or_bool_bool, { { "other", PrimitiveType::Boolean } }, true });
            type->has_size(1);
        });

    s_comparable = ObjectType::register_type(PrimitiveType::Comparable,
        [](std::shared_ptr<ObjectType> type) {
            type->add_method(MethodDescription { Operator::Less, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method(MethodDescription { Operator::LessEquals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method(MethodDescription { Operator::Greater, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method(MethodDescription { Operator::GreaterEquals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } }, true });
        });

    s_integer_number = ObjectType::register_type(PrimitiveType::IntegerNumber,
        [](std::shared_ptr<ObjectType> type) {
            type->has_template_parameter({ "signed", TemplateParameterType::Boolean });
            type->has_template_parameter({ "size", TemplateParameterType::Integer });

            type->add_method(MethodDescription { Operator::Identity, PrimitiveType::Argument });

            type->add_method({ Operator::BitwiseInvert, PrimitiveType::Argument, IntrinsicType::invert_int, {}, true });
            type->add_method({ Operator::Add, PrimitiveType::Self, IntrinsicType::add_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method({ Operator::Add, PrimitiveType::Argument, IntrinsicType::add_int_int, { { "other", PrimitiveType::AssignableTo } }, true });
            type->add_method({ Operator::Subtract, PrimitiveType::Self, IntrinsicType::subtract_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method({ Operator::Subtract, PrimitiveType::Argument, IntrinsicType::subtract_int_int, { { "other", PrimitiveType::AssignableTo } }, true });
            type->add_method({ Operator::Multiply, PrimitiveType::Self, IntrinsicType::multiply_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method({ Operator::Multiply, PrimitiveType::Argument, IntrinsicType::multiply_int_int, { { "other", PrimitiveType::AssignableTo } }, true });
            type->add_method({ Operator::Divide, PrimitiveType::Self, IntrinsicType::divide_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method({ Operator::Divide, PrimitiveType::Self, IntrinsicType::divide_int_int, { { "other", PrimitiveType::AssignableTo } }, true });
            type->add_method({ Operator::BitwiseOr, PrimitiveType::Self, IntrinsicType::bitwise_or_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method({ Operator::BitwiseAnd, PrimitiveType::Self, IntrinsicType::bitwise_and_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method({ Operator::BitwiseXor, PrimitiveType::Self, IntrinsicType::bitwise_xor_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method({ Operator::BitShiftLeft, PrimitiveType::Self, IntrinsicType::shl_int, { { "other", ObjectType::get("u8") } }, true });
            type->add_method({ Operator::BitShiftRight, PrimitiveType::Self, IntrinsicType::shr_int, { { "other", ObjectType::get("u8") } }, true });
            type->add_method({ Operator::Equals, PrimitiveType::Boolean, IntrinsicType::equals_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method({ Operator::Less, PrimitiveType::Boolean, IntrinsicType::less_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method({ Operator::Greater, PrimitiveType::Boolean, IntrinsicType::greater_int_int, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method(MethodDescription { Operator::Range, PrimitiveType::Range, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } }, true });
            type->will_be_a(s_comparable);
            type->will_be_a(s_incrementable);
        });

    s_signed_integer_number = ObjectType::register_type(PrimitiveType::SignedIntegerNumber,
        [](std::shared_ptr<ObjectType> type) {
            type->has_template_parameter({ "signed", TemplateParameterType::Boolean });
            type->has_template_parameter({ "size", TemplateParameterType::Integer });

            type->add_method(MethodDescription { Operator::Negate, PrimitiveType::Self, IntrinsicType::negate_int, {}, true });
            type->will_be_a(s_integer_number);
        });

    s_integer = ObjectType::register_type("s32", s_signed_integer_number, TemplateArguments { { true }, { 4 } },
        [](std::shared_ptr<ObjectType> type) {
            type->has_alias("int");
            type->has_size(4);
        });

    s_unsigned = ObjectType::register_type("u32", s_integer_number, TemplateArguments { { false }, { 4 } },
        [](std::shared_ptr<ObjectType> type) {
            type->has_alias("uint");
            type->has_size(4);
        });

    s_long = ObjectType::register_type("s64", s_signed_integer_number, TemplateArguments { { true }, { 8 } },
        [](std::shared_ptr<ObjectType> type) {
            type->has_alias("long");
            type->has_size(8);
        });

    s_ulong = ObjectType::register_type("u64", s_integer_number, TemplateArguments { { false }, { 8 } },
        [](std::shared_ptr<ObjectType> type) {
            type->has_alias("ulong");
            type->has_size(8);
        });

    s_word = ObjectType::register_type("s16", s_signed_integer_number, TemplateArguments { { true }, { 2 } },
        [](std::shared_ptr<ObjectType> type) {
            type->has_alias("word");
            type->has_size(8);
        });

    s_uword = ObjectType::register_type("u16", s_integer_number, TemplateArguments { { false }, { 2 } },
        [](std::shared_ptr<ObjectType> type) {
            type->has_alias("uword");
            type->has_size(8);
        });

    s_byte = ObjectType::register_type("s8", s_signed_integer_number, TemplateArguments { { true }, { 1 } },
        [](std::shared_ptr<ObjectType> type) {
            type->has_alias("byte");
            type->has_size(1);
        });

    s_char = ObjectType::register_type("u8", s_integer_number, TemplateArguments { { false }, { 1 } },
        [](std::shared_ptr<ObjectType> type) {
            type->has_alias("char");
            type->has_size(1);
        });

    s_float = ObjectType::register_type(PrimitiveType::Float,
        [](std::shared_ptr<ObjectType> type) {
            type->add_method(MethodDescription { Operator::Identity, PrimitiveType::Self, IntrinsicType::NotIntrinsic, {}, true });
            type->add_method(MethodDescription { Operator::Negate, PrimitiveType::Self, IntrinsicType::NotIntrinsic, {}, true });
            type->add_method(MethodDescription { Operator::Add, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method(MethodDescription { Operator::Subtract, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method(MethodDescription { Operator::Multiply, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } }, true });
            type->add_method(MethodDescription { Operator::Divide, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } }, true });
            type->will_be_a(s_comparable);
            type->has_size(4);
        });

    s_null = ObjectType::register_type(PrimitiveType::Null,
        [](std::shared_ptr<ObjectType> type) {
        });

    s_pointer = ObjectType::register_type(PrimitiveType::Pointer,
        [](std::shared_ptr<ObjectType> type) {
            type->has_template_parameter({ "target", TemplateParameterType::Type });
            type->has_alias("ptr");
            type->has_size(8);
            type->add_method(MethodDescription { Operator::Dereference, ObjectType::get("u8") });
            type->add_method(MethodDescription { Operator::UnaryIncrement, PrimitiveType::Self });
            type->add_method(MethodDescription { Operator::UnaryDecrement, PrimitiveType::Self });
            type->add_method(MethodDescription { Operator::BinaryIncrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", ObjectType::get("u64") } } });
            type->add_method(MethodDescription { Operator::BinaryDecrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", ObjectType::get("u64") } } });
            type->add_method(MethodDescription { Operator::Add, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", ObjectType::get("u64") } } });
            type->add_method(MethodDescription { Operator::Subtract, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", ObjectType::get("u64") } } });
            type->will_be_a(s_comparable);

            type->has_template_stamp([](std::shared_ptr<ObjectType> instantiation) {
                instantiation->add_method(MethodDescription { Operator::Dereference, instantiation->template_arguments()[0].as_type() });
            });
        });

    s_array = ObjectType::register_type(PrimitiveType::Array,
        [](std::shared_ptr<ObjectType> type) {
            type->has_template_parameter({ "base_type", TemplateParameterType::Type });
            type->has_template_parameter({ "size", TemplateParameterType::Integer });
            type->has_size(8);

            type->has_template_stamp([](std::shared_ptr<ObjectType> instantiation) {
                instantiation->add_method(MethodDescription { Operator::Subscript, instantiation->template_arguments()[0].as_type(),
                    IntrinsicType::NotIntrinsic, { { "subscript", ObjectType::get("s32") } } });
                instantiation->has_size(instantiation->template_arguments()[1].as_integer() * instantiation->template_arguments()[0].as_type()->size());
            });
        });

    s_string = ObjectType::register_struct_type("string",
        FieldDefs {
            FieldDef { std::string("length"), s_unsigned },
            FieldDef { std::string("data"), ObjectType::specialize(s_pointer, { s_char }).value() } },
        [](std::shared_ptr<ObjectType> type) {
            type->add_method(MethodDescription { Operator::Add, PrimitiveType::Self, IntrinsicType::add_str_str, { { "other", PrimitiveType::Self } }, true });
            type->add_method(MethodDescription { Operator::Multiply, PrimitiveType::Self, IntrinsicType::multiply_str_int, { { "other", s_unsigned } }, true });
            type->will_be_a(s_comparable);
        });

    s_enum = ObjectType::register_type(PrimitiveType::Enum,
        [](std::shared_ptr<ObjectType> type) {
            type->has_template_parameter({ "values", TemplateParameterType::NameValue, TemplateParameterMultiplicity::Multiple });
            type->has_size(4);

            type->has_template_stamp([](std::shared_ptr<ObjectType> instantiation) {
                instantiation->add_method(MethodDescription { Operator::Subscript, s_long,
                    IntrinsicType::NotIntrinsic, { { "subscript", s_string } } });
            });
        });

    s_any = ObjectType::register_type(PrimitiveType::Any,
        [](std::shared_ptr<ObjectType> type) {
            type->add_method(MethodDescription { Operator::Assign, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
            type->add_method(MethodDescription { Operator::Equals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
            type->add_method(MethodDescription { Operator::NotEquals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
            type->add_method(MethodDescription { Operator::Dereference, PrimitiveType::Any, IntrinsicType::NotIntrinsic, { { "attribute", s_string } } });
            type->add_method(MethodDescription { "typename", s_string });
            type->add_method(MethodDescription { "length", s_unsigned });
            type->add_method(MethodDescription { "empty", PrimitiveType::Boolean });
        });
    s_initialized = 2;
}

std::string ObjectType::to_string() const
{
    return name();
}

MethodDescription& ObjectType::add_method(MethodDescription md)
{
    auto shared_ptr_type = ObjectType::get(name());
    md.set_method_of(shared_ptr_type);
    m_methods.push_back(md);
    return m_methods.back();
}

bool ObjectType::operator==(ObjectType const& other) const
{
    if (name() != other.name())
        return false;
    if (template_arguments().size() != other.template_arguments().size())
        return false;
    for (auto ix = 0u; ix < template_arguments().size(); ++ix) {
        if (template_arguments()[ix] != other.template_arguments()[ix])
            return false;
    }
    return true;
}

/*
 * is_assignable_to - is a value of this type assignable to the other type.
 *  - non-integers: types must be the same.
 *  - integers:
 *      -- signedness is the same and this size is less or equal to other
 *      -- signedness is different and this size is stritcly less than other
 */
bool ObjectType::is_assignable_to(ObjectType const& other) const
{
    if (type() == PrimitiveType::SignedIntegerNumber || type() == PrimitiveType::IntegerNumber) {
        if (type() == other.type()) {
            bool ret = size() <= other.size();
            debug(type, "{}.is_assignable_to({}) = {}", to_string(), other.to_string(), ret);
            return ret;
        }
        if (other.type() == PrimitiveType::IntegerNumber || other.type() == PrimitiveType::SignedIntegerNumber) {
            bool ret = size() < other.size();
            debug(type, "{}.is_assignable_to({}) = {}", to_string(), other.to_string(), ret);
            return ret;
        }
        return false;
    }
    return *this == other;
}

/*
 * is_assignable_to - is a value of other type assignable to this type.
 *  - non-integers: types must be the same.
 *  - integers:
 *      -- signedness is the same and this other size is less or equal this size
 *      -- signedness is different and other size is stritcly less than this size
 */
bool ObjectType::is_compatible_with(ObjectType const& other) const
{
    if (type() == PrimitiveType::SignedIntegerNumber || type() == PrimitiveType::IntegerNumber) {
        if (type() == other.type()) {
            bool ret = other.size() <= size();
            debug(type, "{}.is_compatible_with({}) = {}", to_string(), other.to_string(), ret);
            return ret;
        }
        if (other.type() == PrimitiveType::IntegerNumber || other.type() == PrimitiveType::SignedIntegerNumber) {
            bool ret = other.size() < size();
            debug(type, "{}.is_compatible_with({}) = {}", to_string(), other.to_string(), ret);
            return ret;
        }
        return false;
    }
    return *this == other;
}

size_t ObjectType::size() const
{
    if (m_type != PrimitiveType::Struct)
        return m_size;
    size_t ret = 0;
    for (auto const& field : m_fields) {
        ret += field.type->size();
    }
    return ret;
}

ssize_t ObjectType::offset_of(std::string const& name) const
{
    if (m_type != PrimitiveType::Struct)
        return (size_t) -1;
    size_t ret = 0;
    for (auto const& field : m_fields) {
        if (field.name == name)
            return ret;
        ret += field.type->size();
    }
    return (size_t) -1;
}

ssize_t ObjectType::offset_of(size_t field) const
{
    if ((field < 0) || (field >= m_fields.size()))
        return (size_t) -1;
    size_t ret = 0;
    for (auto ix = 0u; ix < field; ix++) {
        ret += m_fields[ix].type->size();
    }
    return ret;
}

FieldDef const& ObjectType::field(std::string const& name) const
{
    static FieldDef unknown { "", ObjectType::get(PrimitiveType::Unknown) };
    if (m_type != PrimitiveType::Struct)
        return unknown;
    for (auto const& field : m_fields) {
        if (field.name == name)
            return field;
    }
    return unknown;
}

bool ObjectType::is_a(ObjectType const* other) const
{
    if ((*other == *this) || (other->type() == PrimitiveType::Any))
        return true;
    return std::any_of(m_is_a.begin(), m_is_a.end(), [&other](auto& super_type) { return super_type->is_a(other); });
}

bool ObjectType::has_template_argument(std::string const& arg) const
{
    if (!is_template_specialization())
        return false;
    for (auto ix = 0u; ix < m_specializes_template->template_parameters().size(); ++ix) {
        auto param = m_specializes_template->template_parameters()[ix];
        if (param.name == arg)
            return true;
    }
    return false;
}

bool ObjectType::is_compatible(MethodDescription const& mth, ObjectTypes const& argument_types) const
{
    if (mth.parameters().size() != argument_types.size())
        return false;
    auto ix = 0u;
    for (; ix < mth.parameters().size(); ++ix) {
        auto const& param = mth.parameters()[ix];
        auto const& arg_type = argument_types[ix];
        switch (param.type->type()) {
            case PrimitiveType::Self:
                if (*arg_type != *this)
                    return false;
                break;
            case PrimitiveType::Compatible:
                if (!is_compatible_with(arg_type))
                    return false;
                break;
            case PrimitiveType::AssignableTo:
                if (!is_assignable_to(arg_type))
                    return false;
                break;
            default:
                if (*param.type != *arg_type)
                    return false;
                break;
        }
    }
    return (ix == mth.parameters().size());
}

std::optional<std::shared_ptr<ObjectType>> ObjectType::return_type_of(std::string_view method_name, ObjectTypes const& argument_types) const
{
    auto check_methods_of = [&method_name, &argument_types, this](ObjectType const* type) -> std::shared_ptr<ObjectType> const& {
        for (auto& mth : type->m_methods) {
            if (mth.is_operator())
                continue;
            if (mth.name() != method_name)
                continue;
            if (!is_compatible(mth, argument_types))
                continue;
            if (*(mth.return_type()) == *s_self)
                return ObjectType::get(this);
            if (*(mth.return_type()) == *s_argument)
                return argument_types[0];
            return mth.return_type();
        }
        return s_unknown;
    };

    auto self = get(this);
    ObjectTypes types { s_any, self };
    while (!types.empty()) {
        auto type = types.back();
        types.pop_back();
        for (auto& is_a : type->m_is_a) {
            types.push_back(is_a);
        }
        if (type->is_template_specialization())
            types.push_back(type->specializes_template());
        if (auto ret = check_methods_of(type.get()); *ret != *s_unknown)
            return ret;
    }
    return {};
}

std::optional<std::shared_ptr<ObjectType>> ObjectType::return_type_of(Operator op, ObjectTypes const& argument_types) const
{
    auto check_operators_of = [&op, &argument_types, this](ObjectType const* type) -> std::shared_ptr<ObjectType> const& {
        for (auto& mth : type->m_methods) {
            if (!mth.is_operator())
                continue;
            if (mth.op() != op)
                continue;
            if (!is_compatible(mth, argument_types)) {
                debug(type, "Found operator but incompatible argument types");
                continue;
            }
            if (mth.return_type()->type() == PrimitiveType::Self)
                return ObjectType::get(this);
            if (mth.return_type()->type() == PrimitiveType::Argument)
                return argument_types[0];
            return mth.return_type();
        }
        return s_unknown;
    };

    std::string s = "";
    for (auto const& arg : argument_types) {
        s += ",";
        s += arg->to_string();
    }
    debug(type, "{}::return_type_of({}{})", this->to_string(), op, s);
    auto self = get(this);
    ObjectTypes types { s_any, self };
    while (!types.empty()) {
        auto type = types.back();
        types.pop_back();
        for (auto& is_a : type->m_is_a) {
            types.push_back(is_a);
        }
        if (type->is_template_specialization())
            types.push_back(type->specializes_template());
        debug(type, "Checking operators of type {}", type->to_string());
        if (auto ret = check_operators_of(type.get()); *ret != *s_unknown) {
            debug(type, "Return type is {}", ret->to_string());
            return ret;
        }
    }
    debug(type, "No matching operator found");
    return {};
}

std::optional<MethodDescription> ObjectType::get_method(Operator op) const
{
    auto check_operators_of = [&op, this](ObjectType const* type) -> std::optional<MethodDescription> {
        for (auto& mth : type->m_methods) {
            if (!mth.is_operator())
                continue;
            if (mth.op() != op)
                continue;
            if (*(mth.return_type()) == *s_self) {
                auto ret = mth;
                ret.set_return_type(ObjectType::get(this));
                return ret;
            }
            return mth;
        }
        return {};
    };

    debug(type, "{}::get_method({})", this->to_string(), op);
    auto self = get(this);
    ObjectTypes types { s_any, self };
    while (!types.empty()) {
        auto type = types.back();
        types.pop_back();
        for (auto& is_a : type->m_is_a) {
            types.push_back(is_a);
        }
        if (type->is_template_specialization())
            types.push_back(type->specializes_template());
        debug(type, "Checking operators of type {}", type->to_string());
        if (auto ret = check_operators_of(type.get()); ret.has_value()) {
            debug(type, "Return method is {}", ret.value().name());
            return ret;
        }
    }
    debug(type, "No matching operator found");
    return {};
}

std::optional<MethodDescription> ObjectType::get_method(Operator op, ObjectTypes const& argument_types) const
{
    auto check_operators_of = [&op, &argument_types, this](ObjectType const* type) -> std::optional<MethodDescription> {
        for (auto& mth : type->m_methods) {
            if (!mth.is_operator())
                continue;
            if (mth.op() != op)
                continue;
            if (!is_compatible(mth, argument_types)) {
                debug(type, "Found operator but incompatible argument types");
                continue;
            }
            if (*(mth.return_type()) == *s_self) {
                auto ret = mth;
                ret.set_return_type(ObjectType::get(this));
                return ret;
            }
            if (*(mth.return_type()) == *s_argument) {
                auto ret = mth;
                ret.set_return_type(argument_types[0]);
                return ret;
            }
            return mth;
        }
        return {};
    };

    debug(type, "{}::get_method({})", this->to_string(), op);
    auto self = get(this);
    ObjectTypes types { s_any, self };
    while (!types.empty()) {
        auto type = types.back();
        types.pop_back();
        for (auto& is_a : type->m_is_a) {
            types.push_back(is_a);
        }
        if (type->is_template_specialization())
            types.push_back(type->specializes_template());
        debug(type, "Checking operators of type {}", type->to_string());
        if (auto ret = check_operators_of(type.get()); ret.has_value()) {
            debug(type, "Return method is {}", ret.value().name());
            return ret;
        }
    }
    debug(type, "No matching operator found");
    return {};
}

std::shared_ptr<ObjectType> const& ObjectType::get(PrimitiveType type)
{
    initialize_types();
    debug(type, "ObjectType::get({}: PrimitiveType)", type);
    if (!s_types_by_id.contains(type)) {
        ObjectType::register_type(type,
            [](std::shared_ptr<ObjectType> type) {
            });
    }
    return s_types_by_id.at(type);
}

std::shared_ptr<ObjectType> const& ObjectType::get(std::string const& type)
{
    initialize_types();
    debug(type, "ObjectType::get({}: std::string)", type);
    if (s_types_by_name.contains(type))
        return s_types_by_name.at(type);
    auto type_maybe = PrimitiveType_by_name(type);
    if (type_maybe.has_value() && s_types_by_id.contains(type_maybe.value())) {
        auto ret = s_types_by_id.at(type_maybe.value());
        s_types_by_name[type] = ret;
        return s_types_by_name[type];
    }
    return s_unknown;
}

std::shared_ptr<ObjectType> const& ObjectType::get(ObjectType const* type)
{
    initialize_types();
    debug(type, "ObjectType::get({}: ObjectType*)", *type);
    if (!type->is_template_specialization())
        return get(type->name());

    for (auto& instantiation : s_template_specializations) {
        if (*type == *instantiation) {
            return instantiation;
        }
    }
    return s_unknown;
}

std::shared_ptr<ObjectType> ObjectType::register_type(PrimitiveType type, ObjectTypeBuilder const& builder) noexcept
{
    return register_type(type, PrimitiveType_name(type), builder);
}

std::shared_ptr<ObjectType> ObjectType::register_type(PrimitiveType type, char const* name, ObjectTypeBuilder const& builder) noexcept
{
    initialize_types();
    debug(type, "Registering primitive type {}", name);
    auto ptr = std::make_shared<ObjectType>(type, name);
    register_type_in_caches(ptr);
    if (builder != nullptr)
        builder(ptr);
    return ptr;
}

std::shared_ptr<ObjectType> ObjectType::register_type(char const* name, std::shared_ptr<ObjectType> specialization_of, TemplateArguments const& template_args, ObjectTypeBuilder const& builder) noexcept
{
    initialize_types();
    debug(type, "Registering {} as specialization of {} with arguments {}", name, specialization_of, template_args);
    auto ret_or_error = ObjectType::specialize(specialization_of, move(template_args));
    if (ret_or_error.is_error())
        fatal("specialize '{}' failed: {}", name, ret_or_error.error());
    auto type = ret_or_error.value();
    type->m_name = name;
    type->m_name_str = name;
    register_type_in_caches(type);
    if (builder)
        builder(type);
    return type;
}

std::shared_ptr<ObjectType> ObjectType::register_struct_type(std::string const& name, FieldDefs fields, ObjectTypeBuilder const& builder)
{
    auto ret_or_error = make_struct_type(name, move(fields), builder);
    if (ret_or_error.is_error()) {
        fatal("Could not register struct type '{}': {}", name, ret_or_error.error());
    }
    return ret_or_error.value();
}

ErrorOr<std::shared_ptr<ObjectType>> ObjectType::specialize(std::shared_ptr<ObjectType> const& base_type, TemplateArguments const& template_args)
{
    if (base_type->is_parameterized() && (template_args.size() != base_type->template_parameters().size()))
        return Error<int> { ErrorCode::TemplateParameterMismatch, base_type, base_type->template_parameters().size(), template_args.size() };
    if (!base_type->is_parameterized() && !template_args.empty())
        return Error<int> { ErrorCode::TypeNotParameterized, base_type };
    if (!base_type->is_parameterized())
        return base_type;
    for (auto& template_specialization : s_template_specializations) {
        if (template_specialization->specializes_template() == base_type) {
            size_t ix;
            for (ix = 0; ix < template_args.size(); ++ix) {
                if (template_specialization->template_arguments()[ix] != template_args[ix])
                    break;
            }
            if (ix >= template_args.size())
                return template_specialization;
        }
    }
    debug(type, "Specializing {} with arguments {}", base_type, template_args);
    auto specialization = register_type(base_type->type(), format("{}{}", base_type->name(), template_args).c_str(),
        [&template_args, &base_type](std::shared_ptr<ObjectType> new_type) {
            new_type->m_specializes_template = base_type;
            new_type->m_template_arguments = template_args;
            if (base_type->m_stamp) {
                base_type->m_stamp(new_type);
            }
        });
    s_template_specializations.push_back(specialization);
    return specialization;
}

ErrorOr<std::shared_ptr<ObjectType>> ObjectType::specialize(std::string const& base_type_name, TemplateArguments const& template_args)
{
    auto base_type = ObjectType::get(base_type_name);
    if (base_type == nullptr || *base_type == *s_unknown)
        return Error<int> { ErrorCode::NoSuchType, base_type_name };
    return specialize(base_type, template_args);
}

ErrorOr<std::shared_ptr<ObjectType>> ObjectType::make_struct_type(std::string name, FieldDefs fields, ObjectTypeBuilder const& builder)
{
    debug(type, "Making struct {}", name);
    initialize_types();
    std::shared_ptr<ObjectType> ret;
    if (s_types_by_name.contains(name)) {
        auto existing = s_types_by_name.at(name);
        if (existing->type() != PrimitiveType::Struct)
            return Error<int> { ErrorCode::DuplicateTypeName, name };
        if (existing->fields().size() != fields.size())
            return Error<int> { ErrorCode::DuplicateTypeName, name };
        for (auto ix = 0u; ix < fields.size(); ++ix) {
            auto existing_field = existing->fields()[ix];
            auto new_field = fields[ix];
            if (*existing_field.type != *new_field.type || existing_field.name != new_field.name)
                return Error<int> { ErrorCode::DuplicateTypeName, name };
        }
        return existing;
    }
    assert(!fields.empty()); // TODO return proper error
    ret = std::make_shared<ObjectType>(PrimitiveType::Struct, move(name));
    register_type_in_caches(ret);
    ret->m_fields = move(fields);
    size_t sz= 0;
    for (auto const& f : ret->m_fields) {
        sz += f.type->size();
    }
    ret->has_size(sz);

    if (builder != nullptr)
        builder(ret);
    return ret;
}

std::shared_ptr<ObjectType> ObjectType::make_enum_type(std::string name, NVPs values)
{
    initialize_types();
    TemplateArgumentValues arg_values;
    for (auto const& nvp : values) {
        arg_values.push_back(nvp);
    }
    TemplateArguments args { TemplateArgument(TemplateParameterType::NameValue, arg_values) };
    return register_type(name.c_str(), s_enum, args);
}

void ObjectType::register_type_in_caches(std::shared_ptr<ObjectType> const& type)
{
    if (!type->is_template_specialization() && !s_types_by_id.contains(type->type()))
        s_types_by_id[type->type()] = type;
    s_types_by_name[type->name()] = type;
    for (auto const& alias : type->m_aliases) {
        s_types_by_name[alias] = type;
    }
}

void ObjectType::dump()
{
    auto dump_type = [](std::shared_ptr<ObjectType> const& type) {
        return format("to_string: {} name: '{}', primitive type: {}, is_specialization: {}",
            type->to_string(), type->name(), type->type(), type->is_template_specialization());
    };

    for (auto const& t : s_types_by_name) {
        std::cout << format("{}: {}", t.first, dump_type(t.second)) << "\n";
    }
}

}
