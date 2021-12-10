/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/Processor.h>

namespace Obelix {

ErrorOrNode bind_types(std::shared_ptr<SyntaxNode> const& tree)
{
    using BindContext = Context<ObelixType>;
    BindContext::ProcessorMap bind_types_map;

    bind_types_map[SyntaxNodeType::VariableDeclaration] = [](std::shared_ptr<SyntaxNode> const& tree, BindContext& ctx) -> ErrorOrNode
    {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        if (var_decl->expression() && var_decl->expression()->node_type() == SyntaxNodeType::TypedExpression) {
            ctx.set(var_decl->variable().identifier(), var_decl->variable().type());
            auto expr = std::dynamic_pointer_cast<TypedExpression>(var_decl->expression());
            return std::make_shared<VariableDeclaration>(var_decl->variable().identifier(), expr->type(), expr);
        }
        return tree;
    };

    bind_types_map[SyntaxNodeType::BinaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree, BindContext& ctx) -> ErrorOrNode
    {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        if (expr->lhs()->node_type() == SyntaxNodeType::TypedExpression && expr->rhs()->node_type() == SyntaxNodeType::TypedExpression) {
            auto lhs = std::dynamic_pointer_cast<TypedExpression>(expr->lhs());
            auto rhs = std::dynamic_pointer_cast<TypedExpression>(expr->rhs());
            std::vector<ObelixType> arg_types;
            arg_types.push_back(rhs->type());
            auto lhs_type = ObjectType::get(lhs->type());
            assert(lhs_type.has_value());
            auto return_type = lhs_type.value()->return_type_of(expr->op().value(), arg_types);
            return std::make_shared<TypedExpression>(expr, return_type);
        }
        if ((expr->lhs()->node_type() == SyntaxNodeType::This) && (expr->rhs()->node_type() == SyntaxNodeType::Identifier)) {
            auto identifier = std::dynamic_pointer_cast<Identifier>(expr->rhs());
            if (auto type_maybe = ctx.get(identifier->name()); type_maybe.has_value()) {
                return std::make_shared<TypedExpression>(expr, type_maybe.value());
            }
        }
        return expr;
    };

    bind_types_map[SyntaxNodeType::Literal] = [](std::shared_ptr<SyntaxNode> const& tree, BindContext& ctx) -> ErrorOrNode
    {
        auto literal = std::dynamic_pointer_cast<Literal>(tree);
        return std::make_shared<TypedExpression>(literal, literal->literal()->type());
    };

    BindContext root(bind_types_map);
    return process_tree(tree, root);
}

}