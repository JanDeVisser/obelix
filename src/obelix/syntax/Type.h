/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Syntax.h>

namespace Obelix {

// -- Type.h ----------------------------------------------------------------

ABSTRACT_NODE_CLASS(TemplateArgumentNode, SyntaxNode)
public:
    explicit TemplateArgumentNode(Token);
    [[nodiscard]] virtual TemplateParameterType parameter_type() const = 0;
};

using TemplateArgumentNodes = std::vector<std::shared_ptr<TemplateArgumentNode>>;

NODE_CLASS(StringTemplateArgument, TemplateArgumentNode)
public:
    StringTemplateArgument(Token token, std::string value);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string value() const;
    [[nodiscard]] TemplateParameterType parameter_type() const override;

private:
    std::string m_value;
};

NODE_CLASS(IntegerTemplateArgument, TemplateArgumentNode)
public:
    IntegerTemplateArgument(Token token, long value);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] long value() const;
    [[nodiscard]] TemplateParameterType parameter_type() const override;

private:
    long m_value;
};

NODE_CLASS(ExpressionType, TemplateArgumentNode)
public:
    ExpressionType(Token, std::string, TemplateArgumentNodes);
    ExpressionType(Token, std::string);
    ExpressionType(Token, PrimitiveType);
    ExpressionType(Token, std::shared_ptr<ObjectType> const&);
    [[nodiscard]] bool is_template_instantiation() const;
    [[nodiscard]] std::string const& type_name() const;
    [[nodiscard]] TemplateArgumentNodes const& template_arguments() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] TemplateParameterType parameter_type() const override;
    [[nodiscard]] ErrorOr<std::shared_ptr<ObjectType>, SyntaxError> resolve_type() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_type_name;
    TemplateArgumentNodes m_template_args {};
};

using ExpressionTypes = std::vector<std::shared_ptr<ExpressionType>>;

}
