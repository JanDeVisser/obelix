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

    bind_types_map[SyntaxNodeType::VariableDeclaration] = [](std::shared_ptr<SyntaxNode> const& tree, BindContext& ctx) -> ErrorOrNode {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        if (var_decl->expression() && var_decl->expression()->is_typed()) {
            auto expr = var_decl->expression();
            ctx.set(var_decl->variable().identifier(), var_decl->variable().type());
            return std::make_shared<VariableDeclaration>(var_decl->variable().identifier(), expr->type(), expr);
        }
        return tree;
    };

    bind_types_map[SyntaxNodeType::BinaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree, BindContext& ctx) -> ErrorOrNode {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        if (expr->lhs()->is_typed() && expr->rhs()->is_typed()) {
            auto lhs = expr->lhs();
            auto rhs = expr->rhs();
            std::vector<ObelixType> arg_types;
            arg_types.push_back(rhs->type());
            auto lhs_type = ObjectType::get(lhs->type());
            assert(lhs_type.has_value());
            auto return_type = lhs_type.value()->return_type_of(expr->op().value(), arg_types);
            return std::make_shared<BinaryExpression>(expr, return_type);
        }
        if ((expr->lhs()->node_type() == SyntaxNodeType::This) && (expr->rhs()->node_type() == SyntaxNodeType::Identifier)) {
            auto identifier = std::dynamic_pointer_cast<Identifier>(expr->rhs());
            if (auto type_maybe = ctx.get(identifier->name()); type_maybe.has_value()) {
                return std::make_shared<BinaryExpression>(expr, type_maybe.value());
            }
        }
        return expr;
    };

    bind_types_map[SyntaxNodeType::UnaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree, BindContext& ctx) -> ErrorOrNode {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        if (expr->operand()->is_typed()) {
            auto operand_type = ObjectType::get(expr->operand()->type());
            assert(operand_type.has_value());
            auto return_type = operand_type.value()->return_type_of(expr->op().value(), std::vector<ObelixType>());
            return std::make_shared<UnaryExpression>(expr, return_type);
        }
        return expr;
    };

    BindContext root(bind_types_map);
    return process_tree(tree, root);
}

}
