/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/Intrinsics.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

using BindContext = Context<std::shared_ptr<SyntaxNode>>;

ErrorOrNode bind_types_processor(std::shared_ptr<SyntaxNode> const& tree, BindContext& ctx)
{
    if (!tree)
        return tree;

    switch (tree->node_type()) {

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto t = var_decl->variable().type();
        std::shared_ptr<Expression> expr { nullptr };
        if (var_decl->expression()) {
            expr = TRY_AND_CAST(Expression, bind_types_processor(var_decl->expression(), ctx));

            if (expr->is_typed() && var_decl->is_typed() && var_decl->type()->type_id() != ObelixType::TypeAny && expr->type() != var_decl->type())
                return Error { ErrorCode::TypeMismatch, var_decl->name(), var_decl->type(), expr->type() };
            if (!expr->is_typed() && !var_decl->is_typed())
                return Error { ErrorCode::UntypedVariable, var_decl->name() };

            if (!t || t->type_id() == ObelixType::TypeUnknown)
                t = expr->type();
        } else if (!var_decl->is_typed()) {
            return Error { ErrorCode::UntypedVariable, var_decl->name() };
        }

        auto ret = std::make_shared<VariableDeclaration>(var_decl->name(), t, expr, var_decl->is_const());
        ctx.declare(var_decl->name(), ret);
        debug(parser, "VariableDeclaration: {}", ret->to_string(0));
        return ret;
    }

    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        auto decl = func_def->declaration();
        if (!decl->identifier().type())
            return Error { ErrorCode::UntypedFunction, decl->identifier().identifier() };
        for (auto& param : decl->parameters()) {
            if (!param.type())
                return Error { ErrorCode::UntypedParameter, param.identifier() };
        }
        ctx.declare(decl->identifier().identifier(), decl);
        debug(parser, "FunctionDecl: {}", decl->to_string(0));
        if (decl->node_type() != SyntaxNodeType::FunctionDecl)
            return tree;

        BindContext func_ctx(ctx);
        for (auto& param : decl->parameters()) {
            if (!param.type())
                return Error { ErrorCode::UntypedParameter, param.identifier() };
            func_ctx.declare(param.name(), std::make_shared<VariableDeclaration>(param));
        }
        auto func_block = TRY_AND_CAST(Statement, bind_types_processor(func_def->statement(), func_ctx));
        return std::make_shared<FunctionDef>(decl, func_block);
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        auto lhs = TRY_AND_CAST(Expression, bind_types_processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(Expression, bind_types_processor(expr->rhs(), ctx));
        if (!lhs->is_typed())
            return Error { ErrorCode::UntypedExpression, lhs->to_string(0) };
        if (!rhs->is_typed())
            return Error { ErrorCode::UntypedExpression, rhs->to_string(0) };
        ObelixTypes arg_types;
        arg_types.push_back(static_cast<ObelixType>(rhs->type()->type_id()));
        auto lhs_type = ObjectType::get(static_cast<ObelixType>(lhs->type()->type_id()));
        assert(lhs_type.has_value());
        auto return_type = lhs_type.value()->return_type_of(expr->op().value(), arg_types);
        if (return_type == ObelixType::TypeUnknown)
            return Error { ErrorCode::ReturnTypeUnresolved, expr->to_string() };
        if (expr->op().code() == TokenCode::Equals || Parser::is_assignment_operator(expr->op().code())) {
            if (lhs->node_type() != SyntaxNodeType::Identifier)
                return Error { ErrorCode::CannotAssignToRValue, lhs->to_string() };
            auto identifier = std::dynamic_pointer_cast<Identifier>(lhs);
            auto var_decl_maybe = ctx.get(identifier->name());
            if (!var_decl_maybe.has_value())
                return Error { ErrorCode::UndeclaredVariable, identifier->name() };
            if (var_decl_maybe.value()->node_type() != SyntaxNodeType::VariableDeclaration)
                return Error { ErrorCode::CannotAssignToFunction, identifier->name() };
            auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(var_decl_maybe.value());
            if (var_decl->is_const())
                return Error { ErrorCode::CannotAssignToConstant, var_decl->name() };
            if (var_decl->type() != rhs->type())
                return Error { ErrorCode::TypeMismatch, rhs->to_string(), var_decl->type(), rhs->type() };
        }
        auto ret = std::make_shared<BinaryExpression>(lhs, expr->op(), rhs, return_type);
        debug(parser, "BinaryExpression: {}", ret->to_string());
        return ret;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, bind_types_processor(expr->operand(), ctx));
        if (!operand->is_typed())
            return Error { ErrorCode::UntypedExpression, operand->to_string(0) };
        auto operand_type = ObjectType::get(static_cast<ObelixType>(operand->type()->type_id()));
        if (!operand_type.has_value())
            return Error { ErrorCode::InternalError, format("No type definition for '{}'", operand->type()) };
        assert(operand_type.has_value());
        auto return_type = operand_type.value()->return_type_of(expr->op().value(), std::vector<ObelixType>());
        if (return_type == ObelixType::TypeUnknown)
            return Error { ErrorCode::ReturnTypeUnresolved, expr->to_string() };
        auto ret = std::make_shared<UnaryExpression>(expr->op(), operand, return_type);
        debug(parser, "UnaryExpression: {}", ret->to_string());
        return ret;
    }

    case SyntaxNodeType::Identifier: {
        auto ident = std::dynamic_pointer_cast<Identifier>(tree);
        auto type_decl_maybe = ctx.get(ident->name());
        if (!type_decl_maybe.has_value())
            return Error { ErrorCode::UntypedVariable, ident->name() };
        if (type_decl_maybe.value()->node_type() != SyntaxNodeType::VariableDeclaration)
            return Error { ErrorCode::SyntaxError, format("Function {} cannot be referenced as a variable", ident->name()) };
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(type_decl_maybe.value());
        auto ret = std::make_shared<Identifier>(Symbol { ident->name(), var_decl->type() });
        debug(parser, "Identifier: {}", ret->to_string());
        return ret;
    }

    case SyntaxNodeType::FunctionCall: {
        auto func_call = std::dynamic_pointer_cast<FunctionCall>(tree);
        std::shared_ptr<FunctionDecl> func_decl;
        if (!Intrinsics::is_intrinsic(func_call)) {
            auto type_decl_maybe = ctx.get(func_call->name());
            if (!type_decl_maybe.has_value())
                return Error { ErrorCode::UntypedFunction, func_call->name() };
            if (type_decl_maybe.value()->node_type() == SyntaxNodeType::VariableDeclaration)
                return Error { ErrorCode::SyntaxError, format("Variable {} cannot be called", func_call->name()) };
            func_decl = std::dynamic_pointer_cast<FunctionDecl>(type_decl_maybe.value());
        } else {
            auto intrinsic = Intrinsics::get_intrinsic(func_call);
            func_decl = intrinsic.declaration;
        }
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

        auto ret = std::make_shared<FunctionCall>(Symbol { func_call->name(), func_decl->type() }, args);
        if (func_decl->node_type() == SyntaxNodeType::NativeFunctionDecl) {
            ret = std::make_shared<NativeFunctionCall>(std::dynamic_pointer_cast<NativeFunctionDecl>(func_decl), ret);
        }
        debug(parser, "FunctionCall: {}", ret->to_string());
        return ret;
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
