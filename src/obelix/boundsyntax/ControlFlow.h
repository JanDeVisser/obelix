/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/boundsyntax/Statement.h>
#include <obelix/boundsyntax/Expression.h>
#include <obelix/syntax/Forward.h>

namespace Obelix {

NODE_CLASS(BoundPass, BoundStatement)
public:
    explicit BoundPass(Span, std::shared_ptr<Statement> const& = nullptr);
    [[nodiscard]] std::shared_ptr<Statement> const& elided_statement() const;
    [[nodiscard]] std::string text_contents() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<Statement> m_elided_statement;
};

NODE_CLASS(BoundReturn, BoundStatement)
public:
    BoundReturn(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundExpression>, bool = false);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;
    [[nodiscard]] bool return_error() const;

private:
    std::shared_ptr<BoundExpression> m_expression;
    bool m_return_error;
};

NODE_CLASS(BoundBranch, BoundStatement)
public:
    BoundBranch(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundExpression>, std::shared_ptr<Statement>);
    BoundBranch(Span, std::shared_ptr<BoundExpression>, std::shared_ptr<Statement>);
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& condition() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;

private:
    std::shared_ptr<BoundExpression> m_condition { nullptr };
    std::shared_ptr<Statement> m_statement;
};

using BoundBranches = std::vector<std::shared_ptr<BoundBranch>>;

NODE_CLASS(BoundIfStatement, BoundStatement)
public:
    BoundIfStatement(std::shared_ptr<IfStatement> const& if_stmt, BoundBranches branches, std::shared_ptr<Statement> else_stmt);
    BoundIfStatement(Span location, BoundBranches branches, std::shared_ptr<Statement> else_stmt = nullptr);

    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] BoundBranches const& branches() const;
private:
    BoundBranches m_branches {};
    std::shared_ptr<Statement> m_else {};
};

NODE_CLASS(BoundWhileStatement, BoundStatement)
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

NODE_CLASS(BoundForStatement, BoundStatement)
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

NODE_CLASS(BoundSwitchStatement, BoundStatement)
public:
    BoundSwitchStatement(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundExpression>, BoundBranches, std::shared_ptr<BoundBranch>);
    BoundSwitchStatement(Span location, std::shared_ptr<BoundExpression>, BoundBranches, std::shared_ptr<BoundBranch>);
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;
    [[nodiscard]] BoundBranches const& cases() const;
    [[nodiscard]] std::shared_ptr<BoundBranch> const& default_case() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<BoundExpression> m_switch_expression;
    BoundBranches m_cases {};
    std::shared_ptr<BoundBranch> m_default {};
};

}
