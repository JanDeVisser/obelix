/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>

#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/Intrinsics.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>
#include <string>
#include <unordered_map>

namespace Obelix {

extern_logging_category(parser);

template <>
std::string dump_value<std::shared_ptr<SyntaxNode>>(std::shared_ptr<SyntaxNode> const& value)
{
    return format("{}", value->node_type());
}

class BindContext : public Context<std::shared_ptr<SyntaxNode>> {
public:
    std::shared_ptr<ObjectType> return_type { nullptr };
    int stage { 0 };

    void add_unresolved_function(std::shared_ptr<FunctionCall> func_call)
    {
        if (parent() != nullptr) {
            (static_cast<BindContext*>(parent()))->add_unresolved_function(func_call);
            return;
        }
        m_unresolved_functions.push_back(func_call);
    }

    [[nodiscard]] std::vector<std::shared_ptr<FunctionCall>> const& unresolved_functions() const
    {
        if (parent() != nullptr) {
            return (static_cast<BindContext*>(parent()))->unresolved_functions();
        }
        return m_unresolved_functions;
    }

    void clear_unresolved_functions()
    {
        if (parent() != nullptr) {
            (static_cast<BindContext*>(parent()))->clear_unresolved_functions();
            return;
        }
        m_unresolved_functions.clear();
    }

    void add_declared_function(std::string const& name, std::shared_ptr<BoundFunctionDecl> func)
    {
        if (parent() != nullptr) {
            (static_cast<BindContext*>(parent()))->add_declared_function(name, func);
            return;
        }
        m_declared_functions[name] = func;
    }

    [[nodiscard]] std::unordered_map<std::string, std::shared_ptr<BoundFunctionDecl>> const& declared_functions() const
    {
        if (parent() != nullptr) {
            return (static_cast<BindContext*>(parent()))->declared_functions();
        }
        return m_declared_functions;
    }

    [[nodiscard]] std::optional<std::shared_ptr<BoundFunctionDecl>> declared_function(std::string const& name) const
    {
        if (parent() != nullptr) {
            return (static_cast<BindContext*>(parent()))->declared_function(name);
        }
        if (m_declared_functions.contains(name))
            return m_declared_functions.at(name);
        return {};
    }

    void clear_declared_functions()
    {
        if (parent() != nullptr) {
            (static_cast<BindContext*>(parent()))->clear_declared_functions();
            return;
        }
        m_declared_functions.clear();
    }

private:
    std::vector<std::shared_ptr<FunctionCall>> m_unresolved_functions;
    std::unordered_map<std::string, std::shared_ptr<BoundFunctionDecl>> m_declared_functions;
};

INIT_NODE_PROCESSOR(BindContext);

NODE_PROCESSOR(StructDefinition)
{
    auto struct_def = std::dynamic_pointer_cast<StructDefinition>(tree);
    if (struct_def->fields().empty())
        return SyntaxError { ErrorCode::SyntaxError, struct_def->token(), format("Struct {} has no fields", struct_def->name()) };
    BoundIdentifiers bound_fields;
    FieldDefs field_defs {};
    for (auto const& field : struct_def->fields()) {
        auto field_type_maybe = field->type()->resolve_type();
        if (field_type_maybe.is_error()) {
            auto err = field_type_maybe.error();
            return SyntaxError { err.code(), struct_def->token(), err.message() };
        }
        auto field_type = field_type_maybe.value();
        bound_fields.push_back(make_node<BoundIdentifier>(field, field_type));
        field_defs.emplace_back(field->name(), field_type);
    }
    auto type_maybe = ObjectType::make_struct_type(struct_def->name(), field_defs);
    if (type_maybe.is_error()) {
        auto err = type_maybe.error();
        return SyntaxError { err.code(), struct_def->token(), err.message() };
    }
    auto type = type_maybe.value();
    return make_node<BoundStructDefinition>(struct_def, type);
}

NODE_PROCESSOR(VariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
    auto t = var_decl->type();
    std::shared_ptr<ObjectType> var_type { nullptr };
    if (t) {
        auto var_type_maybe = t->resolve_type();
        if (var_type_maybe.is_error()) {
            auto err = var_type_maybe.error();
            return SyntaxError { err.code(), var_decl->token(), err.message() };
        }
        var_type = var_type_maybe.value();
    }
    std::shared_ptr<BoundExpression> expr { nullptr };
    if (var_decl->expression()) {
        auto processed_expr = TRY(process(var_decl->expression(), ctx));
        assert(processed_expr != nullptr);
        auto processed_expr_as_bound_expr = std::dynamic_pointer_cast<BoundExpression>(processed_expr);
        if (processed_expr_as_bound_expr == nullptr) {
            return tree;
        }
        expr = std::dynamic_pointer_cast<BoundExpression>(processed_expr);

        if (var_type && var_type->type() != PrimitiveType::Any && (*(expr->type()) != *var_type)) {
            ObjectType::dump();
            return SyntaxError { ErrorCode::TypeMismatch, var_decl->token(), var_decl->name(), var_decl->type(), expr->type() };
        }
        if (!var_type)
            var_type = expr->type();
    } else if (!var_type) {
        return SyntaxError { ErrorCode::UntypedVariable, var_decl->token(), var_decl->name() };
    }

    auto identifier = make_node<BoundIdentifier>(var_decl->identifier(), var_type);
    std::shared_ptr<BoundVariableDeclaration> ret;
    if (tree->node_type() == SyntaxNodeType::StaticVariableDeclaration)
        ret = make_node<BoundStaticVariableDeclaration>(var_decl, identifier, expr);
    else
        ret = make_node<BoundVariableDeclaration>(var_decl, identifier, expr);
    ctx.declare(var_decl->name(), ret);
    return ret;
}

ALIAS_NODE_PROCESSOR(StaticVariableDeclaration, VariableDeclaration);

NODE_PROCESSOR(FunctionDecl)
{
    auto decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
    if (decl->type() == nullptr)
        return SyntaxError { ErrorCode::UntypedFunction, decl->token(), decl->name() };

    // auto ret_type_or_error = decl->type()->resolve_type();
    // if (ret_type_or_error.is_error()) {
    //     return SyntaxError { ret_type_or_error.error(), decl->token() };
    // }
    auto ret_type = TRY_ADAPT(decl->type()->resolve_type(), decl->token());

    auto identifier = make_node<BoundIdentifier>(decl->identifier(), ret_type);
    BoundIdentifiers bound_parameters;
    for (auto& parameter : decl->parameters()) {
        if (parameter->type() == nullptr)
            return SyntaxError { ErrorCode::UntypedParameter, parameter->token(), parameter->name() };
        auto parameter_type = TRY_ADAPT(parameter->type()->resolve_type(), identifier->token());
        bound_parameters.push_back(make_node<BoundIdentifier>(parameter, parameter_type));
    }
    std::shared_ptr<BoundFunctionDecl> bound_decl;
    switch (decl->node_type()) {
    case SyntaxNodeType::IntrinsicDecl:
        bound_decl = make_node<BoundIntrinsicDecl>(std::dynamic_pointer_cast<IntrinsicDecl>(decl), identifier, bound_parameters);
        break;
    case SyntaxNodeType::NativeFunctionDecl:
        bound_decl = make_node<BoundNativeFunctionDecl>(std::dynamic_pointer_cast<NativeFunctionDecl>(decl), identifier, bound_parameters);
        break;
    default:
        bound_decl = make_node<BoundFunctionDecl>(decl, identifier, bound_parameters);
        break;
    }
    ctx.declare(bound_decl->name(), bound_decl);
    ctx.add_declared_function(bound_decl->name(), bound_decl);
    return bound_decl;
}

ALIAS_NODE_PROCESSOR(IntrinsicDecl, FunctionDecl);
ALIAS_NODE_PROCESSOR(NativeFunctionDecl, FunctionDecl);

NODE_PROCESSOR(FunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
    auto decl = TRY_AND_CAST(BoundFunctionDecl, process(func_def->declaration(), ctx));

    std::shared_ptr<Statement> func_block { nullptr };
    if (func_def->statement()) {
        BindContext func_ctx(ctx);
        for (auto& param : decl->parameters()) {
            auto dummy_decl = make_node<VariableDeclaration>(param->token(), make_node<Identifier>(param->token(), param->name()));
            func_ctx.declare(param->name(), make_node<BoundVariableDeclaration>(dummy_decl, param, nullptr));
        }
        func_ctx.return_type = decl->type();
        func_block = TRY_AND_CAST(Statement, process(func_def->statement(), func_ctx));
    }
    return make_node<BoundFunctionDef>(func_def, decl, func_block);
}

NODE_PROCESSOR(BinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
    auto lhs = TRY_AND_CAST(BoundExpression, process(expr->lhs(), ctx));
    if (lhs == nullptr) {
        auto ident = std::dynamic_pointer_cast<Identifier>(expr->lhs());
        if (ident != nullptr) {
            return SyntaxError { ErrorCode::UndeclaredVariable, ident->token(), ident->name() };
        }
        return tree;
    }
    auto rhs = TRY(process(expr->rhs(), ctx));
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
        { TokenCode::OpenBracket, BinaryOperator::Subscript },
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
        return SyntaxError { ErrorCode::OperatorUnresolved, expr->token(), expr->op().value(), lhs->to_string() };

    if (op == BinaryOperator::MemberAccess) {
        if (lhs->type()->type() != PrimitiveType::Struct)
            return SyntaxError { ErrorCode::CannotAccessMember, lhs->token(), lhs->to_string() };
        if (rhs->node_type() != SyntaxNodeType::Identifier)
            return SyntaxError { ErrorCode::NotMember, rhs->token(), rhs, lhs };
        auto ident = std::dynamic_pointer_cast<Identifier>(rhs);
        auto field = lhs->type()->field(ident->name());
        if (field.type->type() == PrimitiveType::Unknown)
            return SyntaxError { ErrorCode::NotMember, rhs->token(), rhs, lhs };
        auto member_identifier = make_node<BoundIdentifier>(rhs->token(), ident->name(), field.type);
        return std::make_shared<BoundMemberAccess>(lhs, member_identifier);
    }

    if (lhs == nullptr)
        return SyntaxError { ErrorCode::UntypedExpression, expr->lhs()->token(), expr->lhs()->to_string() };
    if (rhs_bound == nullptr) {
        auto ident = std::dynamic_pointer_cast<Identifier>(rhs);
        if (ident != nullptr) {
            return SyntaxError { ErrorCode::UndeclaredVariable, ident->token(), ident->name() };
        }
        return tree;
    }

    if (op == BinaryOperator::Subscript) {
        if (lhs->type()->type() != PrimitiveType::Array)
            return SyntaxError { ErrorCode::CannotAccessMember, lhs->token(), lhs->to_string() };
        if (rhs_bound->type()->type() != PrimitiveType::SignedIntegerNumber)
            return SyntaxError { ErrorCode::TypeMismatch, rhs->token(), rhs, ObjectType::get(PrimitiveType::Int), rhs_bound->type() };
        if (rhs_bound->node_type() == SyntaxNodeType::BoundIntLiteral) {
            auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(rhs_bound);
            auto value = literal->value<int>();
            if ((value < 0) || (lhs->type()->template_arguments()[1].as_integer() <= value))
                return SyntaxError { ErrorCode::IndexOutOfBounds, rhs->token(), value, lhs->type()->template_arguments()[1].as_integer() };
        }
        return std::make_shared<BoundArrayAccess>(lhs, rhs_bound, lhs->type()->template_arguments()[0].as_type());
    }

    if (op == BinaryOperator::Assign || BinaryOperator_is_assignment(op)) {
        auto assignee = std::dynamic_pointer_cast<BoundVariableAccess>(lhs);
        if (assignee == nullptr)
            return SyntaxError { ErrorCode::CannotAssignToRValue, lhs->token(), lhs->to_string() };
        auto lhs_as_ident = std::dynamic_pointer_cast<BoundIdentifier>(assignee);
        if (lhs_as_ident) {
            auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(lhs);
            auto var_decl_maybe = ctx.get(identifier->name());
            if (!var_decl_maybe.has_value())
                return SyntaxError { ErrorCode::UndeclaredVariable, lhs->token(), identifier->name() };
            auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(var_decl_maybe.value());
            if (var_decl == nullptr)
                return SyntaxError { ErrorCode::CannotAssignToFunction, lhs->token(), identifier->name() };
            if (var_decl->is_const())
                return SyntaxError { ErrorCode::CannotAssignToConstant, lhs->token(), var_decl->name() };
        }
        if (*(lhs->type()) != *(rhs_bound->type()))
            return SyntaxError { ErrorCode::TypeMismatch, expr->token(), rhs->to_string(), lhs->type(), rhs_bound->type() };

        if (op == BinaryOperator::Assign)
            return make_node<BoundAssignment>(expr->token(), assignee, rhs_bound);

        // +=, -= and friends: rewrite to a straight-up assignment to a binary
        auto new_rhs = make_node<BoundBinaryExpression>(expr->token(),
            lhs, BinaryOperator_for_assignment_operator(op), rhs_bound, rhs_bound->type());
        return make_node<BoundAssignment>(expr->token(), assignee, new_rhs);
    }

    auto return_type = lhs->type()->return_type_of(to_operator(op), rhs_bound->type());
    if (!return_type.has_value())
        return SyntaxError { ErrorCode::ReturnTypeUnresolved, expr->token(), format("{} {} {}", lhs, op, rhs) };
    return make_node<BoundBinaryExpression>(expr, lhs, op, rhs_bound, return_type.value());
}

NODE_PROCESSOR(UnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, process(expr->operand(), ctx));
    if (operand == nullptr)
        return tree;

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
        return SyntaxError { ErrorCode::OperatorUnresolved, expr->token(), expr->op().value(), operand->to_string() };

    auto return_type = operand->type()->return_type_of(to_operator(op));
    if (!return_type.has_value())
        return SyntaxError { ErrorCode::ReturnTypeUnresolved, expr->token(), format("{} {}", op, operand) };
    return make_node<BoundUnaryExpression>(expr, operand, op, return_type.value());
}

NODE_PROCESSOR(Variable)
{
    auto variable = std::dynamic_pointer_cast<Variable>(tree);
    auto type_decl_maybe = ctx.get(variable->name());
    if (!type_decl_maybe.has_value())
        return tree;
    auto type_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(type_decl_maybe.value());
    if (type_decl == nullptr)
        return SyntaxError { ErrorCode::SyntaxError, variable->token(), format("Function {} cannot be referenced as a variable", variable->name()) };
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(type_decl);
    return make_node<BoundVariable>(variable, var_decl->type());
}

NODE_PROCESSOR(IntLiteral)
{
    return make_node<BoundIntLiteral>(std::dynamic_pointer_cast<IntLiteral>(tree));
}

NODE_PROCESSOR(StringLiteral)
{
    return make_node<BoundStringLiteral>(std::dynamic_pointer_cast<StringLiteral>(tree));
}

NODE_PROCESSOR(BooleanLiteral)
{
    return make_node<BoundBooleanLiteral>(std::dynamic_pointer_cast<BooleanLiteral>(tree));
}

NODE_PROCESSOR(FunctionCall)
{
    auto func_call = std::dynamic_pointer_cast<FunctionCall>(tree);
    std::shared_ptr<BoundFunctionDecl> func_decl;

    BoundExpressions args;
    ObjectTypes arg_types;
    for (auto& arg : func_call->arguments()) {
        auto processed_arg = TRY_AND_CAST(BoundExpression, process(arg, ctx));
        if (processed_arg == nullptr) {
            return tree;
        }
        args.push_back(processed_arg);
        arg_types.push_back(processed_arg->type());
    }

    auto type_decl_maybe = ctx.declared_function(func_call->name());
    if (!type_decl_maybe.has_value()) {
        ctx.add_unresolved_function(func_call);
        return tree;
    }
    func_decl = std::dynamic_pointer_cast<BoundFunctionDecl>(type_decl_maybe.value());
    if (func_decl == nullptr)
        return SyntaxError { ErrorCode::SyntaxError, func_call->token(), format("Variable {} cannot be called", func_call->name()) };
    if (args.size() != func_decl->parameters().size())
        return SyntaxError { ErrorCode::ArgumentCountMismatch, func_call->token(), func_call->name(), func_call->arguments().size() };

    for (auto ix = 0u; ix < args.size(); ix++) {
        auto& arg = args.at(ix);
        auto& param = func_decl->parameters().at(ix);
        if (!arg->type()->is_assignable_to(param->type()))
            return SyntaxError { ErrorCode::ArgumentTypeMismatch, param->token(), func_call->name(), arg->type(), param->name() };
    }

    switch (func_decl->node_type()) {
    case SyntaxNodeType::BoundIntrinsicDecl: {
        auto intrinsic = IntrinsicType_by_name(func_decl->name());
        if (intrinsic == IntrinsicType::NotIntrinsic)
            return SyntaxError { ErrorCode::SyntaxError, func_call->token(), format("Intrinsic {} not defined", func_decl->name()) };
        return make_node<BoundIntrinsicCall>(func_call, args, std::dynamic_pointer_cast<BoundIntrinsicDecl>(func_decl), intrinsic);
    }
    case SyntaxNodeType::BoundNativeFunctionDecl:
        return make_node<BoundNativeFunctionCall>(func_call, args, std::dynamic_pointer_cast<BoundNativeFunctionDecl>(func_decl));
    default:
        return make_node<BoundFunctionCall>(func_call, args, func_decl);
    }
}

NODE_PROCESSOR(ExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
    auto bound_expr = TRY_AND_CAST(BoundExpression, process(expr_stmt->expression(), ctx));
    if (bound_expr == nullptr)
        return tree;
    return make_node<BoundExpressionStatement>(expr_stmt, bound_expr);
}

NODE_PROCESSOR(Return)
{
    auto ret_stmt = std::dynamic_pointer_cast<Return>(tree);
    auto bound_expr = TRY_AND_CAST(BoundExpression, process(ret_stmt->expression(), ctx));
    if (ctx.return_type) {
        if (!bound_expr) {
            return tree;
        }
        if (ctx.return_type->type() != bound_expr->type()->type())
            return SyntaxError { ErrorCode::TypeMismatch, ret_stmt->token(), bound_expr->to_string(), ctx.return_type->name(), bound_expr->type_name() };
    } else {
        if (bound_expr)
            return SyntaxError { ErrorCode::SyntaxError, ret_stmt->token(), format("Expected void return, got a return value of type '{}'", bound_expr->type_name()) };
    }
    return make_node<BoundReturn>(ret_stmt, bound_expr);
}

NODE_PROCESSOR(Branch)
{
    auto branch = std::dynamic_pointer_cast<Branch>(tree);
    auto bound_condition = TRY_AND_CAST(BoundExpression, process(branch->condition(), ctx));
    if (bound_condition == nullptr)
        return tree;
    auto bound_statement = TRY_AND_CAST(Statement, process(branch->statement(), ctx));
    return make_node<BoundBranch>(branch, bound_condition, bound_statement);
}

NODE_PROCESSOR(IfStatement)
{
    auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);

    BoundBranches bound_branches;
    for (auto& branch : if_stmt->branches()) {
        auto bound_branch = TRY_AND_CAST(BoundBranch, process(branch, ctx));
        bound_branches.push_back(bound_branch);
    }
    auto bound_else_stmt = TRY_AND_CAST(Statement, process(if_stmt->else_stmt(), ctx));
    return make_node<BoundIfStatement>(if_stmt, bound_branches, bound_else_stmt);
}

NODE_PROCESSOR(WhileStatement)
{
    auto stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
    auto bound_condition = TRY_AND_CAST(BoundExpression, process(stmt->condition(), ctx));
    if (bound_condition == nullptr)
        return tree;
    auto bound_statement = TRY_AND_CAST(Statement, process(stmt->statement(), ctx));
    return make_node<BoundWhileStatement>(stmt, bound_condition, bound_statement);
}

NODE_PROCESSOR(ForStatement)
{
    auto stmt = std::dynamic_pointer_cast<ForStatement>(tree);
    auto bound_range = TRY_AND_CAST(BoundExpression, process(stmt->range(), ctx));
    if (bound_range == nullptr)
        return tree;
    auto range_binary_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(bound_range);
    if (range_binary_expr == nullptr)
        return SyntaxError { ErrorCode::SyntaxError, stmt->token(), "Invalid for-loop range" };
    if (range_binary_expr->op() != BinaryOperator::Range)
        return SyntaxError { ErrorCode::SyntaxError, stmt->token(), "Invalid for-loop range" };
    auto variable_type = range_binary_expr->lhs()->type();

    bool must_declare_variable = true;
    auto type_decl_maybe = ctx.get(stmt->variable());
    if (type_decl_maybe.has_value()) {
        auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(type_decl_maybe.value());
        if (var_decl != nullptr) {
            if (*(var_decl->type()) != *variable_type) {
                return SyntaxError { ErrorCode::TypeMismatch, var_decl->token(), var_decl->name(), var_decl->type(), variable_type };
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
    auto bound_statement = TRY_AND_CAST(Statement, process(stmt->statement(), for_ctx));
    return make_node<BoundForStatement>(stmt, bound_range, bound_statement, must_declare_variable);
}

NODE_PROCESSOR(CaseStatement)
{
    auto branch = std::dynamic_pointer_cast<CaseStatement>(tree);
    auto bound_condition = TRY_AND_CAST(BoundExpression, process(branch->condition(), ctx));
    if (bound_condition == nullptr)
        return tree;
    auto bound_statement = TRY_AND_CAST(Statement, process(branch->statement(), ctx));
    return make_node<BoundCaseStatement>(branch, bound_condition, bound_statement);
}

NODE_PROCESSOR(DefaultCase)
{
    auto branch = std::dynamic_pointer_cast<DefaultCase>(tree);
    auto bound_statement = TRY_AND_CAST(Statement, process(branch->statement(), ctx));
    return make_node<BoundDefaultCase>(branch, bound_statement);
}

NODE_PROCESSOR(SwitchStatement)
{
    auto stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
    auto bound_expression = TRY_AND_CAST(BoundExpression, process(stmt->expression(), ctx));
    if (bound_expression == nullptr)
        return tree;
    BoundCaseStatements bound_case_statements;
    for (auto& case_stmt : stmt->cases()) {
        auto bound_case = TRY_AND_CAST(BoundCaseStatement, process(case_stmt, ctx));
        bound_case_statements.push_back(bound_case);
    }
    auto bound_default_case = TRY_AND_CAST(BoundDefaultCase, process(stmt->default_case(), ctx));
    return make_node<BoundSwitchStatement>(stmt, bound_expression, bound_case_statements, bound_default_case);
}

ErrorOrNode bind_types(std::shared_ptr<SyntaxNode> const& tree)
{
    BindContext root;
    for (root.stage = 0; root.stage < 2; ++root.stage) {
        root.clear_unresolved_functions();
        auto ret = process(tree, root);

        if (ret.is_error() || root.unresolved_functions().empty())
            return ret;
    }
    auto func_call = root.unresolved_functions().back();
    // FIXME Deal with multiple unresolved functions
    return SyntaxError { ErrorCode::UntypedFunction, func_call->token(), func_call->name() };
}

}
