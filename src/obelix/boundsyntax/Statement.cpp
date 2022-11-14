/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/boundsyntax/Statement.h>
#include <obelix/boundsyntax/Function.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundStatement --------------------------------------------------------

BoundStatement::BoundStatement(Token token)
    : Statement(std::move(token))
{
}

// -- BoundExpression -------------------------------------------------------

BoundExpression::BoundExpression(Token token, std::shared_ptr<ObjectType> type)
    : SyntaxNode(std::move(token))
    , m_type(std::move(type))
{
}

BoundExpression::BoundExpression(std::shared_ptr<Expression> const& expr, std::shared_ptr<ObjectType> type)
    : SyntaxNode(expr->token())
    , m_type(std::move(type))
{
}

BoundExpression::BoundExpression(std::shared_ptr<BoundExpression> const& expr)
    : SyntaxNode(expr->token())
    , m_type(expr->type())
{
}

BoundExpression::BoundExpression(Token token, PrimitiveType type)
    : SyntaxNode(std::move(token))
    , m_type(ObjectType::get(type))
{
}

std::shared_ptr<ObjectType> const& BoundExpression::type() const
{
    return m_type;
}

std::string const& BoundExpression::type_name() const
{
    return m_type->name();
}

std::string BoundExpression::attributes() const
{
    return format(R"(type="{}")", type()->name());
}

std::string BoundExpression::qualified_name() const
{
    fatal("Cannot call qualified_name() on nodes of type {}", node_type());
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

bool BoundModule::is_fully_bound() const
{
    return block()->is_fully_bound();
}

int BoundModule::unbound_statements() const
{
    return block()->unbound_statements();
}

std::string BoundModule::qualified_name() const
{
    return name();
}

// -- BoundCompilation ------------------------------------------------------

BoundCompilation::BoundCompilation(BoundModules modules, ObjectTypes const& custom_types, std::string main_module)
    : BoundExpression(Token {}, PrimitiveType::Compilation)
    , m_modules(std::move(modules))
    , m_main_module(std::move(main_module))
{
    for (auto const& module : m_modules) {
        if (module->name() == "/")
            m_root = module;
        else if (module->name() == m_main_module)
            m_main = module;
    }
    for (auto const& type : custom_types)
        m_custom_types.push_back(std::make_shared<BoundType>(Token {}, type));
}

BoundCompilation::BoundCompilation(BoundModules modules, BoundTypes custom_types, std::string main_module)
    : BoundExpression(Token {}, PrimitiveType::Compilation)
    , m_modules(std::move(modules))
    , m_custom_types(std::move(custom_types))
    , m_main_module(std::move(main_module))
{
    for (auto const& module : m_modules) {
        if (module->name() == "/")
            m_root = module;
        else if (module->name() == m_main_module)
            m_main = module;
    }
}

BoundModules const& BoundCompilation::modules() const
{
    return m_modules;
}

BoundTypes const& BoundCompilation::custom_types() const
{
    return m_custom_types;
}

std::shared_ptr<BoundModule> const& BoundCompilation::root() const
{
    return m_root;
}

std::shared_ptr<BoundModule> const& BoundCompilation::main() const
{
    return m_main;
}

std::string const& BoundCompilation::main_module() const
{
    return m_main_module;
}

Nodes BoundCompilation::children() const
{
    BoundModules modules;
    for (auto const& m : m_modules) {
        if (m->name() != "/")
            modules.push_back(m);
    }
    return Nodes { std::make_shared<NodeList<BoundModule>>("modules", modules), std::make_shared<NodeList<BoundType>>("types", m_custom_types) };
}

std::string BoundCompilation::to_string() const
{
    auto ret = format("boundcompilation {}", main_module());
    for (auto const& module : m_modules) {
        ret += "\n";
        ret += "  " + module->to_string();
    }
    return ret;
}

std::string BoundCompilation::attributes() const
{
    return format("main=\"{}\"", main_module());
}

std::string BoundCompilation::root_to_xml() const
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

bool BoundCompilation::is_fully_bound() const
{
    return std::all_of(m_modules.begin(), m_modules.end(),
        [](pBoundModule const& module) {
            return module->is_fully_bound();
        });
}

int BoundCompilation::unbound_statements() const
{
    int ret = 0;
    for (auto const& module : m_modules) {
        ret += module->unbound_statements();
    }
    return ret;
}

}
