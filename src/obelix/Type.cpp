/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/StringUtil.h"
#include "obelix/Architecture.h"
#include "obelix/Intrinsics.h"
#include <core/Logging.h>
#include <ios>
#include <memory>
#include <obelix/Type.h>

namespace Obelix {

logging_category(type);

std::optional<PrimitiveType> PrimitiveType_by_name(std::string const& t)
{
    if (t == "int" || t == "s32")
        return PrimitiveType::Int;
    if (t == "unsigned" || t == "u32")
        return PrimitiveType::Unsigned;
    if (t == "byte" || t == "s8")
        return PrimitiveType::Byte;
    if (t == "char" || t == "u8")
        return PrimitiveType::Char;
    if (t == "bool")
        return PrimitiveType::Boolean;
    if (t == "string")
        return PrimitiveType::String;
    if (t == "ptr")
        return PrimitiveType::Pointer;
    return {};
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

MethodDescription::MethodDescription(char const* name, PrimitiveType type, IntrinsicType intrinsic, MethodParameters parameters)
    : m_name(name)
    , m_is_operator(false)
    , m_return_type(ObjectType::get(type))
    , m_varargs(false)
    , m_parameters(move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

MethodDescription::MethodDescription(char const* name, std::shared_ptr<ObjectType> type, IntrinsicType intrinsic, MethodParameters parameters)
    : m_name(name)
    , m_is_operator(false)
    , m_return_type(move(type))
    , m_varargs(false)
    , m_parameters(move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

MethodDescription::MethodDescription(Operator op, PrimitiveType type, IntrinsicType intrinsic, MethodParameters parameters)
    : m_operator(op)
    , m_is_operator(true)
    , m_return_type(ObjectType::get(type))
    , m_varargs(false)
    , m_parameters(move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

MethodDescription::MethodDescription(Operator op, std::shared_ptr<ObjectType> type, IntrinsicType intrinsic, MethodParameters parameters)
    : m_operator(op)
    , m_is_operator(true)
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

MethodImpl const& MethodDescription::implementation(Architecture arch) const
{
    if (m_implementations.contains(arch))
        return m_implementations.at(arch);
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
std::vector<std::shared_ptr<ObjectType>> ObjectType::s_template_instantiations {};

[[maybe_unused]] auto s_self = ObjectType::register_type(PrimitiveType::Self);
[[maybe_unused]] auto s_argument = ObjectType::register_type(PrimitiveType::Argument);
[[maybe_unused]] auto s_compatible = ObjectType::register_type(PrimitiveType::Compatible);
[[maybe_unused]] auto s_unknown = ObjectType::register_type(PrimitiveType::Unknown);

[[maybe_unused]] auto s_incrementable = ObjectType::register_type(PrimitiveType::Incrementable,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::UnaryIncrement, PrimitiveType::Self });
        type.add_method(MethodDescription { Operator::UnaryDecrement, PrimitiveType::Self });
        type.add_method(MethodDescription { Operator::BinaryIncrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
        type.add_method(MethodDescription { Operator::BinaryDecrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { { "other", PrimitiveType::Compatible } } });
    });

[[maybe_unused]] auto s_any = ObjectType::register_type(PrimitiveType::Any,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Assign, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method(MethodDescription { Operator::Equals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method(MethodDescription { Operator::NotEquals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method(MethodDescription { Operator::Dereference, PrimitiveType::Any, IntrinsicType::NotIntrinsic, { {  "attribute", PrimitiveType::String }  } });
        type.add_method(MethodDescription { "typename", PrimitiveType::String });
        type.add_method(MethodDescription { "length", PrimitiveType::Int });
        type.add_method(MethodDescription { "empty", PrimitiveType::Boolean });
    });

[[maybe_unused]] auto s_comparable = ObjectType::register_type(PrimitiveType::Comparable,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Less, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method(MethodDescription { Operator::LessEquals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method(MethodDescription { Operator::Greater, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method(MethodDescription { Operator::GreaterEquals, PrimitiveType::Boolean, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible }  } });
    });

[[maybe_unused]] auto s_integer_number = ObjectType::register_type(PrimitiveType::IntegerNumber,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Identity, PrimitiveType::Argument });

        type.add_method({ Operator::BitwiseInvert, PrimitiveType::Argument, IntrinsicType::invert_int });
        type.add_method({ Operator::Add, PrimitiveType::Self, IntrinsicType::add_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method({ Operator::Subtract, PrimitiveType::Self, IntrinsicType::subtract_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method({ Operator::Multiply, PrimitiveType::Self, IntrinsicType::multiply_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method({ Operator::Divide, PrimitiveType::Self, IntrinsicType::divide_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method({ Operator::BitwiseOr, PrimitiveType::Argument, IntrinsicType::bitwise_or_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method({ Operator::BitwiseAnd, PrimitiveType::Argument, IntrinsicType::bitwise_and_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method({ Operator::BitwiseXor, PrimitiveType::Argument, IntrinsicType::bitwise_xor_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method({ Operator::BitShiftLeft, PrimitiveType::Argument, IntrinsicType::shl_int, { {  "other", PrimitiveType::Unsigned }  } });
        type.add_method({ Operator::BitShiftRight, PrimitiveType::Argument, IntrinsicType::shr_int, { {  "other", PrimitiveType::Unsigned }  } });
        type.add_method({ Operator::Equals, PrimitiveType::Boolean, IntrinsicType::equals_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method({ Operator::Less, PrimitiveType::Boolean, IntrinsicType::less_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method({ Operator::Greater, PrimitiveType::Boolean, IntrinsicType::greater_int_int, { {  "other", PrimitiveType::Compatible }  } });
        type.add_method(MethodDescription { Operator::Range, PrimitiveType::Range,IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Compatible }  } });
        type.will_be_a(PrimitiveType::Comparable);
        type.will_be_a(PrimitiveType::Incrementable);
    });

[[maybe_unused]] auto s_signed_integer_number = ObjectType::register_type(PrimitiveType::SignedIntegerNumber,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Negate, PrimitiveType::Argument, IntrinsicType::negate_int });
        type.will_be_a(PrimitiveType::IntegerNumber);
    });

[[maybe_unused]] auto s_integer = ObjectType::register_type(PrimitiveType::Int,
    [](ObjectType& type) {
        type.will_be_a(PrimitiveType::SignedIntegerNumber);
        type.has_size(4);
    });

[[maybe_unused]] auto s_unsigned = ObjectType::register_type(PrimitiveType::Unsigned,
    [](ObjectType& type) {
        type.will_be_a(PrimitiveType::IntegerNumber);
        type.has_size(4);
    });

[[maybe_unused]] auto s_byte = ObjectType::register_type(PrimitiveType::Byte,
    [](ObjectType& type) {
        type.will_be_a(PrimitiveType::SignedIntegerNumber);
        type.has_size(1);
    });

[[maybe_unused]] auto s_char = ObjectType::register_type(PrimitiveType::Char,
    [](ObjectType& type) {
        type.will_be_a(PrimitiveType::IntegerNumber);
        type.has_size(1);
    });

[[maybe_unused]] auto s_string = ObjectType::register_type(PrimitiveType::String,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Add, PrimitiveType::String, IntrinsicType::add_str_str, { {  "other", PrimitiveType::String }  } });
        type.add_method(MethodDescription { Operator::Multiply, PrimitiveType::String, IntrinsicType::multiply_str_int, { {  "other", PrimitiveType::Unsigned }  } });
        type.will_be_a(PrimitiveType::Comparable);
        type.has_size(12);
    });

[[maybe_unused]] auto s_float = ObjectType::register_type(PrimitiveType::Float,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Identity, PrimitiveType::Float, IntrinsicType::NotIntrinsic });
        type.add_method(MethodDescription { Operator::Negate, PrimitiveType::Float, IntrinsicType::NotIntrinsic });
        type.add_method(MethodDescription { Operator::Add, PrimitiveType::Float, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Float }  } });
        type.add_method(MethodDescription { Operator::Subtract, PrimitiveType::Float, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Float }  } });
        type.add_method(MethodDescription { Operator::Multiply, PrimitiveType::Float, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Float }  } });
        type.add_method(MethodDescription { Operator::Divide, PrimitiveType::Float, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Float }  } });
        type.will_be_a(PrimitiveType::Comparable);
        type.has_size(4);
    });

[[maybe_unused]] auto s_boolean = ObjectType::register_type(PrimitiveType::Boolean,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::LogicalInvert, PrimitiveType::Boolean, IntrinsicType::invert_bool });
        type.add_method(MethodDescription { Operator::LogicalAnd, PrimitiveType::Boolean, IntrinsicType::and_bool_bool, { {  "other", PrimitiveType::Boolean }  } });
        type.add_method(MethodDescription { Operator::LogicalOr, PrimitiveType::Boolean, IntrinsicType::or_bool_bool, { {  "other", PrimitiveType::Boolean }  } });
        type.has_size(1);
    });

[[maybe_unused]] auto s_null = ObjectType::register_type(PrimitiveType::Null,
    [](ObjectType& type) {
    });

[[maybe_unused]] auto s_pointer = ObjectType::register_type(PrimitiveType::Pointer,
    [](ObjectType& type) {
        type.has_template_parameter("target");
        type.has_size(8);
        type.add_method(MethodDescription { Operator::Dereference, PrimitiveType::Char });
        type.add_method(MethodDescription { Operator::UnaryIncrement, PrimitiveType::Self });
        type.add_method(MethodDescription { Operator::UnaryDecrement, PrimitiveType::Self });
        type.add_method(MethodDescription { Operator::BinaryIncrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Int }  } });
        type.add_method(MethodDescription { Operator::BinaryDecrement, PrimitiveType::Self, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Int }  } });
        type.add_method(MethodDescription { Operator::Add, PrimitiveType::Argument, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Int }  } });
        type.add_method(MethodDescription { Operator::Subtract, PrimitiveType::Argument, IntrinsicType::NotIntrinsic, { {  "other", PrimitiveType::Int }  } });
        type.will_be_a(PrimitiveType::Comparable);

        type.has_template_stamp([](ObjectType& instantiation) {
            instantiation.add_method(MethodDescription { Operator::Dereference, instantiation.template_arguments()[0] });
        });
    });

std::string ObjectType::to_string() const
{
    std::string ret = name();
    auto glue = '<';
    for (auto& parameter : template_arguments()) {
        ret += glue;
        ret += parameter->to_string();
        glue = ',';
    }
    if (glue == ',')
        ret += '>';
    return ret;
}

bool ObjectType::operator==(ObjectType const& other) const
{
    if (name() != other.name())
        return false;
    if (template_arguments().size() != other.template_arguments().size())
        return false;
    for (auto ix = 0; ix < template_arguments().size(); ++ix) {
        if (*template_arguments()[ix] != *other.template_arguments()[ix])
            return false;
    }
    return true;
}

bool ObjectType::is_a(ObjectType const* other) const
{
    if ((*other == *this) || (other->type() == PrimitiveType::Any))
        return true;
    return std::any_of(m_is_a.begin(), m_is_a.end(), [&other](auto& super_type) { return super_type->is_a(other); });
}

bool ObjectType::is_compatible(MethodDescription const& mth, ObjectTypes const& argument_types) const
{
    if (mth.parameters().size() != argument_types.size())
        return false;
    auto ix = 0;
    for (; ix < mth.parameters().size(); ++ix) {
        auto& param = mth.parameters()[ix];
        auto arg_type = argument_types[ix];
        if (!arg_type->is_a(param.type) && (*param.type == *s_compatible && !arg_type->is_a(this)) && (*(param.type) == *s_self && *arg_type != *this))
            break;
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
        if (type->is_template_instantiation())
            types.push_back(type->instantiates_template());
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
            if (*(mth.return_type()) == *s_self)
                return ObjectType::get(this);
            if (*(mth.return_type()) == *s_argument)
                return argument_types[0];
            return mth.return_type();
        }
        return s_unknown;
    };

    debug(type, "{}::return_type_of({})", this->to_string(), op);
    auto self = get(this);
    ObjectTypes types { s_any, self };
    while (!types.empty()) {
        auto type = types.back();
        types.pop_back();
        for (auto& is_a : type->m_is_a) {
            types.push_back(is_a);
        }
        if (type->is_template_instantiation())
            types.push_back(type->instantiates_template());
        debug(type, "Checking operators of type {}", type->to_string());
        if (auto ret = check_operators_of(type.get()); *ret != *s_unknown) {
            debug(type, "Return type is {}", ret->to_string());
            return ret;
        }
    }
    debug(type, "No matching operator found");
    return {};
}

std::optional<MethodDescription> ObjectType::get_method(Operator op, ObjectTypes const& argument_types)
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
        if (type->is_template_instantiation())
            types.push_back(type->instantiates_template());
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
            [](ObjectType& type) {
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
    if (!type->is_template_instantiation())
        return get(type->name());

    for (auto& instantiation : s_template_instantiations) {
        if (*type == *instantiation) {
            return instantiation;
        }
    }
    return s_unknown;
}

ErrorOr<std::shared_ptr<ObjectType>> ObjectType::resolve(std::string const& type_name, ObjectTypes const& template_args)
{
    auto base_type = ObjectType::get(type_name);
    if (base_type == nullptr || *base_type == *s_unknown)
        return Error { ErrorCode::NoSuchType, type_name };
    if (base_type->is_parameterized() && (template_args.size() != base_type->template_parameters().size()))
        return Error { ErrorCode::TemplateParameterMismatch, type_name, base_type->template_parameters().size(), template_args.size() };
    if (!base_type->is_parameterized() && !template_args.empty())
        return Error { ErrorCode::TypeNotParameterized, type_name };
    if (!base_type->is_parameterized())
        return base_type;
    for (auto& template_instantiation : s_template_instantiations) {
        if (template_instantiation->instantiates_template() == base_type) {
            size_t ix;
            for (ix = 0; ix < template_args.size(); ++ix) {
                if (template_instantiation->template_arguments()[ix] != template_args[ix])
                    break;
            }
            if (ix >= template_args.size())
                return template_instantiation;
        }
    }
    auto instantiation = std::make_shared<ObjectType>(base_type->type(), [&template_args, &base_type](ObjectType& new_type) {
        new_type.m_instantiates_template = base_type;
        new_type.m_template_arguments = template_args;
        if (base_type->m_stamp) {
            base_type->m_stamp(new_type);
        }
    });
    s_template_instantiations.push_back(instantiation);
    return instantiation;
}

ErrorOr<std::shared_ptr<ObjectType>> ObjectType::make_type(std::string name, FieldDefs fields)
{
    std::shared_ptr<ObjectType> ret;
    if (s_types_by_name.contains(name))
        return Error { ErrorCode::DuplicateTypeName, name };
    assert(!fields.empty()); // TODO return proper error
    ret = std::make_shared<ObjectType>(PrimitiveType::Struct, name);
    ret->m_fields = move(fields);
    s_types_by_name[ret->name()] = ret;
    return ret;
}

}
