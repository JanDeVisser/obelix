/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <unordered_map>

#include <core/Error.h>
#include <core/Object.h>
#include <obelix/Context.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

#define TRY_AND_CAST(cls, expr)                                  \
    ({                                                           \
        auto __##var##_maybe = (expr);                           \
        if (__##var##_maybe.is_error())                          \
            return __##var##_maybe.error();                      \
        std::dynamic_pointer_cast<cls>(__##var##_maybe.value()); \
    })

template<typename Context, typename Processor>
ErrorOrNode process_tree(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, Processor& processor)
{
    auto xform_expressions = [processor](Expressions const& expressions, Context& ctx) -> ErrorOr<Expressions> {
        Expressions ret;
        for (auto& expr : expressions) {
            auto new_expr = processor(expr, ctx);
            if (new_expr.is_error())
                return new_expr.error();
            ret.push_back(std::dynamic_pointer_cast<Expression>(new_expr.value()));
        }
        return ret;
    };

    if (!tree)
        return tree;

    std::shared_ptr<SyntaxNode> ret = tree;

    switch (tree->node_type()) {

    case SyntaxNodeType::Block: {
        ret = TRY(process_block<Block>(tree, ctx, processor));
        break;
    }

    case SyntaxNodeType::FunctionBlock: {
        ret = TRY(process_block<FunctionBlock>(tree, ctx, processor));
        break;
    }

    case SyntaxNodeType::Module: {
        auto module = std::dynamic_pointer_cast<Module>(tree);
        ret = TRY(process_block<Module>(tree, ctx, processor, module->name()));
        break;
    }

    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        auto func_decl = TRY_AND_CAST(FunctionDecl, processor(func_def->declaration(), ctx));
        auto statement = TRY_AND_CAST(Statement, processor(func_def->statement(), ctx));
        if (func_decl != func_def->declaration() || statement != func_def->statement())
            ret = std::make_shared<FunctionDef>(func_decl, statement);
        break;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        auto expr = TRY_AND_CAST(Expression, processor(stmt->expression(), ctx));
        if (expr != stmt->expression())
            ret = std::make_shared<ExpressionStatement>(expr);
        break;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);

        auto lhs = TRY_AND_CAST(Expression, processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(Expression, processor(expr->rhs(), ctx));
        if ((lhs != expr->lhs()) || (rhs != expr->rhs()))
            ret = std::make_shared<BinaryExpression>(lhs, expr->op(), rhs);
        break;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, processor(expr->operand(), ctx));
        if (operand != expr->operand())
            ret = std::make_shared<UnaryExpression>(expr->op(), operand);
        break;
    }

    case SyntaxNodeType::Assignment: {
        auto assignment = std::dynamic_pointer_cast<Assignment>(tree);
        auto identifier = TRY_AND_CAST(Identifier, processor(assignment->identifier(), ctx));
        auto expression = TRY_AND_CAST(Expression, processor(assignment->expression(), ctx));
        if (identifier != assignment->identifier() || expression != assignment->expression())
            ret = std::make_shared<Assignment>(identifier, expression);
        break;
    }

    case SyntaxNodeType::FunctionCall: {
        auto func_call = std::dynamic_pointer_cast<FunctionCall>(tree);
        auto arguments = TRY(xform_expressions(func_call->arguments(), ctx));
        ret = std::make_shared<FunctionCall>(func_call->function(), arguments);
        break;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(Expression, processor(var_decl->expression(), ctx));
        if (expr != var_decl->expression())
            ret = std::make_shared<VariableDeclaration>(var_decl->variable(), expr);
        break;
    }

    case SyntaxNodeType::Return: {
        auto return_stmt = std::dynamic_pointer_cast<Return>(tree);
        auto expr = TRY_AND_CAST(Expression, processor(return_stmt->expression(), ctx));
        if (expr != return_stmt->expression())
            ret = std::make_shared<Return>(expr);
        break;
    }

    case SyntaxNodeType::Branch: {
        ret = TRY(process_branch<Branch>(tree, ctx, processor));
        break;
    }

    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        Branches branches;
        for (auto& branch : if_stmt->branches()) {
            auto branch_maybe = processor(branch, ctx);
            if (branch_maybe.has_value())
                branches.push_back(std::dynamic_pointer_cast<Branch>(branch_maybe.value()));
        }
        ret = std::make_shared<IfStatement>(branches);
        break;
    }

    case SyntaxNodeType::WhileStatement: {
        auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
        auto condition = TRY_AND_CAST(Expression, processor(while_stmt->condition(), ctx));
        auto stmt = TRY_AND_CAST(Statement, processor(while_stmt->statement(), ctx));
        if ((condition != while_stmt->condition()) || (stmt != while_stmt->statement()))
            ret = std::make_shared<WhileStatement>(condition, stmt);
        break;
    }

    case SyntaxNodeType::ForStatement: {
        auto for_stmt = std::dynamic_pointer_cast<ForStatement>(tree);
        auto range = TRY_AND_CAST(Expression, processor(for_stmt->range(), ctx));
        auto stmt = TRY_AND_CAST(Statement, processor(for_stmt->statement(), ctx));
        if ((range != for_stmt->range()) || (stmt != for_stmt->statement()))
            ret = std::make_shared<ForStatement>(for_stmt->variable(), range, stmt);
        break;
    }

    case SyntaxNodeType::CaseStatement: {
        ret = TRY(process_branch<CaseStatement>(tree, ctx, processor));
        break;
    }

    case SyntaxNodeType::DefaultCase: {
        ret = TRY(process_branch<DefaultCase>(tree, ctx, processor));
        break;
    }

    case SyntaxNodeType::SwitchStatement: {
        auto switch_stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
        auto expr = TRY_AND_CAST(Expression, processor(switch_stmt->expression(), ctx));
        CaseStatements cases;
        for (auto& case_stmt : switch_stmt->cases()) {
            cases.push_back(TRY_AND_CAST(CaseStatement, processor(case_stmt, ctx)));
        }
        auto default_case = TRY_AND_CAST(DefaultCase, processor(switch_stmt->default_case(), ctx));
        if (expr != switch_stmt->expression() || cases != switch_stmt->cases() || default_case != switch_stmt->default_case())
            ret = std::make_shared<SwitchStatement>(expr, cases, default_case);
        break;
    }

    default:
        break;
    }

    return ret;
}

template<class StmtClass, typename Context, typename Processor, typename... Args>
ErrorOrNode process_block(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, Processor& processor, Args&&... args)
{
    auto block = std::dynamic_pointer_cast<Block>(tree);
    Context child_ctx(ctx);
    Statements statements;
    for (auto& stmt : block->statements()) {
        auto new_statement = TRY_AND_CAST(Statement, processor(stmt, child_ctx));
        statements.push_back(new_statement);
    }
    if (statements != block->statements())
        return std::make_shared<StmtClass>(statements, std::forward<Args>(args)...);
    else
        return tree;
}

template<class BranchClass, typename Context, typename Processor, typename... Args>
ErrorOrNode process_branch(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, Processor processor, Args&&... args)
{
    auto branch = std::dynamic_pointer_cast<Branch>(tree);
    std::shared_ptr<Expression> condition { nullptr };
    if (branch->condition())
        condition = TRY_AND_CAST(Expression, processor(branch->condition(), ctx));
    auto statement = TRY_AND_CAST(Statement, processor(branch->statement(), ctx));
    return std::make_shared<BranchClass>(condition, statement, std::forward<Args>(args)...);
}

ErrorOrNode fold_constants(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode bind_types(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode lower(std::shared_ptr<SyntaxNode> const&);

}
