/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/syntax/Type.h>

namespace Obelix {

extern_logging_category(parser);

// -- TemplateArgumentNode --------------------------------------------------

TemplateArgumentNode::TemplateArgumentNode(Span location)
    : SyntaxNode(std::move(location))
{
}

// -- StringTemplateArgument ------------------------------------------------

StringTemplateArgument::StringTemplateArgument(Span location, std::string value)
    : TemplateArgumentNode(std::move(location))
    , m_value(std::move(value))
{
}

std::string StringTemplateArgument::to_string() const
{
    return '"' + value() + '"';
}

std::string StringTemplateArgument::attributes() const
{
    return format(R"(argument_type="string" value="{}")", value());
}

std::string StringTemplateArgument::value() const
{
    return m_value;
}

TemplateParameterType StringTemplateArgument::parameter_type() const
{
    return TemplateParameterType::String;
}

// -- IntegerTemplateArgument -----------------------------------------------

IntegerTemplateArgument::IntegerTemplateArgument(Span location, long value)
    : TemplateArgumentNode(std::move(location))
    , m_value(value)
{
}

std::string IntegerTemplateArgument::to_string() const
{
    return Obelix::to_string(value());
}

std::string IntegerTemplateArgument::attributes() const
{
    return format(R"(argument_type="integer" value="{}")", value());
}

long IntegerTemplateArgument::value() const
{
    return m_value;
}

TemplateParameterType IntegerTemplateArgument::parameter_type() const
{
    return TemplateParameterType::Integer;
}

// -- ExpressionType --------------------------------------------------------

ExpressionType::ExpressionType(Span location, std::string type_name, TemplateArgumentNodes template_arguments)
    : TemplateArgumentNode(std::move(location))
    , m_type_name(std::move(type_name))
    , m_template_args(std::move(template_arguments))
{
}

ExpressionType::ExpressionType(Span location, std::string type_name)
    : TemplateArgumentNode(std::move(location))
    , m_type_name(std::move(type_name))
{
}

ExpressionType::ExpressionType(Span location, PrimitiveType type)
    : TemplateArgumentNode(std::move(location))
    , m_type_name(PrimitiveType_name(type))
{
}

ExpressionType::ExpressionType(Span location, std::shared_ptr<ObjectType> const& type)
    : TemplateArgumentNode(std::move(location))
    , m_type_name(type->name())
{
}

bool ExpressionType::is_template_instantiation() const
{
    return !m_template_args.empty();
}

std::string const& ExpressionType::type_name() const
{
    return m_type_name;
}

TemplateArgumentNodes const& ExpressionType::template_arguments() const
{
    return m_template_args;
}

std::string ExpressionType::attributes() const
{
    return format(R"(argument_type="type" value="{}")", type_name());
}

TemplateParameterType ExpressionType::parameter_type() const
{
    return TemplateParameterType::Type;
}

ErrorOr<std::shared_ptr<ObjectType>, SyntaxError> ExpressionType::resolve_type() const
{
    auto type = ObjectType::get(type_name());
    if (!type)
        return SyntaxError { location(), "Type '{}' does not exist", type_name() };
    if (!type->is_parameterized()) {
        if (!template_arguments().empty())
            return SyntaxError { location(), "Type '{}' is not parameterized so should cannot be specialized", type_name() };
        return type;
    }
    if (template_arguments().size() > type->template_parameters().size())
        return SyntaxError { location(), "Type '{}' has only {} parameters so cannot be specialized with {} arguments", type_name(), type->template_parameters().size(), template_arguments().size() };
    TemplateArguments args;
    auto ix = 0;
    for (auto& arg : template_arguments()) {
        auto param = type->template_parameter(ix);
        switch (arg->node_type()) {
        case SyntaxNodeType::ExpressionType: {
            if (param.type != TemplateParameterType::Type)
                return SyntaxError { location(), "Template parameter {} of '{}' has parameter type '{}', not '{}'", ix, type_name(), param.type, arg->parameter_type() };
            auto expr_type = std::dynamic_pointer_cast<ExpressionType>(arg);
            auto arg_type_or_error = expr_type->resolve_type();
            if (arg_type_or_error.is_error())
                return arg_type_or_error.error();
            args[param.name] = arg_type_or_error.value();
            break;
        }
        case SyntaxNodeType::StringTemplateArgument:
            if (param.type != TemplateParameterType::String)
                return SyntaxError { location(), "Template parameter {} of '{}' has parameter type '{}', not '{}'", ix, type_name(), param.type, arg->parameter_type() };
            args[param.name] = std::dynamic_pointer_cast<StringTemplateArgument>(arg)->value();
            break;
        case SyntaxNodeType::IntegerTemplateArgument:
            if (param.type != TemplateParameterType::Integer)
                return SyntaxError { location(), "Template parameter {} of '{}' has parameter type '{}', not '{}'", ix, type_name(), param.type, arg->parameter_type() };
            args[param.name] = std::dynamic_pointer_cast<IntegerTemplateArgument>(arg)->value();
            break;
        default:
            fatal("Unreachable: nodes of type '{}' can't be template arguments", arg->node_type());
        }
        ++ix;
    }
    auto ret = ObjectType::specialize(type_name(), args);
    if (ret.is_error())
        return SyntaxError { location(), "Could not specialize template class '{}': {}", type_name(), ret.error().message() };
    return ret.value();
}

Nodes ExpressionType::children() const
{
    Nodes ret;
    for (auto& parameter : template_arguments()) {
        ret.push_back(parameter);
    }
    return ret;
}

std::string ExpressionType::to_string() const
{
    auto ret = type_name();
    auto glue = '<';
    for (auto& parameter : template_arguments()) {
        ret += glue;
        glue = ',';
        ret += parameter->to_string();
    }
    if (is_template_instantiation())
        ret += '>';
    return ret;
}

}
