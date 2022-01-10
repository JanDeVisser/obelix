/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/Processor.h>

namespace Obelix {

using BindContext = Context<std::shared_ptr<SyntaxNode>>;

ErrorOrNode bind_types_processor(std::shared_ptr<SyntaxNode> const& tree, BindContext& ctx)
{
    if (!tree)
        return tree;

    switch (tree->node_type()) {

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        ObelixType t = var_decl->variable().type();
        std::shared_ptr<Expression> expr { nullptr };
        if (var_decl->expression()) {
            expr = TRY_AND_CAST(Expression, bind_types_processor(var_decl->expression(), ctx));

            if (expr->is_typed() && var_decl->is_typed() && var_decl->type() != ObelixType::TypeAny && expr->type() != var_decl->type())
                return Error { ErrorCode::TypeMismatch, var_decl->name(), var_decl->type(), expr->type() };
            if (!expr->is_typed() && !var_decl->is_typed())
                return Error { ErrorCode::UntypedVariable, var_decl->name() };

            if (t == ObelixType::TypeUnknown)
                t = expr->type();
        } else if (!var_decl->is_typed()) {
            return Error { ErrorCode::UntypedVariable, var_decl->name() };
        }

        auto ret = std::make_shared<VariableDeclaration>(var_decl->name(), t, expr);
        ctx.set(var_decl->name(), ret);
        return ret;
    }

    case SyntaxNodeType::FunctionDecl: {
        auto decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
        if (decl->identifier().type() == ObelixType::TypeUnknown)
            return Error { ErrorCode::UntypedFunction, decl->identifier().identifier() };
        ctx.set(decl->identifier().identifier(), decl);
        return decl;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        auto lhs = TRY_AND_CAST(Expression, bind_types_processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(Expression, bind_types_processor(expr->rhs(), ctx));
        if (lhs->is_typed() && rhs->is_typed()) {
            std::vector<ObelixType> arg_types;
            arg_types.push_back(rhs->type());
            auto lhs_type = ObjectType::get(lhs->type());
            assert(lhs_type.has_value());
            auto return_type = lhs_type.value()->return_type_of(expr->op().value(), arg_types);
            return std::make_shared<BinaryExpression>(lhs, expr->op(), rhs, return_type);
        }
        return expr;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, bind_types_processor(expr->operand(), ctx));
        if (operand->is_typed()) {
            auto operand_type = ObjectType::get(expr->operand()->type());
            assert(operand_type.has_value());
            auto return_type = operand_type.value()->return_type_of(expr->op().value(), std::vector<ObelixType>());
            return std::make_shared<UnaryExpression>(expr, return_type);
        }
        return expr;
    }

    case SyntaxNodeType::Identifier: {
        auto ident = std::dynamic_pointer_cast<Identifier>(tree);
        auto type_decl_maybe = ctx.get(ident->name());
        if (!type_decl_maybe.has_value())
            return Error { ErrorCode::UntypedVariable, ident->name() };
        if (type_decl_maybe.value()->node_type() != SyntaxNodeType::VariableDeclaration)
            return Error { ErrorCode::SyntaxError, format("Function {} cannot be referenced as a variable", ident->name()) };
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(type_decl_maybe.value());
        return std::make_shared<Identifier>(Symbol { ident->name(), var_decl->type() });
    }

    case SyntaxNodeType::FunctionCall: {
        auto func_call = std::dynamic_pointer_cast<FunctionCall>(tree);
        auto type_decl_maybe = ctx.get(func_call->name());
        if (!type_decl_maybe.has_value())
            return Error { ErrorCode::UntypedFunction, func_call->name() };
        if (type_decl_maybe.value()->node_type() != SyntaxNodeType::FunctionDecl)
            return Error { ErrorCode::SyntaxError, format("Variable {} cannot be called", func_call->name()) };
        auto func_decl = std::dynamic_pointer_cast<FunctionDecl>(type_decl_maybe.value());

        if (func_call->arguments().size() != func_decl->parameters().size())
            return Error { ErrorCode::ArgumentCountMismatch, func_call->name(), func_call->arguments().size() };

        Expressions args;
        for (auto ix = 0; ix < func_call->arguments().size(); ix++) {
            auto& arg = func_call->arguments().at(ix);
            auto& param = func_decl->parameters().at(ix);
            auto a = TRY_AND_CAST(Expression, bind_types_processor(arg, ctx));
            if (a->type() != param.type())
                return Error { ErrorCode::ArgumentTypeMismatch, func_call->name(), a->type(), param.identifier() };

            args.push_back(a);
        }

        return std::make_shared<FunctionCall>(Symbol { func_call->name(), func_decl->type() }, args);
    }

    default:
        return process_tree(tree, ctx, bind_types_processor);
    }
}

ErrorOrNode bind_types(std::shared_ptr<SyntaxNode> const& tree)
{
    BindContext root;
    return bind_types_processor(tree, root);
}

}
