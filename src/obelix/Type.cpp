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

size_t TemplateArgument::hash() const
{
    size_t ret = std::hash<int> {}(static_cast<int>(parameter_type));
    if (multiplicity == TemplateParameterMultiplicity::Multiple) {
        for (auto const& arg : as_arguments()) {
            ret ^= arg.hash();
        }
        return ret;
    }
    switch (parameter_type) {
        case TemplateParameterType::Type:
            ret ^= std::hash<Obelix::ObjectType> {}(*std::get<std::shared_ptr<ObjectType>>(value));
            break;
        case TemplateParameterType::Integer:
            ret ^= std::hash<long> {}(std::get<long>(value));
            break;
        case TemplateParameterType::String:
            ret ^= std::hash<std::string> {}(std::get<std::string>(value));
            break;
        case TemplateParameterType::Boolean:
            ret ^= std::hash<bool> {}(std::get<bool>(value));
            break;
        case TemplateParameterType::NameValue: {
            auto nvp = as_nvp();
            ret ^= std::hash<std::string> {}(nvp.first) ^ std::hash<long> {}(nvp.second);
            break;
        }
    }
    return ret;
}

std::string TemplateArgument::to_string() const
{
    if (multiplicity == TemplateParameterMultiplicity::Multiple) {
        std::string ret = "[ ";
        for (auto const& arg : as_arguments()) {
            ret += arg.to_string();
            ret += ' ';
        }
        return ret + "]";
    }
    switch (parameter_type) {
        case TemplateParameterType::Type:
            return std::get<std::shared_ptr<ObjectType>>(value)->to_string();
        case TemplateParameterType::Integer:
            return Obelix::to_string(std::get<long>(value));
        case TemplateParameterType::String:
            return std::get<std::string>(value);
        case TemplateParameterType::Boolean:
            return Obelix::to_string(std::get<bool>(value));
        case TemplateParameterType::NameValue: {
            auto nvp = as_nvp();
            return format("{}={}", nvp.first, nvp.second);
        }
        default:
            fatal("Unknown parameter type '{}'", (int) parameter_type);
    }
}

bool TemplateArgument::operator==(TemplateArgument const& other) const
{
    if (parameter_type != other.parameter_type || multiplicity != other.multiplicity)
        return false;
    if (multiplicity == TemplateParameterMultiplicity::Multiple) {
        auto my_values = as_arguments();
        auto other_values = other.as_arguments();
        return my_values == other_values;
    }
    switch (parameter_type) {
        case TemplateParameterType::Type:
            return *std::get<std::shared_ptr<ObjectType>>(value) == *std::get<std::shared_ptr<ObjectType>>(other.value);
        case TemplateParameterType::Integer:
            return std::get<long>(value) == std::get<long>(other.value);
        case TemplateParameterType::String:
            return std::get<std::string>(value) == std::get<std::string>(other.value);
        case TemplateParameterType::Boolean:
            return std::get<bool>(value) == std::get<bool>(other.value);
        case TemplateParameterType::NameValue:
            return std::get<NVP>(value) == std::get<NVP>(other.value);
        default:
            fatal("Unknown parameter type '{}'", (int) parameter_type);
    }
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

[[maybe_unused]] auto s_self = ObjectType::register_type(PrimitiveType::Self);
[[maybe_unused]] auto s_argument = ObjectType::register_type(PrimitiveType::Argument);
[[maybe_unused]] auto s_compatible = ObjectType::register_type(PrimitiveType::Compatible);
[[maybe_unused]] auto s_assignable_to = ObjectType::register_type(PrimitiveType::AssignableTo);
[[maybe_unused]] auto s_unknown = ObjectType::register_type(PrimitiveType::Unknown);

[[maybe_unused]] auto s_incrementable = ObjectType::register_type(PrimitiveType::Incrementable,
    [](std::shared_ptr<ObjectType> type) {
        type->add_method(MethodDescription { Operator::UnaryIncrement, PrimitiveType::Self });
        type->add_method(MethodDescription { Operator::UnaryDecrement, PrimitiveType::Self });
        type->add_method(MethodDescription { Operator::BinaryIncrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
        type->add_method(MethodDescription { Operator::BinaryDecrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
    });

[[maybe_unused]] auto s_comparable = ObjectType::register_type(PrimitiveType::Comparable,
    [](std::shared_ptr<ObjectType> type) {
        type->add_method(MethodDescription { Operator::Less, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible } }, true });
        type->add_method(MethodDescription { Operator::LessEquals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible } }, true });
        type->add_method(MethodDescription { Operator::Greater, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible } }, true });
        type->add_method(MethodDescription { Operator::GreaterEquals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible } }, true });
    });

[[maybe_unused]] auto s_boolean = ObjectType::register_type(PrimitiveType::Boolean,
    [](std::shared_ptr<ObjectType> type) {
        type->add_method(MethodDescription { Operator::LogicalInvert, PrimitiveType::Self, IntrinsicType::invert_bool, {}, true });
        type->add_method(MethodDescription { Operator::LogicalAnd, PrimitiveType::Self, IntrinsicType::and_bool_bool, { {  "other", PrimitiveType::Boolean } }, true });
        type->add_method(MethodDescription { Operator::LogicalOr, PrimitiveType::Self, IntrinsicType::or_bool_bool, { { "other", PrimitiveType::Boolean } }, true });
        type->has_size(1);
    });

[[maybe_unused]] auto s_integer_number = ObjectType::register_type(PrimitiveType::IntegerNumber,
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

[[maybe_unused]] auto s_signed_integer_number = ObjectType::register_type(PrimitiveType::SignedIntegerNumber,
    [](std::shared_ptr<ObjectType> type) {
        type->has_template_parameter({ "signed", TemplateParameterType::Boolean });
        type->has_template_parameter({ "size", TemplateParameterType::Integer });

        type->add_method(MethodDescription { Operator::Negate, PrimitiveType::Self, IntrinsicType::negate_int, {}, true });
        type->will_be_a(s_integer_number);
    });

[[maybe_unused]] auto s_integer = ObjectType::register_type("s32", s_signed_integer_number, TemplateArguments { { true }, { 4 } },
    [](std::shared_ptr<ObjectType> type) {
        type->has_alias("int");
        type->has_size(4);
    });

[[maybe_unused]] auto s_unsigned = ObjectType::register_type("u32", s_integer_number, TemplateArguments { { false }, { 4 } },
    [](std::shared_ptr<ObjectType> type) {
        type->has_alias("uint");
        type->has_size(4);
    });

[[maybe_unused]] auto s_long = ObjectType::register_type("s64", s_signed_integer_number, TemplateArguments { { true }, { 8 } },
    [](std::shared_ptr<ObjectType> type) {
        type->has_alias("long");
        type->has_size(8);
    });

[[maybe_unused]] auto s_ulong = ObjectType::register_type("u64", s_integer_number, TemplateArguments { { false }, { 8 } },
    [](std::shared_ptr<ObjectType> type) {
        type->has_alias("ulong");
        type->has_size(8);
    });

[[maybe_unused]] auto s_byte = ObjectType::register_type("s8", s_signed_integer_number, TemplateArguments { { true }, { 1 } },
    [](std::shared_ptr<ObjectType> type) {
        type->has_alias("byte");
        type->has_size(1);
    });

[[maybe_unused]] auto s_char = ObjectType::register_type("u8", s_integer_number, TemplateArguments { { false }, { 1 } },
    [](std::shared_ptr<ObjectType> type) {
        type->has_alias("char");
        type->has_size(1);
    });

[[maybe_unused]] auto s_float = ObjectType::register_type(PrimitiveType::Float,
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

[[maybe_unused]] auto s_null = ObjectType::register_type(PrimitiveType::Null,
    [](std::shared_ptr<ObjectType> type) {
    });

[[maybe_unused]] auto s_pointer = ObjectType::register_type(PrimitiveType::Pointer,
    [](std::shared_ptr<ObjectType> type) {
        type->has_template_parameter({ "target", TemplateParameterType::Type });
        type->has_alias("ptr");
        type->has_size(8);
        type->add_method(MethodDescription { Operator::Dereference, ObjectType::get("u8") });
        type->add_method(MethodDescription { Operator::UnaryIncrement, PrimitiveType::Self });
        type->add_method(MethodDescription { Operator::UnaryDecrement, PrimitiveType::Self });
        type->add_method(MethodDescription { Operator::BinaryIncrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { {  "other", ObjectType::get("s32") }  } });
        type->add_method(MethodDescription { Operator::BinaryDecrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { {  "other", ObjectType::get("s32") }  } });
        type->add_method(MethodDescription { Operator::Add, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { {  "other", ObjectType::get("s32") }  } });
        type->add_method(MethodDescription { Operator::Subtract, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { {  "other", ObjectType::get("s32") }  } });
        type->will_be_a(s_comparable);

        type->has_template_stamp([](std::shared_ptr<ObjectType> instantiation) {
            instantiation->add_method(MethodDescription { Operator::Dereference, instantiation->template_arguments()[0].as_type() });
        });
    });

[[maybe_unused]] auto s_array = ObjectType::register_type(PrimitiveType::Array,
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

[[maybe_unused]] auto s_string = ObjectType::register_struct_type("string",
    FieldDefs {
        FieldDef { std::string("length"), ObjectType::get("u32") },
        FieldDef { std::string("data"), ObjectType::specialize(ObjectType::get(PrimitiveType::Pointer), { ObjectType::get("u8") }).value() } },
    [](std::shared_ptr<ObjectType> type) {
        type->add_method(MethodDescription { Operator::Add, PrimitiveType::Self, IntrinsicType::add_str_str, { { "other", PrimitiveType::Self } }, true });
        type->add_method(MethodDescription { Operator::Multiply, PrimitiveType::Self, IntrinsicType::multiply_str_int, { { "other", ObjectType::get("u32") } }, true });
        type->will_be_a(s_comparable);
    });

[[maybe_unused]] auto s_any = ObjectType::register_type(PrimitiveType::Any,
    [](std::shared_ptr<ObjectType> type) {
        type->add_method(MethodDescription { Operator::Assign, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
        type->add_method(MethodDescription { Operator::Equals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
        type->add_method(MethodDescription { Operator::NotEquals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
        type->add_method(MethodDescription { Operator::Dereference, PrimitiveType::Any, IntrinsicType::NotIntrinsic, { { "attribute", get_type<std::string>() } } });
        type->add_method(MethodDescription { "typename", s_string });
        type->add_method(MethodDescription { "length", ObjectType::get("u32") });
        type->add_method(MethodDescription { "empty", PrimitiveType::Boolean });
    });

std::string ObjectType::to_string() const
{
    std::string ret = name();
    auto glue = '<';
    for (auto& parameter : template_arguments()) {
        ret += glue;
        ret += parameter.to_string();
        glue = ',';
    }
    if (glue == ',')
        ret += '>';
    return ret;
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

bool ObjectType::is_assignable_to(ObjectType const& other) const
{
    if (type() != PrimitiveType::Struct && type() != PrimitiveType::Array) {
        bool ret = (type() == other.type()) && (size() <= other.size());
        debug(type, "{}.is_assignable_to({}) = {}", to_string(), other.to_string(), ret);
        return ret;
    }
    return *this == other;
}

bool ObjectType::can_assign(ObjectType const& other) const
{
    return other.is_assignable_to(*this);
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
        auto& param = mth.parameters()[ix];
        auto arg_type = argument_types[ix];
        switch (param.type->type()) {
            case PrimitiveType::Self:
                if (*arg_type != *this)
                    return false;
                break;
            case PrimitiveType::Compatible:
                if (!can_assign(arg_type))
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
    debug(type, "ObjectType::get({})", type);
    if (!s_types_by_id.contains(type)) {
        ObjectType::register_type(type,
            [](std::shared_ptr<ObjectType> type) {
            });
    }
    return s_types_by_id.at(type);
}

std::shared_ptr<ObjectType> const& ObjectType::get(std::string const& type)
{
    debug(type, "ObjectType::get({})", type);
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
    auto ptr = std::make_shared<ObjectType>(type, PrimitiveType_name(type));
    register_type_in_caches(ptr);
    if (builder != nullptr)
        builder(ptr);
    return ptr;
}

std::shared_ptr<ObjectType> ObjectType::register_type(PrimitiveType type, char const* name, ObjectTypeBuilder const& builder) noexcept
{
    auto ptr = std::make_shared<ObjectType>(type, name);
    register_type_in_caches(ptr);
    if (builder != nullptr)
        builder(ptr);
    return ptr;
}

std::shared_ptr<ObjectType> ObjectType::register_type(char const* name, std::shared_ptr<ObjectType> specialization_of, TemplateArguments const& template_args, ObjectTypeBuilder const& builder) noexcept
{
    auto ret_or_error = ObjectType::specialize(specialization_of, move(template_args));
    if (ret_or_error.is_error())
        fatal("specialize '{}' failed: {}", name, ret_or_error.error());
    auto type = ret_or_error.value();
    type->m_name = name;
    type->m_name_str = name;
    register_type_in_caches(type);
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
    auto specialization = register_type(base_type->type(), 
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
