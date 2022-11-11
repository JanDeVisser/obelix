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

std::shared_ptr<Statement> const& Pass::elided_statement() const
{
    return m_elided_statement;
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
    ret.push_back(std::make_shared<NodeList<Statement>>("statements", m_statements));
    return ret;
}
std::string Block::to_string() const
{
    return format("[ ... {} statements ... ]", m_statements.size());
}

bool Block::is_fully_bound() const
{
    return std::all_of(m_statements.begin(), m_statements.end(),
            [](pStatement const& stmt) {
                return stmt->is_fully_bound();
            });
}

int Block::unbound_statements() const
{
    int ret = 0;
    for (auto const& stmt : m_statements) {
        if (!stmt->is_fully_bound()) ret++;
    }
    return ret;
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

Compilation::Compilation(Modules modules, std::string main_module)
    : SyntaxNode(Token {})
    , m_modules(std::move(modules))
    , m_main_module(std::move(main_module))
{
    for (auto const& module : m_modules) {
        if (module->name() == "/")
            m_root = module;
    }
}

Compilation::Compilation(std::string main_module)
    : SyntaxNode(Token {})
    , m_main_module(std::move(main_module))
{
}

Modules const& Compilation::modules() const
{
    return m_modules;
}

std::shared_ptr<Module> const& Compilation::root() const
{
    return m_root;
}

std::string const& Compilation::main_module() const
{
    return m_main_module;
}

Nodes Compilation::children() const
{
    Nodes ret;
    for (auto& module : m_modules) {
        ret.push_back(module);
    }
    return ret;
}

std::string Compilation::attributes() const
{
    return format("main=\"{}\"", m_main_module);
}

std::string Compilation::to_string() const
{
    auto ret = std::string("compilation ") + main_module();
    for (auto const& module : m_modules) {
        ret += "\n";
        ret += "  " + module->to_string();
    }
    return ret;
}

std::string Compilation::root_to_xml() const
{
    auto indent = 0u;
    auto ret = format("<{} {}", node_type(), attributes());
    auto child_nodes = children();
    if (child_nodes.empty())
        return ret + "/>";
    ret += ">\n";
    for (auto const& child : child_nodes) {
        ret += child->to_xml(indent + 2);
        ret += "\n";
    }
    return ret + format("</{}>", node_type());
}

bool Compilation::is_fully_bound() const
{
    return std::all_of(m_modules.begin(), m_modules.end(),
            [](pModule const& module) {
                return module->is_fully_bound();
            });
}

int Compilation::unbound_statements() const
{
    int ret = 0;
    for (auto const& module : m_modules) {
        ret += module->unbound_statements();
    }
    return ret;
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

Return::Return(Token token, std::shared_ptr<Expression> expression, bool return_error)
    : Statement(std::move(token))
    , m_expression(std::move(expression))
    , m_return_error(return_error)
{
}

std::string Return::attributes() const
{
    return format(R"(return_error="{}")", return_error());
}

Nodes Return::children() const
{
    if (m_expression)
        return { m_expression };
    return {};
}

std::string Return::to_string() const
{
    std::string ret = (return_error()) ? "error" : "return";
    if (m_expression)
        ret = format("{} {}", ret, m_expression->to_string());
    return ret;
}

std::shared_ptr<Expression> const& Return::expression() const
{
    return m_expression;
}

bool Return::return_error() const
{
    return m_return_error;
}

}
