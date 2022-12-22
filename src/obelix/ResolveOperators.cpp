/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/Syntax.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

using ResolveOperatorContext = Context<bool>;

INIT_NODE_PROCESSOR(ResolveOperatorContext);

NODE_PROCESSOR(BoundUnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, expr->operand(), ctx);

    IntrinsicType intrinsic;
    std::shared_ptr<BoundFunctionDecl> decl;
    switch (expr->op()) {
    case UnaryOperator::Identity: {
        return operand;
    }
    case UnaryOperator::Dereference: {
        auto method_descr = operand->type()->get_method(Operator::Dereference);
        if (method_descr == nullptr)
            return SyntaxError { expr->location(), ErrorCode::InternalError, format("No method defined for unary operator {}::{}", operand->type()->to_string(), expr->op()) };
        decl = method_descr->declaration();
        intrinsic = IntrinsicType::dereference;
        break;
    }
    default: {
        auto method_descr = operand->type()->get_method(to_operator(expr->op()), {});
        if (method_descr == nullptr)
            return SyntaxError { expr->location(), ErrorCode::InternalError, format("No method defined for unary operator {}::{}", operand->type()->to_string(), expr->op()) };
        decl = method_descr->declaration();
        auto impl = method_descr->implementation(Architecture::C_TRANSPILER);
        if (!impl.is_intrinsic || impl.intrinsic == IntrinsicType::NotIntrinsic)
            return SyntaxError { expr->location(), ErrorCode::InternalError, format("No intrinsic defined for {}", method_descr->name()) };
        intrinsic = impl.intrinsic;
        break;
    }
    }
    return TRY(process(std::make_shared<BoundIntrinsicCall>(expr->location(), std::dynamic_pointer_cast<BoundIntrinsicDecl>(decl), BoundExpressions { operand }, intrinsic), ctx, result));
}

NODE_PROCESSOR(BoundBinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);

    if (expr->op() == BinaryOperator::Range)
        return tree;
    auto lhs = TRY_AND_CAST(BoundExpression, expr->lhs(), ctx);
    auto rhs = TRY_AND_CAST(BoundExpression, expr->rhs(), ctx);

    if (lhs->type()->type() == PrimitiveType::Pointer && (expr->op() == BinaryOperator::Add || expr->op() == BinaryOperator::Subtract)) {
        std::shared_ptr<BoundExpression> offset { nullptr };
        auto target_type = (lhs->type()->is_template_specialization()) ? lhs->type()->template_argument<std::shared_ptr<ObjectType>>("target") : get_type<uint8_t>();

        if (rhs->node_type() == SyntaxNodeType::BoundIntLiteral) {
            auto offset_literal = std::dynamic_pointer_cast<BoundIntLiteral>(rhs);
            auto offset_val = offset_literal->value();
            if (expr->op() == BinaryOperator::Subtract)
                offset_val *= -1;
            offset = std::make_shared<BoundIntLiteral>(rhs->location(), target_type->size() * offset_val);
        } else {
            offset = rhs;
            if (expr->op() == BinaryOperator::Subtract)
                offset = std::make_shared<BoundUnaryExpression>(expr->location(), offset, UnaryOperator::Negate, expr->type());
            auto size = std::make_shared<BoundIntLiteral>(rhs->location(), target_type->size());
            offset = TRY_AND_CAST(BoundExpression, std::make_shared<BoundBinaryExpression>(expr->location(), size, BinaryOperator::Multiply, offset, ObjectType::get("u64")), ctx);
        }
        auto ident = std::make_shared<BoundIdentifier>(Span {}, BinaryOperator_name(expr->op()), lhs->type());
        BoundIdentifiers params;
        params.push_back(std::make_shared<BoundIdentifier>(Span {}, "ptr", lhs->type()));
        params.push_back(std::make_shared<BoundIdentifier>(Span {}, "offset", ObjectType::get("s32")));
        auto decl = std::make_shared<BoundIntrinsicDecl>("/", ident, params);
        return TRY(process(std::make_shared<BoundIntrinsicCall>(expr->location(), decl, BoundExpressions { lhs, offset }, IntrinsicType::ptr_math), ctx, result));
    }

    if ((rhs->node_type() == SyntaxNodeType::BoundIntLiteral) && (rhs->type()->type() == lhs->type()->type()) && (rhs->type()->size() > lhs->type()->size()))
        rhs = TRY(std::dynamic_pointer_cast<BoundIntLiteral>(rhs)->cast(lhs->type()));
    if ((lhs->node_type() == SyntaxNodeType::BoundIntLiteral) && (rhs->type()->type() == lhs->type()->type()) && (lhs->type()->size() > rhs->type()->size()))
        lhs = TRY(std::dynamic_pointer_cast<BoundIntLiteral>(rhs)->cast(rhs->type()));
    auto method_descr = lhs->type()->get_method(to_operator(expr->op()), { rhs->type() });
    if (method_descr == nullptr)
        return SyntaxError { lhs->location(), "No method defined for binary operator {}::{}({})", lhs->type(), expr->op(), rhs->type() };
    auto impl = method_descr->implementation();
    if (!impl.is_intrinsic || impl.intrinsic == IntrinsicType::NotIntrinsic)
        return SyntaxError { lhs->location(), "No intrinsic defined for {}", method_descr->name() };
    auto intrinsic = impl.intrinsic;
    auto decl = method_descr->declaration();
    if (auto intrinsic_decl = std::dynamic_pointer_cast<BoundIntrinsicDecl>(decl); intrinsic_decl != nullptr)
        return TRY(process(std::make_shared<BoundIntrinsicCall>(expr->location(), intrinsic_decl, BoundExpressions { lhs, rhs }, intrinsic), ctx));
    return TRY(process(std::make_shared<BoundFunctionCall>(expr->location(), decl, BoundExpressions { lhs, rhs }), ctx));
}

ProcessResult& resolve_operators(ProcessResult& result)
{
    if (result.is_error())
        return result;
    Config config;
    ResolveOperatorContext ctx(config);
    process<ResolveOperatorContext>(result.value(), ctx, result);
    return result;
}

}
