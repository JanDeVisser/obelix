/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/Error.h"
#include "obelix/Syntax.h"
#include <memory>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>
#include <obelix/SyntaxNodeType.h>
#include <obelix/interp/Context.h>
#include <obelix/interp/InterpIntrinsics.h>

namespace Obelix {

extern_logging_category(parser);

ErrorOrNode interpret(std::shared_ptr<SyntaxNode> expr, InterpContext &ctx)
{
    return process<InterpContext>(expr, ctx);
}

ErrorOrNode interpret(std::shared_ptr<SyntaxNode> expr)
{
    InterpContext ctx;
    return interpret(expr, ctx);
}

INIT_NODE_PROCESSOR(InterpContext);

NODE_PROCESSOR(BoundVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
    auto expr = TRY_AND_CAST(BoundExpression, process(var_decl->expression(), ctx));
    auto literal = std::dynamic_pointer_cast<BoundLiteral>(expr);
    if (var_decl->is_const() && (literal != nullptr)) {
        ctx.declare(var_decl->name(), literal);
        return make_node<Pass>(var_decl);
    }
    switch (var_decl->node_type()) {
    case SyntaxNodeType::BoundVariableDeclaration:
        return make_node<BoundVariableDeclaration>(var_decl->token(), var_decl->variable(), var_decl->is_const(), expr);
    case SyntaxNodeType::BoundStaticVariableDeclaration:
        return make_node<BoundStaticVariableDeclaration>(var_decl->token(), var_decl->variable(), var_decl->is_const(), expr);
    default:
        fatal("Unreachable: node type = {}", var_decl->node_type());
    }
}

ALIAS_NODE_PROCESSOR(BoundStaticVariableDeclaration, BoundVariableDeclaration);

NODE_PROCESSOR(BoundVariable)
{
    auto variable = std::dynamic_pointer_cast<BoundVariable>(tree);
    if (auto constant_maybe = ctx.get(variable->name()); constant_maybe.has_value()) {
        return constant_maybe.value();
    }
    return tree;
}

NODE_PROCESSOR(BoundIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);
    BoundLiterals processed;
    for (auto const& arg : call->arguments()) {
        auto literal = TRY_AND_TRY_CAST(BoundLiteral, process(arg, ctx));
        if (literal == nullptr)
            return tree;
        processed.push_back(literal);
    }
    auto intrinsic = get_interp_intrinsic(call->intrinsic());
    if (intrinsic == nullptr)
        return tree;
    return intrinsic(ctx, processed);
}

}
