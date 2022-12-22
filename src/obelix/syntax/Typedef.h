/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Syntax.h>
#include <obelix/syntax/Type.h>
#include <obelix/syntax/Expression.h>
#include <obelix/syntax/Function.h>
#include <obelix/syntax/Statement.h>

namespace Obelix {

NODE_CLASS(StructForward, Statement)
public:
    StructForward(Span, std::string);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string const& name() const;

private:
    std::string m_name;
};

NODE_CLASS(StructDefinition, Statement)
public:
    StructDefinition(Span, std::string, Identifiers, FunctionDefs = {});
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] Identifiers const& fields() const;
    [[nodiscard]] FunctionDefs const& methods() const;
private:
    std::string m_name;
    Identifiers m_fields;
    FunctionDefs m_methods;
};

NODE_CLASS(EnumValue, SyntaxNode)
public:
    EnumValue(Span location, std::string label, std::optional<long> value);
    [[nodiscard]] std::optional<long> const& value() const;
    [[nodiscard]] std::string const& label() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::optional<long> m_value;
    std::string m_label;
};

using EnumValues = std::vector<std::shared_ptr<EnumValue>>;

NODE_CLASS(EnumDef, Statement)
public:
    EnumDef(Span, std::string, EnumValues, bool = false);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] EnumValues const& values() const;
    [[nodiscard]] bool extend() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_name;
    EnumValues m_values;
    bool m_extend { false };
};

NODE_CLASS(TypeDef, Statement)
public:
    TypeDef(Span, std::string, std::shared_ptr<ExpressionType>);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ExpressionType> const& type() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_name;
    std::shared_ptr<ExpressionType> m_type;
};

}
