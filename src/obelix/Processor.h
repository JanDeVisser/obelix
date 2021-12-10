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
            return tree;                                         \
        std::dynamic_pointer_cast<cls>(__##var##_maybe.value()); \
    })

#define ENUMERATE_FLOWCONTROLS(S) \
    S(None)                       \
    S(Break)                      \
    S(Continue)                   \
    S(Return)                     \
    S(Goto)                       \
    S(Label)                      \
    S(Skipped)

enum class FlowControl {
#undef __ENUM_FLOWCONTROL
#define __ENUM_FLOWCONTROL(fc) fc,
    ENUMERATE_FLOWCONTROLS(__ENUM_FLOWCONTROL)
#undef __ENUM_FLOWCONTROL
};

constexpr std::string_view FlowControl_name(FlowControl fc)
{
    switch (fc) {
#undef __ENUM_FLOWCONTROL
#define __ENUM_FLOWCONTROL(fc) \
    case FlowControl::fc:      \
        return #fc;
        ENUMERATE_FLOWCONTROLS(__ENUM_FLOWCONTROL)
#undef __ENUM_FLOWCONTROL
    }
}

class StatementExecutionResult : public SyntaxNode {
public:
    explicit StatementExecutionResult(Obj result = Object::null(), FlowControl flow_control = FlowControl::None)
        : SyntaxNode()
        , m_flow_control(flow_control)
        , m_result(std::move(result))
    {
    }

    [[nodiscard]] std::string to_string(int) const override { return format("{} [{}]", result()->to_string(), FlowControl_name(flow_control())); }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::StatementExecutionResult; }

    [[nodiscard]] FlowControl flow_control() const { return m_flow_control; }
    [[nodiscard]] Obj const& result() const { return m_result; }

private:
    FlowControl m_flow_control;
    Obj m_result;
};

inline std::shared_ptr<StatementExecutionResult> execution_ok()
{
    static std::shared_ptr<StatementExecutionResult> s_ok = std::make_shared<StatementExecutionResult>();
    return s_ok;
}

inline std::shared_ptr<StatementExecutionResult> execution_evaluates_to(Obj return_value)
{
    return std::make_shared<StatementExecutionResult>(std::move(return_value));
}

inline std::shared_ptr<StatementExecutionResult> return_result(Obj return_value)
{
    return std::make_shared<StatementExecutionResult>(std::move(return_value), FlowControl::Return);
}

inline std::shared_ptr<StatementExecutionResult> break_loop()
{
    static std::shared_ptr<StatementExecutionResult> s_break = std::make_shared<StatementExecutionResult>(Object::null(), FlowControl::Break);
    return s_break;
}

inline std::shared_ptr<StatementExecutionResult> continue_loop()
{
    static std::shared_ptr<StatementExecutionResult> s_continue = std::make_shared<StatementExecutionResult>(Object::null(), FlowControl::Continue);
    return s_continue;
}

inline std::shared_ptr<StatementExecutionResult> skip_block()
{
    static std::shared_ptr<StatementExecutionResult> s_skipped = std::make_shared<StatementExecutionResult>(Object::null(), FlowControl::Skipped);
    return s_skipped;
}

inline std::shared_ptr<StatementExecutionResult> goto_label(int id)
{
    return std::make_shared<StatementExecutionResult>(make_obj<Integer>(id), FlowControl::Goto);
}

inline std::shared_ptr<StatementExecutionResult> mark_label(int id)
{
    return std::make_shared<StatementExecutionResult>(make_obj<Integer>(id), FlowControl::Label);
}

template<typename Context>
ErrorOrNode process_node(std::shared_ptr<SyntaxNode> const& tree, Context& ctx)
{
    ErrorOrNode ret_or_error = ctx.process(tree);
    if (!ret_or_error.is_error()) {
        auto ret = ret_or_error.value();
        if (ret != tree)
            ret -> set_node_id(tree->node_id());
        if (auto processor_maybe = ctx.processor_for(SyntaxNodeType::Statement); processor_maybe.has_value()) {
            auto new_tree = TRY(ctx.add_if_error(processor_maybe.value()(ret, ctx)));
        }
        return ret;
    } else {
        debug(parser, "process_tree returns error: {}", ret_or_error.error().message());
        return ret_or_error.error();
    }
}

template<typename Context>
ErrorOrNode process_tree(std::shared_ptr<SyntaxNode> const& tree, Context& ctx)
{
    auto xform_expressions = [](Expressions const& expressions, Context& ctx) -> ErrorOr<Expressions> {
        Expressions ret;
        for (auto& expr : expressions) {
            auto new_expr = process_tree(expr, ctx);
            if (new_expr.is_error())
                return new_expr.error();
            ret.push_back(std::dynamic_pointer_cast<Expression>(new_expr.value()));
        }
        return ret;
    };

    if (!tree)
        return tree;

    std::shared_ptr<SyntaxNode> ret = tree;

    if (auto processor_maybe = ctx.processor_for(SyntaxNodeType::SyntaxNode); processor_maybe.has_value()) {
        auto new_tree = TRY(ctx.add_if_error(processor_maybe.value()(tree, ctx)));
    }

    switch (tree->node_type()) {

    case SyntaxNodeType::Block: {
        ret = TRY(process_block<Block>(tree, ctx));
        break;
    }

    case SyntaxNodeType::Module: {
        auto module = std::dynamic_pointer_cast<Module>(tree);
        ret = TRY(process_block<Module>(tree, ctx, module->name()));
        break;
    }

    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        Context child_ctx(&ctx);
        auto statement = TRY_AND_CAST(Statement, process_tree(func_def->statement(), child_ctx));
        if (statement != func_def->statement())
            ret = std::make_shared<FunctionDef>(func_def->identifier(), func_def->parameters(), statement);
        break;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        auto expr = TRY_AND_CAST(Expression, process_tree(stmt->expression(), ctx));
        if (expr != stmt->expression())
            ret = std::make_shared<ExpressionStatement>(expr);
        break;
    }

    case SyntaxNodeType::TypedExpression: {
        auto typed_expression = std::dynamic_pointer_cast<TypedExpression>(tree);
        auto expr = TRY_AND_CAST(Expression, process_tree(typed_expression->expression(), ctx));
        if (expr != typed_expression->expression())
            ret = std::make_shared<TypedExpression>(expr, typed_expression->type());
        break;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);

        auto lhs = TRY_AND_CAST(Expression, process_tree(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(Expression, process_tree(expr->rhs(), ctx));
        if ((lhs != expr->lhs()) || (rhs != expr->rhs()))
            ret = std::make_shared<BinaryExpression>(lhs, expr->op(), rhs);
        break;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, process_tree(expr->operand(), ctx));
        if (operand != expr->operand())
            ret = std::make_shared<UnaryExpression>(expr->op(), operand);
        break;
    }

    case SyntaxNodeType::FunctionCall: {
        auto func_call = std::dynamic_pointer_cast<FunctionCall>(tree);
        auto func = TRY_AND_CAST(Expression, process_tree(func_call->function(), ctx));
        auto arguments = TRY(xform_expressions(func_call->arguments(), ctx));
        if ((func != func_call->function()) || (arguments != func_call->arguments()))
            ret = std::make_shared<FunctionCall>(func, arguments);
        break;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(Expression, process_tree(var_decl->expression(), ctx));
        if (expr != var_decl->expression())
            ret = std::make_shared<VariableDeclaration>(var_decl->variable(), expr);
        break;
    }

    case SyntaxNodeType::Return: {
        auto return_stmt = std::dynamic_pointer_cast<Return>(tree);
        auto expr = TRY_AND_CAST(Expression, process_tree(return_stmt->expression(), ctx));
        if (expr != return_stmt->expression())
            ret = std::make_shared<Return>(expr);
        break;
    }

    case SyntaxNodeType::Branch: {
        ret = TRY(process_branch<Branch>(tree, ctx, false));
        break;
    }

    case SyntaxNodeType::ElseStatement: {
        ret = TRY(process_branch<ElseStatement>(tree, ctx, false));
        break;
    }

    case SyntaxNodeType::ElifStatement: {
        ret = TRY(process_branch<ElifStatement>(tree, ctx, false));
        break;
    }

    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        ElifStatements elifs;
        for (auto& elif : if_stmt->elifs()) {
            auto elif_maybe = process_tree(elif, ctx);
            if (elif_maybe.has_value())
                elifs.push_back(std::dynamic_pointer_cast<ElifStatement>(elif_maybe.value()));
        }
        auto else_stmt = TRY_AND_CAST(ElseStatement, process_tree(if_stmt->else_stmt(), ctx));
        ret = TRY(process_branch<IfStatement>(tree, ctx, elifs != if_stmt->elifs() || else_stmt != if_stmt->else_stmt(), elifs, else_stmt));
        break;
    }

    case SyntaxNodeType::WhileStatement: {
        auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
        auto condition = TRY_AND_CAST(Expression, process_tree(while_stmt->condition(), ctx));
        auto stmt = TRY_AND_CAST(Statement, process_tree(while_stmt->statement(), ctx));
        if ((condition != while_stmt->condition()) || (stmt != while_stmt->statement()))
            ret = std::make_shared<WhileStatement>(condition, stmt);
        break;
    }

    case SyntaxNodeType::ForStatement: {
        auto for_stmt = std::dynamic_pointer_cast<ForStatement>(tree);
        auto range = TRY_AND_CAST(Expression, process_tree(for_stmt->range(), ctx));
        auto stmt = TRY_AND_CAST(Statement, process_tree(for_stmt->statement(), ctx));
        if ((range != for_stmt->range()) || (stmt != for_stmt->statement()))
            ret = std::make_shared<ForStatement>(for_stmt->variable(), range, stmt);
        break;
    }

    case SyntaxNodeType::CaseStatement: {
        ret = TRY(process_branch<CaseStatement>(tree, ctx, false));
        break;
    }

    case SyntaxNodeType::DefaultCase: {
        ret = TRY(process_branch<DefaultCase>(tree, ctx, false));
        break;
    }

    case SyntaxNodeType::SwitchStatement: {
        auto switch_stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
        auto expr = TRY_AND_CAST(Expression, process_tree(switch_stmt->expression(), ctx));
        CaseStatements cases;
        for (auto& case_stmt : switch_stmt->cases()) {
            cases.push_back(TRY_AND_CAST(CaseStatement, process_tree(case_stmt, ctx)));
        }
        auto default_case = TRY_AND_CAST(DefaultCase, process_tree(switch_stmt->default_case(), ctx));
        if (expr != switch_stmt->expression() || cases != switch_stmt->cases() || default_case != switch_stmt->default_case())
            ret = std::make_shared<SwitchStatement>(expr, cases, default_case);
        break;
    }

    default:
        break;
    }

    if (ret != tree)
        ret -> set_node_id(tree->node_id());
    ErrorOrNode ret_or_error = ctx.process(ret);
    if (!ret_or_error.is_error()) {
        ret = ret_or_error.value();
        if (ret != tree)
            ret -> set_node_id(tree->node_id());
        if (auto processor_maybe = ctx.processor_for(SyntaxNodeType::Statement); processor_maybe.has_value()) {
            auto new_tree = TRY(ctx.add_if_error(processor_maybe.value()(ret, ctx)));
        }
        return ret;
    } else {
        debug(parser, "process_tree returns error: {}", ret_or_error.error().message());
        return ret_or_error.error();
    }
}

template<class StmtClass, typename Context, typename... Args>
ErrorOrNode process_block(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, Args&&... args)
{
    auto block = std::dynamic_pointer_cast<Block>(tree);
    Context child_ctx(ctx);
    std::shared_ptr<StatementExecutionResult> result;
    Statements statements;
    for (auto& stmt : block->statements()) {
        auto new_statement = TRY_AND_CAST(Statement, process_tree(stmt, child_ctx));
        statements.push_back(new_statement);
    }
    if (statements != block->statements())
        return std::make_shared<StmtClass>(statements, std::forward<Args>(args)...);
    else
        return tree;
}

template<class BranchClass, typename Context, typename... Args>
ErrorOrNode process_branch(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, bool dirty, Args&&... args)
{
    auto branch = std::dynamic_pointer_cast<Branch>(tree);
    auto condition = TRY_AND_CAST(Expression, process_tree(branch->condition(), ctx));
    auto statement = TRY_AND_CAST(Statement, process_tree(branch->statement(), ctx));
    if (dirty || (condition != branch->condition()) || (statement != branch->statement()))
        return std::make_shared<BranchClass>(condition, statement, std::forward<Args>(args)...);
    else
        return tree;
}

ErrorOrNode fold_constants(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode bind_types(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode lower(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode execute(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode execute(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& root);

}
