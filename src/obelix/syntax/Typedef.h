/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/syntax/Syntax.h>
#include <obelix/syntax/Type.h>
#include <obelix/syntax/Expression.h>
#include <obelix/syntax/Statement.h>

namespace Obelix {

NODE_CLASS(StructForward, Statement)
public:
    StructForward(Token, std::string);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string const& name() const;

private:
    std::string m_name;
};

NODE_CLASS(StructDefinition, Statement)
public:
    StructDefinition(Token, std::string, Identifiers);
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] Identifiers const& fields() const;

private:
    std::string m_name;
    Identifiers m_fields;
};

NODE_CLASS(EnumValue, SyntaxNode)
public:
    EnumValue(Token token, std::string label, std::optional<long> value);
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
    EnumDef(Token, std::string, EnumValues);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] EnumValues const& values() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

protected:
    std::string m_name;
    EnumValues m_values;
};

}
