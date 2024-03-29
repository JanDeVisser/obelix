/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/syntax/ControlFlow.h>
#include <obelix/boundsyntax/ControlFlow.h>
#include <obelix/boundsyntax/Variable.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundPass -------------------------------------------------------------

BoundPass::BoundPass(Span location, std::shared_ptr<Statement> const& elided_statement)
    : BoundStatement(std::move(location))
    , m_elided_statement(std::move(elided_statement))
{
}

std::shared_ptr<Statement> const& BoundPass::elided_statement() const
{
    return m_elided_statement;
}

std::string BoundPass::text_contents() const
{
    if (!m_elided_statement)
        return "";
    return format("{}/* {} */", m_elided_statement->to_string());
}

std::string BoundPass::to_string() const
{
    return text_contents();
}

// -- BoundIfStatement ------------------------------------------------------

BoundIfStatement::BoundIfStatement(std::shared_ptr<IfStatement> const& if_stmt, BoundBranches branches, std::shared_ptr<Statement> else_stmt)
    : BoundStatement(if_stmt->location())
    , m_branches(std::move(branches))
    , m_else(std::move(else_stmt))
{
}

BoundIfStatement::BoundIfStatement(Span location, BoundBranches branches, std::shared_ptr<Statement> else_stmt)
    : BoundStatement(std::move(location))
    , m_branches(std::move(branches))
    , m_else(std::move(else_stmt))
{
}

Nodes BoundIfStatement::children() const
{
    Nodes ret;
    for (auto& branch : m_branches) {
        ret.push_back(branch);
    }
    return ret;
}

std::string BoundIfStatement::to_string() const
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

BoundBranches const& BoundIfStatement::branches() const
{
    return m_branches;
}

// -- BoundWhileStatement ---------------------------------------------------

BoundWhileStatement::BoundWhileStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> stmt)
    : BoundStatement(node->location())
    , m_condition(std::move(condition))
    , m_stmt(std::move(stmt))
{
}

Nodes BoundWhileStatement::children() const
{
    return { m_condition, m_stmt };
}

std::shared_ptr<BoundExpression> const& BoundWhileStatement::condition() const
{
    return m_condition;
}

std::shared_ptr<Statement> const& BoundWhileStatement::statement() const
{
    return m_stmt;
}

std::string BoundWhileStatement::to_string() const
{
    return format("while ({})\n{}", m_condition->to_string(), m_stmt->to_string());
}

// -- BoundReturn -----------------------------------------------------------

BoundReturn::BoundReturn(std::shared_ptr<SyntaxNode> const& ret, std::shared_ptr<BoundExpression> expression, bool return_error)
    : BoundStatement(ret->location())
    , m_expression(std::move(expression))
    , m_return_error(return_error)
{
}

std::string BoundReturn::attributes() const
{
    return format(R"(return_error="{}")", return_error());
}

Nodes BoundReturn::children() const
{
    if (m_expression)
        return { m_expression };
    return {};
}

std::string BoundReturn::to_string() const
{
    std::string ret = (return_error()) ? "error" : "return";
    if (m_expression)
        ret = format("{} {}", ret, m_expression->to_string());
    return ret;
}

std::shared_ptr<BoundExpression> const& BoundReturn::expression() const
{
    return m_expression;
}

bool BoundReturn::return_error() const
{
    return m_return_error;
}

// -- BoundBranch -----------------------------------------------------------

BoundBranch::BoundBranch(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> bound_statement)
    : BoundStatement(node->location())
    , m_condition(std::move(condition))
    , m_statement(std::move(bound_statement))
{
}

BoundBranch::BoundBranch(Span location, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> bound_statement)
    : BoundStatement(std::move(location))
    , m_condition(std::move(condition))
    , m_statement(std::move(bound_statement))
{
}

Nodes BoundBranch::children() const
{
    Nodes ret;
    if (m_condition)
        ret.push_back(m_condition);
    ret.push_back(m_statement);
    return ret;
}

std::string BoundBranch::to_string() const
{
    if (m_condition)
        return format("if ({})\n{}", m_condition->to_string(), m_statement->to_string());
    return format("else\n{}", m_statement->to_string());
}

std::shared_ptr<BoundExpression> const& BoundBranch::condition() const
{
    return m_condition;
}

std::shared_ptr<Statement> const& BoundBranch::statement() const
{
    return m_statement;
}

// -- BoundForStatement -----------------------------------------------------

BoundForStatement::BoundForStatement(std::shared_ptr<ForStatement> const& orig_for_stmt, std::shared_ptr<BoundVariable> variable, std::shared_ptr<BoundExpression> range, std::shared_ptr<Statement> stmt, bool must_declare)
    : BoundStatement(orig_for_stmt->location())
    , m_variable(std::move(variable))
    , m_range(std::move(range))
    , m_stmt(std::move(stmt))
    , m_must_declare_variable(must_declare)
{
}

BoundForStatement::BoundForStatement(std::shared_ptr<BoundForStatement> const& orig_for_stmt, std::shared_ptr<BoundVariable> variable, std::shared_ptr<BoundExpression> range, std::shared_ptr<Statement> stmt)
    : BoundStatement(orig_for_stmt->location())
    , m_variable(std::move(variable))
    , m_range(std::move(range))
    , m_stmt(std::move(stmt))
    , m_must_declare_variable(orig_for_stmt->must_declare_variable())
{
}

std::string BoundForStatement::attributes() const
{
    return format(R"(variable="{}")", m_variable);
}

Nodes BoundForStatement::children() const
{
    return { m_range, m_stmt };
}

std::shared_ptr<BoundVariable> const& BoundForStatement::variable() const
{
    return m_variable;
}

std::shared_ptr<BoundExpression> const& BoundForStatement::range() const
{
    return m_range;
}

std::shared_ptr<Statement> const& BoundForStatement::statement() const
{
    return m_stmt;
}

bool BoundForStatement::must_declare_variable() const
{
    return m_must_declare_variable;
}

std::string BoundForStatement::to_string() const
{
    return format("for ({} in {})\n{}", m_variable, m_range->to_string(), m_stmt->to_string());
}

// -- BoundSwitchStatement ----------------------------------------------------------

BoundSwitchStatement::BoundSwitchStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> switch_expr, BoundBranches cases, std::shared_ptr<BoundBranch> default_case)
    : BoundStatement(node->location())
    , m_switch_expression(std::move(switch_expr))
    , m_cases(std::move(cases))
    , m_default(std::move(default_case))
{
}

BoundSwitchStatement::BoundSwitchStatement(Span location, std::shared_ptr<BoundExpression> switch_expr, BoundBranches cases, std::shared_ptr<BoundBranch> default_case)
    : BoundStatement(std::move(location))
    , m_switch_expression(std::move(switch_expr))
    , m_cases(std::move(cases))
    , m_default(std::move(default_case))
{
}

std::shared_ptr<BoundExpression> const& BoundSwitchStatement::expression() const
{
    return m_switch_expression;
}

BoundBranches const& BoundSwitchStatement::cases() const
{
    return m_cases;
}

std::shared_ptr<BoundBranch> const& BoundSwitchStatement::default_case() const
{
    return m_default;
}

Nodes BoundSwitchStatement::children() const
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

std::string BoundSwitchStatement::to_string() const
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
