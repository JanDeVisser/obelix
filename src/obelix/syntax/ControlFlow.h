/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Syntax.h>
#include <obelix/syntax/Type.h>
#include <obelix/syntax/Expression.h>
#include <obelix/syntax/Statement.h>

namespace Obelix {

NODE_CLASS(Break, Statement)
public:
    explicit Break(Span);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] bool is_fully_bound() const override { return true; }
};

NODE_CLASS(Continue, Statement)
public:
    explicit Continue(Span);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] bool is_fully_bound() const override { return true; }
};

NODE_CLASS(Branch, Statement)
public:
    Branch(Span, std::shared_ptr<Expression>, std::shared_ptr<Statement>);
    Branch(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<Expression>, std::shared_ptr<Statement>);
    Branch(Span, std::shared_ptr<Statement>);
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<Expression> const& condition() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;

private:
    std::shared_ptr<Expression> m_condition { nullptr };
    std::shared_ptr<Statement> m_statement;
};

using Branches = std::vector<std::shared_ptr<Branch>>;

NODE_CLASS(IfStatement, Statement)
public:
    IfStatement(Span, Branches);
    IfStatement(Span, std::shared_ptr<Expression>, std::shared_ptr<Statement>, Branches, std::shared_ptr<Statement>);
    IfStatement(Span, std::shared_ptr<Expression>, std::shared_ptr<Statement>, std::shared_ptr<Statement>);
    IfStatement(Span, std::shared_ptr<Expression>, std::shared_ptr<Statement>);
    [[nodiscard]] std::shared_ptr<Statement> const& else_stmt() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] Branches const& branches() const;

private:
    Branches m_branches {};
    std::shared_ptr<Statement> m_else {};
};

NODE_CLASS(WhileStatement, Statement)
public:
    WhileStatement(Span, std::shared_ptr<Expression>, std::shared_ptr<Statement>);
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::shared_ptr<Expression> const& condition() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Statement> m_stmt;
};

NODE_CLASS(ForStatement, Statement)
public:
    ForStatement(Span, std::shared_ptr<Variable>, std::shared_ptr<Expression>, std::shared_ptr<Statement>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::shared_ptr<Variable> const& variable() const;
    [[nodiscard]] std::shared_ptr<Expression> const& range() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<Variable> m_variable;
    std::shared_ptr<Expression> m_range;
    std::shared_ptr<Statement> m_stmt;
};

NODE_CLASS(CaseStatement, Branch)
public:
    CaseStatement(Span, std::shared_ptr<Expression> const&, std::shared_ptr<Statement> const&);
    CaseStatement(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<Expression> const&, std::shared_ptr<Statement> const&);
};

using CaseStatements = std::vector<std::shared_ptr<CaseStatement>>;

NODE_CLASS(DefaultCase, Branch)
public:
    DefaultCase(Span, std::shared_ptr<Statement> const&);
    DefaultCase(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<Statement> const&);
    DefaultCase(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<Expression> const&, std::shared_ptr<Statement> const&);
};

NODE_CLASS(SwitchStatement, Statement)
public:
    SwitchStatement(Span, std::shared_ptr<Expression>, CaseStatements, std::shared_ptr<DefaultCase>);
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const;
    [[nodiscard]] CaseStatements const& cases() const;
    [[nodiscard]] std::shared_ptr<DefaultCase> const& default_case() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<Expression> m_switch_expression;
    CaseStatements m_cases {};
    std::shared_ptr<DefaultCase> m_default {};
};

}
