/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Syntax.h>
#include <obelix/syntax/Type.h>
#include <obelix/syntax/Expression.h>
#include <obelix/syntax/Statement.h>

namespace Obelix {

NODE_CLASS(VariableDeclaration, Statement)
public:
    VariableDeclaration(Span, std::shared_ptr<Identifier>, std::shared_ptr<Expression> = nullptr, bool = false);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<Identifier> const& identifier() const;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ExpressionType> const& type() const;
    [[nodiscard]] bool is_typed() const;
    [[nodiscard]] bool is_const() const;
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const;

private:
    std::shared_ptr<Identifier> m_identifier;
    bool m_const { false };
    std::shared_ptr<Expression> m_expression;
};

NODE_CLASS(StaticVariableDeclaration, VariableDeclaration)
public:
    StaticVariableDeclaration(Span, std::shared_ptr<Identifier>, std::shared_ptr<Expression> = nullptr, bool = false);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(LocalVariableDeclaration, VariableDeclaration)
public:
    LocalVariableDeclaration(Span, std::shared_ptr<Identifier>, std::shared_ptr<Expression> = nullptr, bool = false);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(GlobalVariableDeclaration, VariableDeclaration)
public:
    GlobalVariableDeclaration(Span, std::shared_ptr<Identifier>, std::shared_ptr<Expression> = nullptr, bool = false);
    [[nodiscard]] std::string to_string() const override;
};

}
