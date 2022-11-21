/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/syntax/ControlFlow.h>

namespace Obelix {

extern_logging_category(parser);

// -- Break -----------------------------------------------------------------

Break::Break(Token token)
    : Statement(std::move(token))
{
}

std::string Break::to_string() const
{
    return "break";
}

// -- Continue --------------------------------------------------------------

Continue::Continue(Token token)
    : Statement(std::move(token))
{
}
std::string Continue::to_string() const
{
    return "continue";
}

// -- Branch ----------------------------------------------------------------

Branch::Branch(Token token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> statement)
    : Statement(std::move(token))
    , m_condition(std::move(condition))
    , m_statement(std::move(statement))
{
}

Branch::Branch(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> statement)
    : Statement(node->token())
    , m_condition(std::move(condition))
    , m_statement(std::move(statement))
{
}

Branch::Branch(Token token, std::shared_ptr<Statement> statement)
    : Statement(std::move(token))
    , m_statement(std::move(statement))
{
}

Nodes Branch::children() const
{
    Nodes ret;
    if (m_condition)
        ret.push_back(m_condition);
    ret.push_back(m_statement);
    return ret;
}

std::string Branch::to_string() const
{
    if (m_condition)
        return format("if ({})\n{}", m_condition->to_string(), m_statement->to_string());
    return format("else\n{}", m_statement->to_string());
}

std::shared_ptr<Expression> const& Branch::condition() const
{
    return m_condition;
}

std::shared_ptr<Statement> const& Branch::statement() const
{
    return m_statement;
}

// -- IfStatement -----------------------------------------------------------

IfStatement::IfStatement(Token token, Branches branches)
    : Statement(std::move(token))
    , m_branches(std::move(branches))
{
    auto const& last_branch = m_branches.back();
    if (last_branch->condition() == nullptr) {
        m_branches.pop_back();
        m_else = last_branch->statement();
    }
}

IfStatement::IfStatement(Token token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> if_stmt, Branches branches, std::shared_ptr<Statement> else_stmt)
    : Statement(std::move(token))
    , m_branches(std::move(branches))
    , m_else(std::move(else_stmt))
{
    m_branches.insert(m_branches.begin(), std::make_shared<Branch>(if_stmt->token(), std::move(condition), std::move(if_stmt)));
}

IfStatement::IfStatement(Token token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> if_stmt, std::shared_ptr<Statement> else_stmt)
    : IfStatement(std::move(token), std::move(condition), std::move(if_stmt), Branches {}, std::move(else_stmt))
{
}

IfStatement::IfStatement(Token token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> if_stmt)
    : IfStatement(std::move(token), std::move(condition), std::move(if_stmt), nullptr)
{
}

std::shared_ptr<Statement> const& IfStatement::else_stmt() const
{
    return m_else;
}

Nodes IfStatement::children() const
{
    Nodes ret;
    for (auto& branch : m_branches) {
        ret.push_back(branch);
    }
    return ret;
}

std::string IfStatement::to_string() const
{
    std::string ret;
    bool first;
    for (auto& branch : m_branches) {
        if (first)
            ret = branch->to_string();
        else
            ret += "el" + branch->to_string();
        first = false;
    }
    if (m_else)
        ret += "else\n" + m_else->to_string();
    return ret;
}

Branches const& IfStatement::branches() const
{
    return m_branches;
}

// -- WhileStatement --------------------------------------------------------

WhileStatement::WhileStatement(Token token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> stmt)
    : Statement(std::move(token))
    , m_condition(std::move(condition))
    , m_stmt(std::move(stmt))
{
}

Nodes WhileStatement::children() const
{
    return { m_condition, m_stmt };
}

std::shared_ptr<Expression> const& WhileStatement::condition() const
{
    return m_condition;
}

std::shared_ptr<Statement> const& WhileStatement::statement() const
{
    return m_stmt;
}

std::string WhileStatement::to_string() const
{
    return format("while ({})\n{}", m_condition->to_string());
}

// -- ForStatement ----------------------------------------------------------

ForStatement::ForStatement(Token token, std::shared_ptr<Variable> variable, std::shared_ptr<Expression> range, std::shared_ptr<Statement> stmt)
    : Statement(std::move(token))
    , m_variable(std::move(variable))
    , m_range(std::move(range))
    , m_stmt(std::move(stmt))
{
}

std::string ForStatement::attributes() const
{
    return format(R"(variable="{}")", m_variable);
}

Nodes ForStatement::children() const
{
    return { m_range, m_stmt };
}

std::shared_ptr<Variable> const& ForStatement::variable() const
{
    return m_variable;
}

std::shared_ptr<Expression> const& ForStatement::range() const
{
    return m_range;
}

std::shared_ptr<Statement> const& ForStatement::statement() const
{
    return m_stmt;
}

std::string ForStatement::to_string() const
{
    return format("for ({} in {})\n{}", m_variable, m_range->to_string(), m_stmt->to_string());
}

// -- CaseStatement ---------------------------------------------------------

CaseStatement::CaseStatement(Token token, std::shared_ptr<Expression> const& case_expression, std::shared_ptr<Statement> const& stmt)
    : Branch(std::move(token), case_expression, stmt)
{
}

CaseStatement::CaseStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Expression> const& case_expression, std::shared_ptr<Statement> const& stmt)
    : Branch(node, case_expression, stmt)
{
}

// -- DefaultCase -----------------------------------------------------------

DefaultCase::DefaultCase(Token token, std::shared_ptr<Statement> const& stmt)
    : Branch(std::move(token), stmt)
{
}

DefaultCase::DefaultCase(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Statement> const& stmt)
    : Branch(node, nullptr, stmt)
{
}

DefaultCase::DefaultCase(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Expression> const&, std::shared_ptr<Statement> const& stmt)
    : Branch(node, nullptr, stmt)
{
}

// -- SwitchStatement -------------------------------------------------------

SwitchStatement::SwitchStatement(Token token, std::shared_ptr<Expression> switch_expression, CaseStatements case_statements, std::shared_ptr<DefaultCase> default_case)
    : Statement(std::move(token))
    , m_switch_expression(std::move(switch_expression))
    , m_cases(std::move(case_statements))
    , m_default(std::move(default_case))
{
}

std::shared_ptr<Expression> const& SwitchStatement::expression() const
{
    return m_switch_expression;
}

CaseStatements const& SwitchStatement::cases() const
{
    return m_cases;
}

std::shared_ptr<DefaultCase> const& SwitchStatement::default_case() const
{
    return m_default;
}

Nodes SwitchStatement::children() const
{
    Nodes ret;
    ret.push_back(m_switch_expression);
    for (auto& case_stmt : m_cases) {
        ret.push_back(case_stmt);
    }
    if (m_default)
        ret.push_back(m_default);
    return ret;
}

std::string SwitchStatement::to_string() const
{
    auto ret = format("switch ({}) {{\n", expression()->to_string());
    for (auto& case_stmt : m_cases) {
        ret += '\n';
        ret += case_stmt->to_string();
    }
    if (m_default) {
        ret += '\n';
        ret += m_default->to_string();
    }
    return ret + "}";
}

}
