/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "lexer/Token.h"
#include "obelix/Operator.h"
#include "obelix/SyntaxNodeType.h"
#include "obelix/Type.h"
#include <memory>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Intrinsics.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

class BindContext : public Context<std::shared_ptr<SyntaxNode>> {
public:
    std::shared_ptr<ObjectType> return_type { nullptr };
};

INIT_NODE_PROCESSOR(BindContext);

NODE_PROCESSOR(StructDefinition)
{
    auto struct_def = std::dynamic_pointer_cast<StructDefinition>(tree);
    if (struct_def->fields().empty())
        return Error { ErrorCode::SyntaxError, format("Struct {} has no fields", struct_def->name()) };
    BoundIdentifiers bound_fields;
    FieldDefs field_defs {};
    for (auto const& field : struct_def->fields()) {
        auto field_type = TRY(field->type()->resolve_type());
        bound_fields.push_back(make_node<BoundIdentifier>(field, field_type));
        field_defs.emplace_back(field->name(), field_type);
    }
    auto type = TRY(ObjectType::make_type(struct_def->name(), field_defs));
    return make_node<BoundStructDefinition>(struct_def, type);
});

NODE_PROCESSOR(VariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
    auto t = var_decl->type();
    std::shared_ptr<ObjectType> var_type { nullptr };
    if (t)
        var_type = TRY(t->resolve_type());
    std::shared_ptr<BoundExpression> expr { nullptr };
    if (var_decl->expression()) {
        auto processed_expr = TRY(processor.process(var_decl->expression(), ctx));
        assert(processed_expr != nullptr);
        auto processed_expr_as_bound_expr = std::dynamic_pointer_cast<BoundExpression>(processed_expr);
        if (processed_expr_as_bound_expr == nullptr) {
            return Error(ErrorCode::UntypedExpression, processed_expr);
        }
        expr = std::dynamic_pointer_cast<BoundExpression>(processed_expr);

        if (var_type && var_type->type() != PrimitiveType::Any && (*(expr->type()) != *var_type))
            return Error { ErrorCode::TypeMismatch, var_decl->name(), var_decl->type(), expr->type() };
        if (!var_type)
            var_type = expr->type();
    } else if (!var_type) {
        return Error { ErrorCode::UntypedVariable, var_decl->name() };
    }

    auto identifier = make_node<BoundIdentifier>(var_decl->identifier(), var_type);
    auto ret = make_node<BoundVariableDeclaration>(var_decl, identifier, expr);
    ctx.declare(var_decl->name(), ret);
    return ret;
});

NODE_PROCESSOR(FunctionDecl)
{
    auto decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
    if (decl->type() == nullptr)
        return Error { ErrorCode::UntypedFunction, decl->name() };

    auto ret_type_or_error = decl->type()->resolve_type();
    if (ret_type_or_error.is_error())
        return ret_type_or_error.error();
    auto ret_type = ret_type_or_error.value();

    auto identifier = make_node<BoundIdentifier>(decl->identifier(), ret_type);
    BoundIdentifiers bound_parameters;
    for (auto& parameter : decl->parameters()) {
        if (parameter->type() == nullptr)
            return Error { ErrorCode::UntypedParameter, parameter->name() };
        auto parameter_type = TRY(parameter->type()->resolve_type());
        bound_parameters.push_back(make_node<BoundIdentifier>(parameter, parameter_type));
    }
    switch (decl->node_type()) {
    case SyntaxNodeType::IntrinsicDecl:
        return make_node<BoundIntrinsicDecl>(std::dynamic_pointer_cast<IntrinsicDecl>(decl), identifier, bound_parameters);
    case SyntaxNodeType::NativeFunctionDecl:
        return make_node<BoundNativeFunctionDecl>(std::dynamic_pointer_cast<NativeFunctionDecl>(decl), identifier, bound_parameters);
    default:
        return make_node<BoundFunctionDecl>(decl, identifier, bound_parameters);
    }
});

ALIAS_NODE_PROCESSOR(IntrinsicDecl, FunctionDecl);
ALIAS_NODE_PROCESSOR(NativeFunctionDecl, FunctionDecl);

NODE_PROCESSOR(FunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
    auto decl = TRY_AND_CAST(BoundFunctionDecl, processor.process(func_def->declaration(), ctx));
    ctx.declare(decl->name(), decl);

    std::shared_ptr<Statement> func_block { nullptr };
    if (func_def->statement()) {
        BindContext func_ctx(ctx);
        for (auto& param : decl->parameters()) {
            auto dummy_decl = make_node<VariableDeclaration>(param->token(), make_node<Identifier>(param->token(), param->name()));
            func_ctx.declare(param->name(), make_node<BoundVariableDeclaration>(dummy_decl, param, nullptr));
        }
        func_ctx.return_type = decl->type();
        func_block = TRY_AND_CAST(Statement, processor.process(func_def->statement(), func_ctx));
    }
    return make_node<BoundFunctionDef>(func_def, decl, func_block);
});

NODE_PROCESSOR(BinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
    auto lhs = TRY_AND_CAST(BoundExpression, processor.process(expr->lhs(), ctx));
    auto rhs = TRY(processor.process(expr->rhs(), ctx));
    auto rhs_bound = std::dynamic_pointer_cast<BoundExpression>(rhs);

    struct BinaryOperatorMap {
        TokenCode code;
        BinaryOperator op;
    };

    constexpr static BinaryOperatorMap operator_map[] = {
        { TokenCode::Plus, BinaryOperator::Add },
        { TokenCode::Minus, BinaryOperator::Subtract },
        { TokenCode::Asterisk, BinaryOperator::Multiply },
        { TokenCode::Slash, BinaryOperator::Divide },
        { TokenCode::Percent, BinaryOperator::Modulo },
        { TokenCode::Equals, BinaryOperator::Assign },
        { TokenCode::EqualsTo, BinaryOperator::Equals },
        { TokenCode::NotEqualTo, BinaryOperator::NotEquals },
        { TokenCode::GreaterEqualThan, BinaryOperator::GreaterEquals },
        { TokenCode::LessEqualThan, BinaryOperator::LessEquals },
        { TokenCode::GreaterThan, BinaryOperator::Greater },
        { TokenCode::LessThan, BinaryOperator::Less },
        { TokenCode::LogicalAnd, BinaryOperator::LogicalAnd },
        { TokenCode::LogicalOr, BinaryOperator::LogicalOr },
        { TokenCode::Ampersand, BinaryOperator::BitwiseAnd },
        { TokenCode::Pipe, BinaryOperator::BitwiseOr },
        { TokenCode::Hat, BinaryOperator::BitwiseXor },
        { TokenCode::Period, BinaryOperator::MemberAccess },
        { Parser::KeywordIncEquals, BinaryOperator::BinaryIncrement },
        { Parser::KeywordDecEquals, BinaryOperator::BinaryDecrement },
        { Parser::KeywordRange, BinaryOperator::Range },
    };

    BinaryOperator op = BinaryOperator::Invalid;
    for (auto ix : operator_map) {
        if (ix.code == expr->op().code()) {
            op = ix.op;
            break;
        }
    }
    if (op == BinaryOperator::Invalid)
        return Error { ErrorCode::OperatorUnresolved, expr->op().value(), lhs->to_string() };

    if (op == BinaryOperator::MemberAccess) {
        if (lhs->type()->type() != PrimitiveType::Struct)
            return Error { ErrorCode::CannotAccessMember, lhs->to_string() };
        if (rhs->node_type() != SyntaxNodeType::Identifier)
            return Error { ErrorCode::NotMember, rhs, lhs };
        auto ident = std::dynamic_pointer_cast<Identifier>(rhs);
        auto field = lhs->type()->field(ident->name());
        if (field.type->type() == PrimitiveType::Unknown)
            return Error { ErrorCode::NotMember, rhs, lhs };
        auto member_literal = make_node<BoundLiteral>(rhs->token(), ident->name());
        return std::make_shared<BoundMemberAccess>(lhs, member_literal, field.type);
    }

    if (rhs->node_type() == SyntaxNodeType::Identifier) {
        auto ident = std::dynamic_pointer_cast<Identifier>(rhs);
        return Error { ErrorCode::UndeclaredVariable, ident->name() };
    }

    if (rhs_bound == nullptr)
        return Error { ErrorCode::UntypedExpression, rhs->to_string() };

    if (op == BinaryOperator::Assign || BinaryOperator_is_assignment(op)) {
        auto assignee = std::dynamic_pointer_cast<BoundVariableAccess>(lhs);
        if (assignee == nullptr)
            return Error { ErrorCode::CannotAssignToRValue, lhs->to_string() };
        auto lhs_as_ident = std::dynamic_pointer_cast<BoundIdentifier>(assignee);
        if (lhs_as_ident) {
            auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(lhs);
            auto var_decl_maybe = ctx.get(identifier->name());
            if (!var_decl_maybe.has_value())
                return Error { ErrorCode::UndeclaredVariable, identifier->name() };
            if (var_decl_maybe.value()->node_type() != SyntaxNodeType::BoundVariableDeclaration)
                return Error { ErrorCode::CannotAssignToFunction, identifier->name() };
            auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(var_decl_maybe.value());
            if (var_decl->is_const())
                return Error { ErrorCode::CannotAssignToConstant, var_decl->name() };
        }
        if (*(lhs->type()) != *(rhs_bound->type()))
            return Error { ErrorCode::TypeMismatch, rhs->to_string(), lhs->type(), rhs_bound->type() };

        if (op == BinaryOperator::Assign)
            return make_node<BoundAssignment>(expr->token(), assignee, rhs_bound);

        // +=, -= and friends: rewrite to a straight-up assignment to a binary
        auto new_rhs = make_node<BoundBinaryExpression>(expr->token(),
            lhs, BinaryOperator_for_assignment_operator(op), rhs_bound, rhs_bound->type());
        return make_node<BoundAssignment>(expr->token(), assignee, new_rhs);
    }

    auto return_type = lhs->type()->return_type_of(to_operator(op), rhs_bound->type());
    if (!return_type.has_value())
        return Error { ErrorCode::ReturnTypeUnresolved, format("{} {} {}", lhs, op, rhs) };
    return make_node<BoundBinaryExpression>(expr, lhs, op, rhs_bound, return_type.value());
});

NODE_PROCESSOR(UnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, processor.process(expr->operand(), ctx));

    struct UnaryOperatorMap {
        TokenCode code;
        UnaryOperator op;
    };

    constexpr static UnaryOperatorMap operator_map[8] = {
        { TokenCode::Asterisk, UnaryOperator::Dereference },
        { TokenCode::AtSign, UnaryOperator::AddressOf },
        { TokenCode::Plus, UnaryOperator::Identity },
        { TokenCode::Minus, UnaryOperator::Negate },
        { TokenCode::ExclamationPoint, UnaryOperator::LogicalInvert },
        { TokenCode::UnaryIncrement, UnaryOperator::UnaryIncrement },
        { TokenCode::UnaryDecrement, UnaryOperator::UnaryDecrement },
        { TokenCode::Tilde, UnaryOperator::BitwiseInvert },
    };

    UnaryOperator op { UnaryOperator::InvalidUnary };
    for (auto ix : operator_map) {
        if (ix.code == expr->op().code()) {
            op = ix.op;
            break;
        }
    }
    if (op == UnaryOperator::InvalidUnary)
        return Error { ErrorCode::OperatorUnresolved, expr->op().value(), operand->to_string() };

    auto return_type = operand->type()->return_type_of(to_operator(op));
    if (!return_type.has_value())
        return Error { ErrorCode::ReturnTypeUnresolved, format("{} {}", op, operand) };
    return make_node<BoundUnaryExpression>(expr, operand, op, return_type.value());
});

NODE_PROCESSOR(Identifier)
{
    auto ident = std::dynamic_pointer_cast<Identifier>(tree);
    auto type_decl_maybe = ctx.get(ident->name());
    if (!type_decl_maybe.has_value())
        return tree;
    if (type_decl_maybe.value()->node_type() != SyntaxNodeType::BoundVariableDeclaration)
        return Error { ErrorCode::SyntaxError, format("Function {} cannot be referenced as a variable", ident->name()) };
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(type_decl_maybe.value());
    return make_node<BoundIdentifier>(ident, var_decl->type());
});

NODE_PROCESSOR(Literal)
{
    auto literal = std::dynamic_pointer_cast<Literal>(tree);
    return make_node<BoundLiteral>(literal);
});

NODE_PROCESSOR(FunctionCall)
{
    auto func_call = std::dynamic_pointer_cast<FunctionCall>(tree);
    std::shared_ptr<BoundFunctionDecl> func_decl;

    BoundExpressions args;
    ObjectTypes arg_types;
    for (auto& arg : func_call->arguments()) {
        auto processed_arg = TRY_AND_CAST(BoundExpression, processor.process(arg, ctx));
        args.push_back(processed_arg);
        arg_types.push_back(processed_arg->type());
    }

    auto type_decl_maybe = ctx.get(func_call->name());
    if (!type_decl_maybe.has_value())
        return Error { ErrorCode::UntypedFunction, func_call->name() };
    if (type_decl_maybe.value()->node_type() == SyntaxNodeType::BoundVariableDeclaration)
        return Error { ErrorCode::SyntaxError, format("Variable {} cannot be called", func_call->name()) };
    func_decl = std::dynamic_pointer_cast<BoundFunctionDecl>(type_decl_maybe.value());
    if (args.size() != func_decl->parameters().size())
        return Error { ErrorCode::ArgumentCountMismatch, func_call->name(), func_call->arguments().size() };

    for (auto ix = 0; ix < args.size(); ix++) {
        auto& arg = args.at(ix);
        auto& param = func_decl->parameters().at(ix);
        if (*(arg->type()) != *(param->type()))
            return Error { ErrorCode::ArgumentTypeMismatch, func_call->name(), arg->type(), param->name() };
    }

    switch (func_decl->node_type()) {
    case SyntaxNodeType::BoundIntrinsicDecl: {
        auto intrinsic = IntrinsicType_by_name(func_decl->name());
        if (intrinsic == IntrinsicType::NotIntrinsic)
            return Error { ErrorCode::SyntaxError, format("Intrinsic {} not defined", func_decl->name()) };
        return make_node<BoundIntrinsicCall>(func_call, intrinsic, args, func_decl->type());
    }
    case SyntaxNodeType::BoundNativeFunctionDecl:
        return make_node<BoundNativeFunctionCall>(func_call, args, std::dynamic_pointer_cast<BoundNativeFunctionDecl>(func_decl));
    default:
        return make_node<BoundFunctionCall>(func_call, args, func_decl);
    }
});

NODE_PROCESSOR(ExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
    auto bound_expr = TRY_AND_CAST(BoundExpression, processor.process(expr_stmt->expression(), ctx));
    return make_node<BoundExpressionStatement>(expr_stmt, bound_expr);
});

NODE_PROCESSOR(Return)
{
    auto ret_stmt = std::dynamic_pointer_cast<Return>(tree);
    auto bound_expr = TRY_AND_CAST(BoundExpression, processor.process(ret_stmt->expression(), ctx));
    if (ctx.return_type) {
        if (!bound_expr)
            return Error { ErrorCode::SyntaxError, format("Expected return value of type '{}', got a void return", ctx.return_type->name()) };
        if (ctx.return_type->type() != bound_expr->type()->type())
            return Error { ErrorCode::TypeMismatch, bound_expr->to_string(), ctx.return_type->name(), bound_expr->type_name() };
    } else {
        if (bound_expr)
            return Error { ErrorCode::SyntaxError, format("Expected void return, got a return value of type '{}'", bound_expr->type_name()) };
    }
    return make_node<BoundReturn>(ret_stmt, bound_expr);
});

NODE_PROCESSOR(Branch)
{
    auto branch = std::dynamic_pointer_cast<Branch>(tree);
    auto bound_condition = TRY_AND_CAST(BoundExpression, processor.process(branch->condition(), ctx));
    auto bound_statement = TRY_AND_CAST(Statement, processor.process(branch->statement(), ctx));
    return make_node<BoundBranch>(branch, bound_condition, bound_statement);
});

NODE_PROCESSOR(IfStatement)
{
    auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);

    BoundBranches bound_branches;
    for (auto& branch : if_stmt->branches()) {
        auto bound_branch = TRY_AND_CAST(BoundBranch, processor.process(branch, ctx));
        bound_branches.push_back(bound_branch);
    }
    auto bound_else_stmt = TRY_AND_CAST(Statement, processor.process(if_stmt->else_stmt(), ctx));
    return make_node<BoundIfStatement>(if_stmt, bound_branches, bound_else_stmt);
});

NODE_PROCESSOR(WhileStatement)
{
    auto stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
    auto bound_condition = TRY_AND_CAST(BoundExpression, processor.process(stmt->condition(), ctx));
    auto bound_statement = TRY_AND_CAST(Statement, processor.process(stmt->statement(), ctx));
    return make_node<BoundWhileStatement>(stmt, bound_condition, bound_statement);
});

NODE_PROCESSOR(ForStatement)
{
    auto stmt = std::dynamic_pointer_cast<ForStatement>(tree);
    auto bound_range = TRY_AND_CAST(BoundExpression, processor.process(stmt->range(), ctx));
    if (bound_range->node_type() != SyntaxNodeType::BoundBinaryExpression)
        return Error { ErrorCode::SyntaxError, "Invalid for-loop range" };
    auto range_binary_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(bound_range);
    if (range_binary_expr->op() != BinaryOperator::Range)
        return Error { ErrorCode::SyntaxError, "Invalid for-loop range" };
    auto variable_type = range_binary_expr->lhs()->type();

    bool must_declare_variable = true;
    auto type_decl_maybe = ctx.get(stmt->variable());
    if (type_decl_maybe.has_value()) {
        if (type_decl_maybe.value()->node_type() == SyntaxNodeType::BoundVariableDeclaration) {
            auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(type_decl_maybe.value());
            if (*(var_decl->type()) != *variable_type) {
                return Error { ErrorCode::TypeMismatch, var_decl->name(), var_decl->type(), variable_type };
            }
            must_declare_variable = false;
        }
    }

    BindContext for_ctx(ctx);
    if (must_declare_variable)
        for_ctx.declare(stmt->variable(),
            std::make_shared<BoundVariableDeclaration>(stmt->token(),
                std::make_shared<BoundIdentifier>(stmt->token(), stmt->variable(), variable_type),
                false, nullptr));
    auto bound_statement = TRY_AND_CAST(Statement, processor.process(stmt->statement(), for_ctx));
    return make_node<BoundForStatement>(stmt, bound_range, bound_statement, must_declare_variable);
});

NODE_PROCESSOR(CaseStatement)
{
    auto branch = std::dynamic_pointer_cast<CaseStatement>(tree);
    auto bound_condition = TRY_AND_CAST(BoundExpression, processor.process(branch->condition(), ctx));
    auto bound_statement = TRY_AND_CAST(Statement, processor.process(branch->statement(), ctx));
    return make_node<BoundCaseStatement>(branch, bound_condition, bound_statement);
});

NODE_PROCESSOR(DefaultCase)
{
    auto branch = std::dynamic_pointer_cast<DefaultCase>(tree);
    auto bound_statement = TRY_AND_CAST(Statement, processor.process(branch->statement(), ctx));
    return make_node<BoundDefaultCase>(branch, bound_statement);
});

NODE_PROCESSOR(SwitchStatement)
{
    auto stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
    auto bound_expression = TRY_AND_CAST(BoundExpression, processor.process(stmt->expression(), ctx));
    BoundCaseStatements bound_case_statements;
    for (auto& case_stmt : stmt->cases()) {
        auto bound_case = TRY_AND_CAST(BoundCaseStatement, processor.process(case_stmt, ctx));
        bound_case_statements.push_back(bound_case);
    }
    auto bound_default_case = TRY_AND_CAST(BoundDefaultCase, processor.process(stmt->default_case(), ctx));
    return make_node<BoundSwitchStatement>(stmt, bound_expression, bound_case_statements, bound_default_case);
});

ErrorOrNode bind_types(std::shared_ptr<SyntaxNode> const& tree)
{
    return processor_for_context<BindContext>(tree);
}

}
