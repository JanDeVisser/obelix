/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/boundsyntax/Function.h>
#include <obelix/boundsyntax/Variable.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundVariableAccess ---------------------------------------------------

BoundVariableAccess::BoundVariableAccess(std::shared_ptr<Expression> reference, std::shared_ptr<ObjectType> type)
    : BoundExpression(std::move(reference), std::move(type))
{
}

BoundVariableAccess::BoundVariableAccess(Token token, std::shared_ptr<ObjectType> type)
    : BoundExpression(std::move(token), std::move(type))
{
}

// -- BoundIdentifier -------------------------------------------------------

BoundIdentifier::BoundIdentifier(std::shared_ptr<Identifier> const& identifier, std::shared_ptr<ObjectType> type)
    : BoundVariableAccess(identifier, std::move(type))
    , m_identifier(identifier->name())
{
}

BoundIdentifier::BoundIdentifier(Token token, std::string identifier, std::shared_ptr<ObjectType> type)
    : BoundVariableAccess(std::move(token), std::move(type))
    , m_identifier(std::move(identifier))
{
}

std::string const& BoundIdentifier::name() const
{
    return m_identifier;
}

std::string BoundIdentifier::attributes() const
{
    return format(R"(name="{}" type="{}")", name(), type());
}

std::string BoundIdentifier::to_string() const
{
    return format("{}: {}", name(), type()->to_string());
}

// -- BoundVariable ---------------------------------------------------------

BoundVariable::BoundVariable(std::shared_ptr<Variable> identifier, std::shared_ptr<ObjectType> type)
    : BoundIdentifier(std::move(identifier), std::move(type))
{
}

BoundVariable::BoundVariable(Token token, std::string name, std::shared_ptr<ObjectType> type)
    : BoundIdentifier(std::move(token), std::move(name), std::move(type))
{
}

// -- BoundMemberAccess -----------------------------------------------------

BoundMemberAccess::BoundMemberAccess(std::shared_ptr<BoundExpression> strukt, std::shared_ptr<BoundIdentifier> member)
    : BoundVariableAccess(strukt->token(), member->type())
    , m_struct(std::move(strukt))
    , m_member(std::move(member))
{
}

std::shared_ptr<BoundExpression> const& BoundMemberAccess::structure() const
{
    return m_struct;
}

std::shared_ptr<BoundIdentifier> const& BoundMemberAccess::member() const
{
    return m_member;
}

std::string BoundMemberAccess::attributes() const
{
    return format(R"(type="{}")", type());
}

Nodes BoundMemberAccess::children() const
{
    return { m_struct, m_member };
}

std::string BoundMemberAccess::to_string() const
{
    return format("{}.{}: {}", structure(), member(), type_name());
}

// -- BoundArrayAccess ------------------------------------------------------

BoundArrayAccess::BoundArrayAccess(std::shared_ptr<BoundExpression> array, std::shared_ptr<BoundExpression> index, std::shared_ptr<ObjectType> type)
    : BoundVariableAccess(array->token(), std::move(type))
    , m_array(std::move(array))
    , m_index(std::move(index))
{
}

std::shared_ptr<BoundExpression> const& BoundArrayAccess::array() const
{
    return m_array;
}

std::shared_ptr<BoundExpression> const& BoundArrayAccess::subscript() const
{
    return m_index;
}

std::string BoundArrayAccess::attributes() const
{
    return format(R"(type="{}")", type());
}

Nodes BoundArrayAccess::children() const
{
    return { m_array, m_index };
}

std::string BoundArrayAccess::to_string() const
{
    return format("{}[{}]: {}", array(), subscript(), type_name());
}

// -- BoundVariableDeclaration ----------------------------------------------

BoundVariableDeclaration::BoundVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
    : Statement(decl->token())
    , m_variable(std::move(variable))
    , m_const(decl->is_const())
    , m_expression(std::move(expr))
{
}

BoundVariableDeclaration::BoundVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
    : Statement(std::move(token))
    , m_variable(std::move(variable))
    , m_const(is_const)
    , m_expression(std::move(expr))
{
}

std::string BoundVariableDeclaration::attributes() const
{
    return format(R"(name="{}" type="{}" is_const="{}")", name(), type(), is_const());
}

Nodes BoundVariableDeclaration::children() const
{
    if (m_expression)
        return { m_expression };
    return {};
}

std::string BoundVariableDeclaration::to_string() const
{
    auto ret = format("{} {}: {}", (is_const()) ? "const" : "var", name(), type());
    if (m_expression)
        ret += format(" = {}", m_expression->to_string());
    return ret;
}

std::string const& BoundVariableDeclaration::name() const
{
    return m_variable->name();
}

std::shared_ptr<ObjectType> BoundVariableDeclaration::type() const
{
    return m_variable->type();
}

bool BoundVariableDeclaration::is_const() const
{
    return m_const;
}

std::shared_ptr<BoundExpression> const& BoundVariableDeclaration::expression() const
{
    return m_expression;
}

std::shared_ptr<BoundIdentifier> const& BoundVariableDeclaration::variable() const
{
    return m_variable;
}

// -- BoundStaticVariableDeclaration ----------------------------------------

BoundStaticVariableDeclaration::BoundStaticVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(decl, std::move(variable), std::move(expr))
{
}

BoundStaticVariableDeclaration::BoundStaticVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(std::move(token), std::move(variable), is_const, std::move(expr))
{
}

std::string BoundStaticVariableDeclaration::to_string() const
{
    return "static " + BoundVariableDeclaration::to_string();
}

// -- BoundLocalVariableDeclaration -----------------------------------------

BoundLocalVariableDeclaration::BoundLocalVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(decl, std::move(variable), std::move(expr))
{
}

BoundLocalVariableDeclaration::BoundLocalVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(std::move(token), std::move(variable), is_const, std::move(expr))
{
}

std::string BoundLocalVariableDeclaration::to_string() const
{
    return "global " + BoundVariableDeclaration::to_string();
}

// -- BoundGlobalVariableDeclaration ----------------------------------------

BoundGlobalVariableDeclaration::BoundGlobalVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(decl, std::move(variable), std::move(expr))
{
}

BoundGlobalVariableDeclaration::BoundGlobalVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(std::move(token), std::move(variable), is_const, std::move(expr))
{
}

std::string BoundGlobalVariableDeclaration::to_string() const
{
    return "global " + BoundVariableDeclaration::to_string();
}

// -- BoundAssignment -------------------------------------------------------

BoundAssignment::BoundAssignment(Token token, std::shared_ptr<BoundVariableAccess> assignee, std::shared_ptr<BoundExpression> expression)
    : BoundExpression(std::move(token), assignee->type())
    , m_assignee(std::move(assignee))
    , m_expression(std::move(expression))
{
    debug(parser, "m_identifier->type() = {} m_expression->type() = {}", m_assignee->type(), m_expression->type());
    assert(*(m_assignee->type()) == *(m_expression->type()));
}

std::shared_ptr<BoundVariableAccess> const& BoundAssignment::assignee() const
{
    return m_assignee;
}

std::shared_ptr<BoundExpression> const& BoundAssignment::expression() const
{
    return m_expression;
}

Nodes BoundAssignment::children() const
{
    return { assignee(), expression() };
}

std::string BoundAssignment::to_string() const
{
    return format("{} = {}", assignee()->to_string(), expression()->to_string());
}

// -- BoundModule -----------------------------------------------------------

BoundModule::BoundModule(Token token, std::string name, std::shared_ptr<Block> block, BoundFunctionDecls exports, BoundFunctionDecls imports)
    : BoundExpression(std::move(token), PrimitiveType::Module)
    , m_name(std::move(name))
    , m_block(std::move(block))
    , m_exports(std::move(exports))
    , m_imports(std::move(imports))
{
}

std::string const& BoundModule::name() const
{
    return m_name;
}

std::shared_ptr<Block> const& BoundModule::block() const
{
    return m_block;
}

BoundFunctionDecls const& BoundModule::exports() const
{
    return m_exports;
}

BoundFunctionDecls const& BoundModule::imports() const
{
    return m_imports;
}

std::shared_ptr<BoundFunctionDecl> BoundModule::exported(std::string const& name)
{
    auto it = std::find_if(exports().begin(), exports().end(), [name](auto const& e) {
       return e->name() == name;
    });
    if (it != exports().end())
        return *it;
    return nullptr;
}

std::shared_ptr<BoundFunctionDecl> BoundModule::resolve(std::string const& name, ObjectTypes const& arg_types) const
{
    debug(parser, "resolving function {}({})", name, arg_types);
    std::shared_ptr<BoundFunctionDecl> func_decl = nullptr;
    for (auto const& declaration : exports()) {
        debug(parser, "checking {}({})", declaration->name(), declaration->parameters());
        if (declaration->name() != name)
            continue;
        if (arg_types.size() != declaration->parameters().size())
            continue;

        bool all_matched = true;
        for (auto ix = 0u; all_matched && ix < arg_types.size(); ix++) {
            auto& arg_type = arg_types.at(ix);
            auto& param = declaration->parameters().at(ix);
            if (!arg_type->is_assignable_to(param->type()))
                all_matched = false;
        }
        if (all_matched) {
            func_decl = declaration;
            break;
        }
    }
    if (func_decl != nullptr)
        debug(parser, "resolve() returns {}", *func_decl);
    else
        debug(parser, "No matching function found");
    return func_decl;
}

std::string BoundModule::attributes() const
{
    return format(R"(name="{}")", name());
}

Nodes BoundModule::children() const
{
    Nodes ret;
    BoundFunctionDecls export_list;
    for (auto const& exported : exports()) {
        export_list.push_back(exported);
    }
    ret.push_back(std::make_shared<NodeList<BoundFunctionDecl>>("exports", export_list));
    BoundFunctionDecls import_list;
    for (auto const& imported : imports()) {
        import_list.push_back(imported);
    }
    ret.push_back(std::make_shared<NodeList<BoundFunctionDecl>>("imports", import_list));
    auto statements = block()->children();
    ret.push_back(statements.front());
    return ret;
}

std::string BoundModule::to_string() const
{
    return format("module {}", m_name);
}

// -- BoundCompilation ------------------------------------------------------

BoundCompilation::BoundCompilation(BoundModules modules)
    : BoundExpression(Token {}, PrimitiveType::Compilation)
    , m_modules(std::move(modules))
{
    for (auto const& module : m_modules) {
        if (module->name() == "")
            m_root = module;
    }
}

BoundModules const& BoundCompilation::modules() const
{
    return m_modules;
}

std::shared_ptr<BoundModule> const& BoundCompilation::root() const
{
    return m_root;
}

Nodes BoundCompilation::children() const
{
    Nodes ret;
    for (auto& module : m_modules) {
        ret.push_back(module);
    }
    return ret;
}

std::string BoundCompilation::to_string() const
{
    std::string ret = "boundcompilation";
    for (auto const& module : m_modules) {
        ret += "\n";
        ret += "  " + module->to_string();
    }
    return ret;
}

std::string BoundCompilation::root_to_xml() const
{
    auto indent = 0u;
    auto ret = format("<{}", node_type());
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

}
