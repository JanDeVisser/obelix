/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/syntax/Statement.h>

namespace Obelix {

extern_logging_category(parser);

// -- Statement -------------------------------------------------------------

Statement::Statement(Token token)
    : SyntaxNode(std::move(token))
{
}

// -- Import ---------------------------------------------------------------

Import::Import(Token token, std::string name)
    : Statement(std::move(token))
    , m_name(std::move(name))
{
}

std::string Import::attributes() const
{
    return format("module=\"{}\"", m_name);
}

std::string Import::to_string() const
{
    return format("import {}", m_name);
}

std::string const& Import::name() const
{
    return m_name;
}

// -- Pass ------------------------------------------------------------------

Pass::Pass(Token token)
    : Statement(std::move(token))
{
}

Pass::Pass(std::shared_ptr<Statement> elided_statement)
    : Statement(elided_statement->token())
    , m_elided_statement(std::move(elided_statement))
{
}

std::string Pass::text_contents() const
{
    if (!m_elided_statement)
        return "";
    return format("{}/* {} */", m_elided_statement->to_string());
}

std::string Pass::to_string() const
{
    return text_contents();
}

// -- Label -----------------------------------------------------------------

int Label::m_current_id = 0;

Label::Label(Token token)
    : Statement(std::move(token))
    , m_label_id(reserve_id())
{
}

Label::Label(std::shared_ptr<Goto> const& goto_stmt)
    : Statement(goto_stmt->token())
    , m_label_id(goto_stmt->label_id())
{
}

std::string Label::attributes() const
{
    return format("id=\"{}\"", m_label_id);
}

std::string Label::to_string() const
{
    return format("{}:", label_id());
}

int Label::label_id() const
{
    return m_label_id;
}

int Label::reserve_id()
{
    return m_current_id++;
}

// -- Goto ------------------------------------------------------------------

Goto::Goto(Token token, std::shared_ptr<Label> const& label)
    : Statement(std::move(token))
    , m_label_id((label) ? label->label_id() : Label::reserve_id())
{
}

std::string Goto::attributes() const
{
    return format("label=\"{}\"", m_label_id);
}

std::string Goto::to_string() const
{
    return format("goto {}", label_id());
}

int Goto::label_id() const
{
    return m_label_id;
}

// -- Block -----------------------------------------------------------------

Block::Block(Token token, Statements statements)
    : Statement(std::move(token))
    , m_statements(std::move(statements))
{
}

Statements const& Block::statements() const
{
    return m_statements;
}

Nodes Block::children() const
{
    Nodes ret;
    for (auto& statement : m_statements) {
        ret.push_back(statement);
    }
    return ret;
}
std::string Block::to_string() const
{
    return format("[ ... {} statements ... ]", m_statements.size());
}

// -- Module ----------------------------------------------------------------

Module::Module(Statements const& statements, std::string name)
    : Block({}, statements)
    , m_name(std::move(name))
{
}

Module::Module(Token token, Statements const& statements, std::string name)
    : Block(std::move(token), statements)
    , m_name(std::move(name))
{
}

Module::Module(std::shared_ptr<Module> const& original, Statements const& statements)
    : Block(original->token(), statements)
    , m_name(original->name())
{
}

std::string Module::attributes() const
{
    return format("name=\"{}\"", m_name);
}

std::string Module::to_string() const
{
    return format("module {} {}", name(), Block::to_string());
}

const std::string& Module::name() const
{
    return m_name;
}

// -- Compilation -----------------------------------------------------------

Compilation::Compilation(std::shared_ptr<Module> const& root, Modules modules)
    : Module(root->statements(), "")
    , m_modules(std::move(modules))
{
}

Compilation::Compilation(Statements const& statements, Modules modules)
    : Module(statements, "")
    , m_modules(std::move(modules))
{
}

Modules const& Compilation::modules() const
{
    return m_modules;
}

Nodes Compilation::children() const
{
    Nodes ret /* = Block::children() */;
    for (auto& module : m_modules) {
        ret.push_back(module);
    }
    return ret;
}

std::string Compilation::to_string() const
{
    std::string ret = Module::to_string();
    for (auto& module : m_modules) {
        ret += "\n";
        ret += module->to_string();
    }
    return ret;
}

std::string Compilation::root_to_xml() const
{
    auto indent = 0u;
    auto ret = format("<{}", node_type());
    auto attrs = Module::attributes();
    auto child_nodes = Module::children();
    auto text = Module::text_contents();
    if (!attrs.empty()) {
        ret += " " + attrs;
    }
    if (text.empty() && child_nodes.empty())
        return ret + "/>";
    ret += ">\n";
    for (auto& child : child_nodes) {
        ret += child->to_xml(indent + 2);
        ret += "\n";
    }
    return ret + format("{}</{}>", text, node_type());
}

// -- ExpressionStatement ---------------------------------------------------

ExpressionStatement::ExpressionStatement(std::shared_ptr<Expression> expression)
    : Statement(expression->token())
    , m_expression(std::move(expression))
{
}

std::shared_ptr<Expression> const& ExpressionStatement::expression() const
{
    return m_expression;
}

Nodes ExpressionStatement::children() const
{
    return { m_expression };
}

std::string ExpressionStatement::to_string() const
{
    return m_expression->to_string();
}

// -- Return ----------------------------------------------------------------

Return::Return(Token token, std::shared_ptr<Expression> expression)
    : Statement(std::move(token))
    , m_expression(std::move(expression))
{
}

Nodes Return::children() const
{
    if (m_expression)
        return { m_expression };
    return {};
}

std::string Return::to_string() const
{
    std::string ret = "return";
    if (m_expression)
        ret = format("{} {}", ret, m_expression->to_string());
    return ret;
}

std::shared_ptr<Expression> const& Return::expression() const
{
    return m_expression;
}

}
