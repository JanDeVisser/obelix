/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>

#include <core/Error.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/MaterializedSyntaxNode.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

#define TRY_AND_CAST(cls, expr)                                                                       \
    ({                                                                                                \
        auto __##var##_maybe = (expr);                                                                \
        if (__##var##_maybe.is_error()) {                                                             \
            debug(parser, "Processing node results in error '{}' instead of node of type '" #cls "'", \
                __##var##_maybe.error());                                                             \
            return __##var##_maybe.error();                                                           \
        }                                                                                             \
        auto __##var##_processed = __##var##_maybe.value();                                           \
        std::shared_ptr<cls> __##var##_casted = nullptr;                                              \
        if (__##var##_processed != nullptr) {                                                         \
            __##var##_casted = std::dynamic_pointer_cast<cls>(__##var##_processed);                   \
            if (__##var##_casted == nullptr) {                                                        \
                return SyntaxError {                                                                  \
                    ErrorCode::InternalError,                                                         \
                    format("Processing node results in unexpected type '{}' instead of '" #cls "'",   \
                        __##var##_processed->node_type())                                             \
                };                                                                                    \
            }                                                                                         \
        }                                                                                             \
        __##var##_casted;                                                                             \
    })

#define TRY_AND_TRY_CAST(cls, expr)                                                                   \
    ({                                                                                                \
        auto __##var##_maybe = (expr);                                                                \
        if (__##var##_maybe.is_error()) {                                                             \
            debug(parser, "Processing node results in error '{}' instead of node of type '" #cls "'", \
                __##var##_maybe.error());                                                             \
            return __##var##_maybe.error();                                                           \
        }                                                                                             \
        auto __##var##_processed = __##var##_maybe.value();                                           \
        std::shared_ptr<cls> __##var##_casted = nullptr;                                              \
        if (__##var##_processed != nullptr) {                                                         \
            __##var##_casted = std::dynamic_pointer_cast<cls>(__##var##_processed);                   \
        }                                                                                             \
        __##var##_casted;                                                                             \
    })

using ErrorOrNode = ErrorOr<std::shared_ptr<SyntaxNode>, SyntaxError>;

template<typename Context, typename Processor>
ErrorOrNode process_tree(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, Processor processor)
{
    if (!tree)
        return tree;

    std::shared_ptr<SyntaxNode> ret = tree;

    switch (tree->node_type()) {

    case SyntaxNodeType::Compilation: {
        auto compilation = std::dynamic_pointer_cast<Compilation>(tree);

        Statements statements;
        for (auto& stmt : compilation->statements()) {
            auto new_statement = TRY_AND_CAST(Statement, processor(stmt, ctx));
            statements.push_back(new_statement);
        }

        Modules modules;
        for (auto& module : compilation->modules()) {
            modules.push_back(TRY_AND_CAST(Module, processor(module, ctx)));
        }
        ret = make_node<Compilation>(statements, modules);
        break;
    }

    case SyntaxNodeType::Module: {
        auto module = std::dynamic_pointer_cast<Module>(tree);
        ret = TRY(process_block<Module>(tree, ctx, processor, module->name()));
        break;
    }

    case SyntaxNodeType::Block: {
        ret = TRY(process_block<Block>(tree, ctx, processor));
        break;
    }

    case SyntaxNodeType::FunctionBlock: {
        ret = TRY(process_block<FunctionBlock>(tree, ctx, processor));
        break;
    }

    case SyntaxNodeType::ExpressionType: {
        auto expr_type = std::dynamic_pointer_cast<ExpressionType>(tree);
        TemplateArgumentNodes arguments;
        for (auto& arg : expr_type->template_arguments()) {
            auto processed_arg = TRY_AND_CAST(ExpressionType, processor(arg, ctx));
            arguments.push_back(arg);
        }
        return std::make_shared<ExpressionType>(expr_type->token(), expr_type->type_name(), arguments);
    }

    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        auto func_decl = TRY_AND_CAST(FunctionDecl, processor(func_def->declaration(), ctx));
        auto statement = func_def->statement();
        if (statement)
            statement = TRY_AND_CAST(Statement, processor(statement, ctx));
        ret = std::make_shared<FunctionDef>(func_def->token(), func_decl, statement);
        break;
    }

    case SyntaxNodeType::BoundFunctionDef: {
        auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
        auto func_decl = TRY_AND_CAST(BoundFunctionDecl, processor(func_def->declaration(), ctx));
        auto statement = func_def->statement();
        if (statement)
            statement = TRY_AND_CAST(Statement, processor(statement, ctx));
        ret = std::make_shared<BoundFunctionDef>(func_def->token(), func_decl, statement);
        break;
    }

    case SyntaxNodeType::FunctionDecl:
    case SyntaxNodeType::NativeFunctionDecl:
    case SyntaxNodeType::IntrinsicDecl: {
        auto func_decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
        auto identifier = TRY_AND_CAST(Identifier, processor(func_decl->identifier(), ctx));
        Identifiers parameters;
        for (auto& param : func_decl->parameters()) {
            auto processed_param = TRY_AND_CAST(Identifier, processor(param, ctx));
            parameters.push_back(processed_param);
        }
        switch (func_decl->node_type()) {
        case SyntaxNodeType::FunctionDecl:
            ret = std::make_shared<FunctionDecl>(func_decl->token(), identifier, parameters);
            break;
        case SyntaxNodeType::NativeFunctionDecl:
            ret = std::make_shared<NativeFunctionDecl>(func_decl->token(), identifier, parameters,
                std::dynamic_pointer_cast<NativeFunctionDecl>(func_decl)->native_function_name());
            break;
        case SyntaxNodeType::IntrinsicDecl:
            ret = std::make_shared<IntrinsicDecl>(func_decl->token(), identifier, parameters);
            break;
        default:
            fatal("Unreachable");
        }
        break;
    }

    case SyntaxNodeType::BoundFunctionDecl:
    case SyntaxNodeType::BoundNativeFunctionDecl:
    case SyntaxNodeType::BoundIntrinsicDecl: {
        auto func_decl = std::dynamic_pointer_cast<BoundFunctionDecl>(tree);
        auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(func_decl->identifier());
        BoundIdentifiers parameters;
        for (auto& param : func_decl->parameters()) {
            auto processed_param = std::dynamic_pointer_cast<BoundIdentifier>(param);
            parameters.push_back(processed_param);
        }
        switch (func_decl->node_type()) {
        case SyntaxNodeType::BoundFunctionDecl:
            ret = std::make_shared<BoundFunctionDecl>(func_decl, identifier, parameters);
            break;
        case SyntaxNodeType::BoundNativeFunctionDecl:
            ret = std::make_shared<BoundNativeFunctionDecl>(std::dynamic_pointer_cast<BoundNativeFunctionDecl>(func_decl), identifier, parameters);
            break;
        case SyntaxNodeType::BoundIntrinsicDecl:
            ret = std::make_shared<BoundIntrinsicDecl>(func_decl, identifier, parameters);
            break;
        default:
            fatal("Unreachable");
        }
        break;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        auto expr = TRY_AND_CAST(Expression, processor(stmt->expression(), ctx));
        ret = std::make_shared<ExpressionStatement>(expr);
        break;
    }

    case SyntaxNodeType::BoundExpressionStatement: {
        auto stmt = std::dynamic_pointer_cast<BoundExpressionStatement>(tree);
        auto expr = TRY_AND_CAST(BoundExpression, processor(stmt->expression(), ctx));
        ret = std::make_shared<BoundExpressionStatement>(stmt->token(), expr);
        break;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);

        auto lhs = TRY_AND_CAST(Expression, processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(Expression, processor(expr->rhs(), ctx));
        ret = std::make_shared<BinaryExpression>(lhs, expr->op(), rhs, expr->type());
        break;
    }

    case SyntaxNodeType::BoundBinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);

        auto lhs = TRY_AND_CAST(BoundExpression, processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(BoundExpression, processor(expr->rhs(), ctx));
        ret = std::make_shared<BoundBinaryExpression>(expr->token(), lhs, expr->op(), rhs, expr->type());
        break;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, processor(expr->operand(), ctx));
        ret = std::make_shared<UnaryExpression>(expr->op(), operand, expr->type());
        break;
    }

    case SyntaxNodeType::BoundUnaryExpression: {
        auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
        auto operand = TRY_AND_CAST(BoundExpression, processor(expr->operand(), ctx));
        ret = std::make_shared<BoundUnaryExpression>(expr->token(), operand, expr->op(), expr->type());
        break;
    }

    case SyntaxNodeType::BoundAssignment: {
        auto assignment = std::dynamic_pointer_cast<BoundAssignment>(tree);
        auto assignee = TRY_AND_CAST(BoundVariableAccess, processor(assignment->assignee(), ctx));
        auto expression = TRY_AND_CAST(BoundExpression, processor(assignment->expression(), ctx));
        ret = std::make_shared<BoundAssignment>(assignment->token(), assignee, expression);
        break;
    }

    case SyntaxNodeType::FunctionCall: {
        auto func_call = std::dynamic_pointer_cast<FunctionCall>(tree);
        auto arguments = TRY(xform_expressions(func_call->arguments(), ctx, processor));
        ret = std::make_shared<FunctionCall>(func_call->token(), func_call->name(), arguments);
        break;
    }

    case SyntaxNodeType::BoundFunctionCall: {
        auto func_call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);
        auto arguments = TRY(xform_bound_expressions(func_call->arguments(), ctx, processor));
        ret = std::make_shared<BoundFunctionCall>(func_call, arguments);
        break;
    }

    case SyntaxNodeType::BoundNativeFunctionCall: {
        auto func_call = std::dynamic_pointer_cast<BoundNativeFunctionCall>(tree);
        auto arguments = TRY(xform_bound_expressions(func_call->arguments(), ctx, processor));
        ret = std::make_shared<BoundNativeFunctionCall>(func_call, arguments);
        break;
    }

    case SyntaxNodeType::BoundIntrinsicCall: {
        auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);
        auto arguments = TRY(xform_bound_expressions(call->arguments(), ctx, processor));
        ret = std::make_shared<BoundIntrinsicCall>(call, arguments);
        break;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(Expression, processor(var_decl->expression(), ctx));
        ret = std::make_shared<VariableDeclaration>(var_decl->token(), var_decl->identifier(), expr);
        break;
    }

    case SyntaxNodeType::BoundVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
        auto identifier = TRY_AND_CAST(BoundIdentifier, processor(var_decl->variable(), ctx));
        auto expr = TRY_AND_CAST(BoundExpression, processor(var_decl->expression(), ctx));
        ret = std::make_shared<BoundVariableDeclaration>(var_decl->token(), identifier, var_decl->is_const(), expr);
        break;
    }

    case SyntaxNodeType::Return: {
        auto return_stmt = std::dynamic_pointer_cast<Return>(tree);
        auto expr = TRY_AND_CAST(Expression, processor(return_stmt->expression(), ctx));
        ret = std::make_shared<Return>(return_stmt->token(), expr);
        break;
    }

    case SyntaxNodeType::BoundReturn: {
        auto return_stmt = std::dynamic_pointer_cast<BoundReturn>(tree);
        auto expr = TRY_AND_CAST(BoundExpression, processor(return_stmt->expression(), ctx));
        ret = std::make_shared<BoundReturn>(return_stmt, expr);
        break;
    }

    case SyntaxNodeType::Branch: {
        ErrorOrNode ret_or_error = process_branch<Branch, Expression>(tree, ctx, processor);
        if (ret_or_error.is_error())
            return ret_or_error.error();
        ret = ret_or_error.value();
        break;
    }

    case SyntaxNodeType::BoundBranch: {
        ErrorOrNode ret_or_error = process_branch<BoundBranch, BoundExpression>(tree, ctx, processor);
        if (ret_or_error.is_error())
            return ret_or_error.error();
        ret = ret_or_error.value();
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
        ret = std::make_shared<IfStatement>(if_stmt->token(), branches);
        break;
    }

    case SyntaxNodeType::BoundIfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<BoundIfStatement>(tree);
        BoundBranches branches;
        for (auto& branch : if_stmt->branches()) {
            auto branch_maybe = processor(branch, ctx);
            if (branch_maybe.has_value())
                branches.push_back(std::dynamic_pointer_cast<BoundBranch>(branch_maybe.value()));
        }
        ret = std::make_shared<BoundIfStatement>(if_stmt->token(), branches);
        break;
    }

    case SyntaxNodeType::WhileStatement: {
        auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
        auto condition = TRY_AND_CAST(Expression, processor(while_stmt->condition(), ctx));
        auto stmt = TRY_AND_CAST(Statement, processor(while_stmt->statement(), ctx));
        if ((condition != while_stmt->condition()) || (stmt != while_stmt->statement()))
            ret = std::make_shared<WhileStatement>(while_stmt->token(), condition, stmt);
        break;
    }

    case SyntaxNodeType::BoundWhileStatement: {
        auto while_stmt = std::dynamic_pointer_cast<BoundWhileStatement>(tree);
        auto condition = TRY_AND_CAST(BoundExpression, processor(while_stmt->condition(), ctx));
        auto stmt = TRY_AND_CAST(Statement, processor(while_stmt->statement(), ctx));
        if ((condition != while_stmt->condition()) || (stmt != while_stmt->statement()))
            ret = std::make_shared<BoundWhileStatement>(while_stmt, condition, stmt);
        break;
    }

    case SyntaxNodeType::ForStatement: {
        auto for_stmt = std::dynamic_pointer_cast<ForStatement>(tree);
        auto range = TRY_AND_CAST(Expression, processor(for_stmt->range(), ctx));
        auto stmt = TRY_AND_CAST(Statement, processor(for_stmt->statement(), ctx));
        ret = std::make_shared<ForStatement>(for_stmt->token(), for_stmt->variable(), range, stmt);
        break;
    }

    case SyntaxNodeType::BoundForStatement: {
        auto for_stmt = std::dynamic_pointer_cast<BoundForStatement>(tree);
        auto range = TRY_AND_CAST(BoundExpression, processor(for_stmt->range(), ctx));
        auto stmt = TRY_AND_CAST(Statement, processor(for_stmt->statement(), ctx));
        ret = std::make_shared<BoundForStatement>(for_stmt, range, stmt);
        break;
    }

    case SyntaxNodeType::CaseStatement: {
        ErrorOrNode ret_or_error = process_branch<CaseStatement, Expression>(tree, ctx, processor);
        if (ret_or_error.is_error())
            return ret_or_error.error();
        ret = ret_or_error.value();
        break;
    }

    case SyntaxNodeType::BoundCaseStatement: {
        ErrorOrNode ret_or_error = process_branch<BoundCaseStatement, BoundExpression>(tree, ctx, processor);
        if (ret_or_error.is_error())
            return ret_or_error.error();
        ret = ret_or_error.value();
        break;
    }

    case SyntaxNodeType::DefaultCase: {
        ErrorOrNode ret_or_error = process_branch<DefaultCase, Expression>(tree, ctx, processor);
        if (ret_or_error.is_error())
            return ret_or_error.error();
        ret = ret_or_error.value();
        break;
    }

    case SyntaxNodeType::BoundDefaultCase: {
        ErrorOrNode ret_or_error = process_branch<BoundDefaultCase, BoundExpression>(tree, ctx, processor);
        if (ret_or_error.is_error())
            return ret_or_error.error();
        ret = ret_or_error.value();
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
        ret = std::make_shared<SwitchStatement>(switch_stmt->token(), expr, cases, default_case);
        break;
    }

    case SyntaxNodeType::BoundSwitchStatement: {
        auto switch_stmt = std::dynamic_pointer_cast<BoundSwitchStatement>(tree);
        auto expr = TRY_AND_CAST(BoundExpression, processor(switch_stmt->expression(), ctx));
        BoundCaseStatements cases;
        for (auto& case_stmt : switch_stmt->cases()) {
            cases.push_back(TRY_AND_CAST(BoundCaseStatement, processor(case_stmt, ctx)));
        }
        auto default_case = TRY_AND_CAST(BoundDefaultCase, processor(switch_stmt->default_case(), ctx));
        ret = std::make_shared<BoundSwitchStatement>(switch_stmt, expr, cases, default_case);
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
        return std::make_shared<StmtClass>(tree->token(), statements, std::forward<Args>(args)...);
    else
        return tree;
}

template<class BranchClass, class ExprClass, typename Context, typename Processor, typename... Args>
ErrorOrNode process_branch(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, Processor processor, Args&&... args)
{
    auto branch = std::dynamic_pointer_cast<BranchClass>(tree);
    std::shared_ptr<ExprClass> condition { nullptr };
    if (branch->condition())
        condition = TRY_AND_CAST(ExprClass, processor(branch->condition(), ctx));
    auto statement = TRY_AND_CAST(Statement, processor(branch->statement(), ctx));
    return std::make_shared<BranchClass>(branch, condition, statement, std::forward<Args>(args)...);
}

template<typename Context, typename Processor>
ErrorOr<Expressions, SyntaxError> xform_expressions(Expressions const& expressions, Context& ctx, Processor processor)
{
    Expressions ret;
    for (auto& expr : expressions) {
        auto new_expr = TRY(processor(expr, ctx));
        ret.push_back(std::dynamic_pointer_cast<Expression>(new_expr));
    }
    return ret;
};

template<typename Context, typename Processor>
ErrorOr<BoundExpressions, SyntaxError> xform_bound_expressions(BoundExpressions const& expressions, Context& ctx, Processor processor)
{
    BoundExpressions ret;
    for (auto& expr : expressions) {
        auto new_expr = processor(expr, ctx);
        if (new_expr.is_error())
            return new_expr.error();
        ret.push_back(std::dynamic_pointer_cast<BoundExpression>(new_expr.value()));
    }
    return ret;
};

template<typename Ctx>
ErrorOrNode process(std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx)
{
    if (!tree)
        return tree;
    std::string log_message;
    debug(parser, "Process <{} {}>", tree->node_type(), tree);
    switch (tree->node_type()) {
#undef __SYNTAXNODETYPE
#define __SYNTAXNODETYPE(type)                                                \
    case SyntaxNodeType::type: {                                              \
        log_message = format("<{} {}> => ", #type, tree);                     \
        ErrorOrNode ret = process_node<Ctx, SyntaxNodeType::type>(tree, ctx); \
        if (ret.is_error()) {                                                 \
            log_message += format("Error {}", ret.error());                   \
            ctx.add_error(ret.error());                                       \
        } else {                                                              \
            log_message += format("<{} {}>", tree->node_type(), tree);        \
        }                                                                     \
        debug(parser, "{}", log_message);                                     \
        return ret;                                                           \
    }
        ENUMERATE_SYNTAXNODETYPES(__SYNTAXNODETYPE)
#undef __SYNTAXNODETYPE
    default:
        fatal("Unkown SyntaxNodeType '{}'", (int) tree->node_type());
    }
}

template<typename Ctx>
ErrorOrNode process(std::shared_ptr<SyntaxNode> const& tree)
{
    Ctx ctx;
    return process<Ctx>(tree, ctx);
}

template<typename Ctx, SyntaxNodeType node_type>
ErrorOrNode process_node(std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx)
{
    return process_tree(tree, ctx, [](std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx) {
        return process(tree, ctx);
    });
}

#define INIT_NODE_PROCESSOR(context_type)                                                           \
    using ContextType = context_type;                                                               \
    ErrorOrNode context_type##_processor(std::shared_ptr<SyntaxNode> const& tree, ContextType& ctx) \
    {                                                                                               \
        return process<context_type>(tree, ctx);                                                    \
    }

#define NODE_PROCESSOR(node_type) \
    template<>                    \
    ErrorOrNode process_node<ContextType, SyntaxNodeType::node_type>(std::shared_ptr<SyntaxNode> const& tree, ContextType& ctx)

#define ALIAS_NODE_PROCESSOR(node_type, alias_node_type)                                                                        \
    template<>                                                                                                                  \
    ErrorOrNode process_node<ContextType, SyntaxNodeType::node_type>(std::shared_ptr<SyntaxNode> const& tree, ContextType& ctx) \
    {                                                                                                                           \
        return process_node<ContextType, SyntaxNodeType::alias_node_type>(tree, ctx);                                           \
    }

ErrorOrNode fold_constants(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode bind_types(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode lower(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode resolve_operators(std::shared_ptr<SyntaxNode> const&);

}
