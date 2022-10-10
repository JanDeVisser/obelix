/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/syntax/ControlFlow.h>
#include <obelix/boundsyntax/Variable.h>

namespace Obelix {

// -- FlowControl.h ---------------------------------------------------------

NODE_CLASS(BoundReturn, Statement)
public:
    BoundReturn(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundExpression>);
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;

private:
    std::shared_ptr<BoundExpression> m_expression;
};

NODE_CLASS(BoundBranch, Statement)
public:
    BoundBranch(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundExpression>, std::shared_ptr<Statement>);
    BoundBranch(Token, std::shared_ptr<BoundExpression>, std::shared_ptr<Statement>);
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& condition() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;

private:
    std::shared_ptr<BoundExpression> m_condition { nullptr };
    std::shared_ptr<Statement> m_statement;
};

using BoundBranches = std::vector<std::shared_ptr<BoundBranch>>;

NODE_CLASS(BoundIfStatement, Statement)
public:
    BoundIfStatement(std::shared_ptr<IfStatement> const& if_stmt, BoundBranches branches, std::shared_ptr<Statement> else_stmt);
    BoundIfStatement(Token token, BoundBranches branches, std::shared_ptr<Statement> else_stmt = nullptr);

    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] BoundBranches const& branches() const;
private:
    BoundBranches m_branches {};
    std::shared_ptr<Statement> m_else {};
};

NODE_CLASS(BoundWhileStatement, Statement)
public:
    BoundWhileStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> stmt);

    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& condition() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;
    [[nodiscard]] std::string to_string() const override;
private:
    std::shared_ptr<BoundExpression> m_condition;
    std::shared_ptr<Statement> m_stmt;
};

NODE_CLASS(BoundForStatement, Statement)
public:
    BoundForStatement(std::shared_ptr<ForStatement> const&, std::shared_ptr<BoundVariable>, std::shared_ptr<BoundExpression>, std::shared_ptr<Statement>, bool);
    BoundForStatement(std::shared_ptr<BoundForStatement> const&, std::shared_ptr<BoundVariable>, std::shared_ptr<BoundExpression>, std::shared_ptr<Statement>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::shared_ptr<BoundVariable> const& variable() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& range() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;
    [[nodiscard]] bool must_declare_variable() const;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<BoundVariable> m_variable;
    std::shared_ptr<BoundExpression> m_range;
    std::shared_ptr<Statement> m_stmt;
    bool m_must_declare_variable { false };
};

NODE_CLASS(BoundCaseStatement, BoundBranch)
public:
    BoundCaseStatement(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundExpression>, std::shared_ptr<Statement>);
    BoundCaseStatement(Token, std::shared_ptr<BoundExpression>, std::shared_ptr<Statement>);
};

using BoundCaseStatements = std::vector<std::shared_ptr<BoundCaseStatement>>;

NODE_CLASS(BoundDefaultCase, BoundBranch)
public:
    BoundDefaultCase(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<Statement>);
    BoundDefaultCase(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundExpression> const&, std::shared_ptr<Statement>);
};

NODE_CLASS(BoundSwitchStatement, Statement)
public:
    BoundSwitchStatement(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundExpression>, BoundCaseStatements, std::shared_ptr<BoundDefaultCase>);
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;
    [[nodiscard]] BoundCaseStatements const& cases() const;
    [[nodiscard]] std::shared_ptr<BoundDefaultCase> const& default_case() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<BoundExpression> m_switch_expression;
    BoundCaseStatements m_cases {};
    std::shared_ptr<BoundDefaultCase> m_default {};
};

}