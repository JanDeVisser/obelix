/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>

#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/Intrinsics.h>
#include <obelix/Processor.h>
#include <obelix/bind/BindContext.h>
#include <obelix/parser/Parser.h>

namespace Obelix {

logging_category(bind);

ErrorOr<std::shared_ptr<BoundExpression>, SyntaxError> make_function_call(BindContext& ctx, pSyntaxNode function, pBoundExpressionList arguments)
{
    BoundExpressions args;
    ObjectTypes arg_types;
    for (auto& arg : arguments->expressions()) {
        args.push_back(arg);
        auto smallest_type = arg->type()->smallest_compatible_type();
        arg_types.push_back(smallest_type);
    }

    pBoundFunctionDecl declaration;
    if (auto variable = std::dynamic_pointer_cast<Variable>(function); variable != nullptr) {
        declaration = ctx.match(variable->name(), arg_types, true);
        if (declaration == nullptr)
            return nullptr;
    }

    if (auto member_access = std::dynamic_pointer_cast<BoundMemberAccess>(function); member_access != nullptr)
        return SyntaxError { function->location(), "Cannot call '{}'", function };

    if (auto bound_identifier = std::dynamic_pointer_cast<BoundIdentifier>(function); bound_identifier != nullptr)
        return SyntaxError { function->location(), "Cannot call '{}'", function };

    if (auto bound_literal = std::dynamic_pointer_cast<BoundLiteral>(function); bound_literal != nullptr)
        return SyntaxError { function->location(), "Cannot call '{}'", function };

    if (auto member_access = std::dynamic_pointer_cast<UnboundMemberAccess>(function); member_access != nullptr) {
        switch (member_access->structure()->type()->type()) {
        case PrimitiveType::Module: {
            auto module = std::dynamic_pointer_cast<BoundModule>(member_access->structure());
            declaration = ctx.match(member_access->structure()->qualified_name(), member_access->member()->name(), arg_types);
            if (declaration == nullptr)
                return nullptr;
        }
        case PrimitiveType::Struct: {
            if (auto struct_def = ctx.struct_definition(member_access->structure()->type_name()); struct_def != nullptr) {
                if (auto method_description = member_access->structure()->type()->get_method(member_access->member()->name(), arg_types); method_description != nullptr) {
                    for (auto const& method : struct_def->methods()) {
                        auto bound_method = std::dynamic_pointer_cast<BoundFunctionDef>(method);
                        auto bound_method_decl = std::dynamic_pointer_cast<BoundMethodDecl>(bound_method->declaration());
                        if (bound_method_decl->method() == method_description)
                            return std::make_shared<BoundMethodCall>(function->location(), bound_method_decl, member_access->structure(), args);
                    }
                }
            }
        }

        /* Fall through */
        default: {
            BoundExpressions arg_list_with_this;
            arg_list_with_this.push_back(member_access->structure());
            for (auto const& arg : arguments->expressions()) {
                arg_list_with_this.push_back(arg);
            }
            return make_function_call(ctx, member_access->member(), std::make_shared<BoundExpressionList>(arguments->location(), arg_list_with_this));
        }
        }
    }
    if (declaration == nullptr)
        return nullptr;

    switch (declaration->node_type()) {
    case SyntaxNodeType::BoundIntrinsicDecl: {
        auto intrinsic = IntrinsicType_by_name(declaration->name());
        if (intrinsic == IntrinsicType::NotIntrinsic)
            return SyntaxError { function->location(), "Intrinsic {} not defined", declaration->name() };
        return std::make_shared<BoundIntrinsicCall>(function->location(), std::dynamic_pointer_cast<BoundIntrinsicDecl>(declaration), args, intrinsic);
    }
    case SyntaxNodeType::BoundNativeFunctionDecl:
        return std::make_shared<BoundNativeFunctionCall>(function->location(), std::dynamic_pointer_cast<BoundNativeFunctionDecl>(declaration), args);
    default:
        return std::make_shared<BoundFunctionCall>(function->location(), declaration, args);
    }
}

ErrorOr<pBoundExpression, SyntaxError> make_expression_for_assignment(pBoundExpression expr, pObjectType desired_type = nullptr)
{
    auto cast_literal = [&expr](pObjectType const& type) -> pBoundExpression {
        if (type != nullptr && !expr->type()->is_assignable_to(type)) {
            auto int_literal = std::dynamic_pointer_cast<BoundIntLiteral>(expr);
            if (int_literal == nullptr)
                return nullptr;
            auto casted_literal_maybe = int_literal->cast(type);
            if (casted_literal_maybe.is_error())
                int_literal = nullptr;
            else
                int_literal = casted_literal_maybe.value();
            return int_literal;
        }
        return expr;
    };

    pBoundExpression new_expr = expr;

    // var c: type_a/type_b
    // ...
    // const x: type_a = c
    // const y: type_b = c
    if (desired_type != nullptr && expr->type()->type() == PrimitiveType::Conditional && expr->type()->is_assignable_to(desired_type) && expr->type() != desired_type) {
        std::string member = "value";
        auto member_type = expr->type()->template_argument<std::shared_ptr<ObjectType>>("success_type");
        auto error_type = expr->type()->template_argument<std::shared_ptr<ObjectType>>("error_type");
        if (!member_type->is_assignable_to(desired_type)) {
            member = "error";
            member_type = error_type;
            if (!member_type->is_assignable_to(desired_type))
                return SyntaxError { expr->location(), "Cannot assign '{}' which has type '{}' to type '{}'", expr->to_string(), expr->type(), desired_type };
        }
        auto member_identifier = std::make_shared<BoundIdentifier>(expr->location(), member, member_type);
        return std::make_shared<BoundMemberAccess>(expr, member_identifier);
    }

    // var x: type_a/type_b
    // const foo: type_a = ...
    // const bar: type_b = ...
    // x = foo
    // x = bar
    if (desired_type != nullptr && desired_type->type() == PrimitiveType::Conditional) {
        auto success = true;
        new_expr = cast_literal(desired_type->template_argument<pObjectType>("success_type"));
        if (new_expr == nullptr) {
            success = false;
            new_expr = cast_literal(desired_type->template_argument<pObjectType>("error_type"));
        }
        if (new_expr == nullptr)
            return SyntaxError { expr->location(), "Cannot assign '{}' which has type '{}' to type '{}'", expr->to_string(), expr->type(), desired_type };
        return std::make_shared<BoundConditionalValue>(new_expr->location(), new_expr, success, desired_type);
    }

    if (desired_type != nullptr) {
        new_expr = cast_literal(desired_type);
        if (new_expr == nullptr)
            return SyntaxError { expr->location(), "Cannot assign '{}' which has type '{}' to type '{}'", expr->to_string(), expr->type(), desired_type };
        return new_expr;
    }
    return expr;
}

template<class Ctx>
ErrorOrTypedNode<Statement> process_branch(std::shared_ptr<Branch> const& branch, Ctx& ctx, ProcessResult& result)
{
    pBoundExpression bound_condition = nullptr;
    if (branch->condition() != nullptr)
        bound_condition = TRY_AND_TRY_CAST_RETURN(BoundExpression, branch->condition(), ctx, branch);
    auto statement_processed_maybe = try_and_cast<Statement>(branch->statement(), ctx, result);
    if (statement_processed_maybe.is_error())
        return statement_processed_maybe.error();
    auto statement_processed = statement_processed_maybe.value();
    if (statement_processed == nullptr || !statement_processed->is_fully_bound())
        return branch;
    return std::make_shared<BoundBranch>(branch->location(), bound_condition, statement_processed);
}

#define PROCESS_BRANCH(tree, branch, ctx)                                                    \
    ({                                                                                       \
        auto __processed_branch = process_branch(branch, ctx, result);                       \
        if (__processed_branch.is_error())                                                   \
            return __processed_branch.error();                                               \
        auto __bound_branch = __processed_branch.value();                                    \
        if (__bound_branch == nullptr)                                                       \
            return tree;                                                                     \
        auto __bound_branch_casted = std::dynamic_pointer_cast<BoundBranch>(__bound_branch); \
        if (__bound_branch_casted == nullptr)                                                \
            return tree;                                                                     \
        __bound_branch_casted;                                                               \
    })

#define PROCESS_BRANCHES(tree, branches, ctx)                                                    \
    ({                                                                                           \
        BoundBranches __bound_branches;                                                          \
        for (auto const& __branch : branches) {                                                  \
            auto __processed_branch = process_branch(__branch, ctx, result);                     \
            if (__processed_branch.is_error())                                                   \
                return __processed_branch.error();                                               \
            auto __bound_branch = __processed_branch.value();                                    \
            if (__bound_branch == nullptr)                                                       \
                return tree;                                                                     \
            auto __bound_branch_casted = std::dynamic_pointer_cast<BoundBranch>(__bound_branch); \
            if (__bound_branch_casted == nullptr)                                                \
                return tree;                                                                     \
            __bound_branches.push_back(__bound_branch_casted);                                   \
        }                                                                                        \
        __bound_branches;                                                                        \
    })

INIT_NODE_PROCESSOR(BindContext)

NODE_PROCESSOR(StructDefinition)
{
    auto struct_def = std::dynamic_pointer_cast<StructDefinition>(tree);
    if (struct_def->fields().empty())
        return SyntaxError { struct_def->location(), "Struct '{}' has no fields", struct_def->name() };
    auto& struct_ctx = ctx.make_structcontext(struct_def->name());
    BoundIdentifiers bound_fields;
    FieldDefs field_defs {};
    for (auto const& field : struct_def->fields()) {
        auto field_type_maybe = field->type()->resolve_type();
        if (field_type_maybe.is_error()) {
            return SyntaxError { struct_def->location(), field_type_maybe.error().message() };
        }
        auto field_type = field_type_maybe.value();
        auto bound_field_name = std::make_shared<BoundIdentifier>(field, field_type);
        bound_fields.push_back(bound_field_name);
        field_defs.emplace_back(field->name(), field_type);
    }
    auto type = ObjectType::get(struct_def->name());
    if (type == nullptr) {
        type = ObjectType::register_struct_type(struct_def->name(), field_defs);
        if (type == nullptr)
            return SyntaxError { struct_def->location(), "Could not register struct type '{}'", struct_def->name() };
        struct_ctx.add_custom_type(type);
    }
    Statements bound_methods;
    auto resolved { true };
    for (auto const& method : struct_def->methods()) {
        auto bound_method = TRY_AND_CAST(BoundFunctionDef, method, struct_ctx);
        if (bound_method == nullptr) {
            resolved = false;
            continue;
        }
        MethodParameters params;
        ObjectTypes param_types;
        for (auto const& p : bound_method->parameters()) {
            params.emplace_back(p->name(), p->type());
            param_types.push_back(p->type());
        }
        pMethodDescription method_descr = type->get_method(bound_method->name(), param_types);
        if (method_descr == nullptr)
            method_descr = type->add_method(MethodDescription(bound_method->name(), bound_method->type(), params));
        // FIXME if a matching method is found, check the return type
        bound_methods.push_back(std::make_shared<BoundFunctionDef>(method->location(),
            std::make_shared<BoundMethodDecl>(method->declaration(), method_descr),
            bound_method->statement()));
    }
    if (!resolved)
        return tree;
    auto ret = std::make_shared<BoundStructDefinition>(struct_def, type, bound_fields, bound_methods);
    struct_ctx.set_struct_definition(ret);
    return ret;
}

NODE_PROCESSOR(EnumDef)
{
    auto enum_def = std::dynamic_pointer_cast<EnumDef>(tree);
    NVPs enum_values;
    BoundEnumValueDefs bound_values;
    long v = 0;
    for (auto const& value : enum_def->values()) {
        if (value->value().has_value())
            // FIXME Check sanity of value
            v = value->value().value();
        enum_values.emplace_back(value->label(), v);
        bound_values.push_back(std::make_shared<BoundEnumValueDef>(value->location(), value->label(), v));
        v++;
    }
    std::shared_ptr<ObjectType> type;
    if (!enum_def->extend()) {
        type = ObjectType::make_enum_type(enum_def->name(), enum_values);
        ctx.add_custom_type(type);
    } else {
        type = ObjectType::get(enum_def->name());
        if (type->type() != PrimitiveType::Enum)
            return SyntaxError { enum_def->location(), "Cannot extend non-existing enum '{}'", enum_def->name() };
        TRY_RETURN(type->extend_enum_type(enum_values));
    }
    return std::make_shared<BoundEnumDef>(enum_def, type, bound_values);
}

NODE_PROCESSOR(ExpressionType)
{
    auto type = std::dynamic_pointer_cast<ExpressionType>(tree);
    auto type_maybe = type->resolve_type();
    if (type_maybe.is_error())
        return SyntaxError { tree->location(), type_maybe.error().message() };
    return std::make_shared<BoundType>(type->location(), type_maybe.value());
}

NODE_PROCESSOR(TypeDef)
{
    auto type_def = std::dynamic_pointer_cast<TypeDef>(tree);

    // FIXME: Make sure type alias isn't yet used for type or function or var or ...
    auto bound_type = TRY_AND_CAST(BoundType, type_def->type(), ctx);
    bound_type->type()->has_alias(type_def->name());
    return std::make_shared<BoundTypeDef>(type_def->location(), type_def->name(), bound_type);
}

NODE_PROCESSOR(Compilation)
{
    auto compilation = std::dynamic_pointer_cast<Compilation>(tree);
    assert(ctx.type() == BindContextType::RootContext);
    BoundModules modules;
    for (auto& imported : compilation->modules()) {
        modules.push_back(TRY_AND_CAST(BoundModule, imported, ctx));
    }
    return std::make_shared<BoundCompilation>(modules, ctx.custom_types(), compilation->main_module());
}

NODE_PROCESSOR(BoundCompilation)
{
    auto compilation = std::dynamic_pointer_cast<BoundCompilation>(tree);
    if (compilation->is_fully_bound())
        return tree;
    assert(ctx.type() == BindContextType::RootContext);
    BoundModules modules;
    for (auto& imported : compilation->modules()) {
        modules.push_back(TRY_AND_CAST(BoundModule, imported, ctx));
    }
    return std::make_shared<BoundCompilation>(modules, ctx.custom_types(), compilation->main_module());
}

NODE_PROCESSOR(Module)
{
    auto module = std::dynamic_pointer_cast<Module>(tree);
    assert(ctx.type() == BindContextType::RootContext);
    std::cout << "Pass " << ctx.stage << ": " << module->name() << "\n"; // "...\015";
    auto& module_ctx = ctx.make_modulecontext(module->name());
    Statements statements;
    for (auto& stmt : module->statements()) {
        statements.push_back(TRY_AND_CAST(Statement, stmt, module_ctx));
    }
    auto block = std::make_shared<Block>(tree->location(), statements);
    auto ret = std::make_shared<BoundModule>(module->location(), module->name(), block, module_ctx.exports(), module_ctx.imports());
    ctx.add_module(ret);
    return ret;
}

NODE_PROCESSOR(BoundModule)
{
    auto module = std::dynamic_pointer_cast<BoundModule>(tree);
    if (module->is_fully_bound())
        return tree;
    assert(ctx.type() == BindContextType::RootContext);
    auto& module_ctx = ctx.make_modulecontext(module->name());
    Statements statements;
    for (auto& stmt : module->block()->statements()) {
        statements.push_back(TRY_AND_CAST(Statement, stmt, module_ctx));
    }
    auto block = std::make_shared<Block>(tree->location(), statements);
    return std::make_shared<BoundModule>(module->location(), module->name(), block, module_ctx.exports(), module_ctx.imports());
}

NODE_PROCESSOR(BoundStructDefinition)
{
    auto struct_def = std::dynamic_pointer_cast<BoundStructDefinition>(tree);
    if (struct_def->is_fully_bound())
        return tree;
    assert(ctx.type() == BindContextType::ModuleContext);
    auto& struct_ctx = ctx.make_structcontext(struct_def->name());
    Statements methods;
    for (auto& method : struct_def->methods()) {
        methods.push_back(TRY_AND_CAST(Statement, method, struct_ctx));
    }
    return std::make_shared<BoundStructDefinition>(struct_def->location(), struct_def->type(), struct_def->fields(), methods);
}

NODE_PROCESSOR(VariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
    auto t = var_decl->type();
    std::shared_ptr<ObjectType> var_type { nullptr };
    if (t) {
        var_type = TRY(t->resolve_type());
    }
    std::shared_ptr<BoundExpression> expr { nullptr };
    if (var_decl->expression()) {
        expr = TRY_AND_TRY_CAST_RETURN(BoundExpression, var_decl->expression(), ctx, tree);
        expr = TRY(make_expression_for_assignment(expr, var_type));
        if (!var_type)
            var_type = expr->type();
    } else if (!var_type) {
        return SyntaxError { var_decl->location(), "Type of variable '{}' could not be determined", var_decl->name() };
    }

    if (var_type->is_custom())
        ctx.add_custom_type(var_type);
    auto identifier = std::make_shared<BoundIdentifier>(var_decl->identifier(), var_type);
    std::shared_ptr<BoundVariableDeclaration> ret;
    bool is_exported { false };
    switch (tree->node_type()) {
    case SyntaxNodeType::VariableDeclaration:
        ret = std::make_shared<BoundVariableDeclaration>(var_decl, identifier, expr);
        break;
    case SyntaxNodeType::StaticVariableDeclaration:
        ret = std::make_shared<BoundStaticVariableDeclaration>(var_decl, identifier, expr);
        break;
    case SyntaxNodeType::LocalVariableDeclaration:
        ret = std::make_shared<BoundLocalVariableDeclaration>(var_decl, identifier, expr);
        // Even though this variable cannot be accessed from other modules, we declare
        // it anyway, so we can give an error message when an attempt is made to access it:
        is_exported = true;
        break;
    case SyntaxNodeType::GlobalVariableDeclaration:
        ret = std::make_shared<BoundGlobalVariableDeclaration>(var_decl, identifier, expr);
        is_exported = true;
        break;
    default:
        fatal("Unreachable");
    }
    TRY_RETURN(ctx.declare(var_decl->name(), ret));
    if (is_exported)
        ctx.add_exported_variable(var_decl->name(), ret);
    return ret;
}

ALIAS_NODE_PROCESSOR(StaticVariableDeclaration, VariableDeclaration)
ALIAS_NODE_PROCESSOR(LocalVariableDeclaration, VariableDeclaration)
ALIAS_NODE_PROCESSOR(GlobalVariableDeclaration, VariableDeclaration)

NODE_PROCESSOR(BoundVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
    TRY_RETURN(ctx.declare(var_decl->name(), var_decl));
    return tree;
}

ALIAS_NODE_PROCESSOR(BoundStaticVariableDeclaration, BoundVariableDeclaration)
ALIAS_NODE_PROCESSOR(BoundLocalVariableDeclaration, BoundVariableDeclaration)
ALIAS_NODE_PROCESSOR(BoundGlobalVariableDeclaration, BoundVariableDeclaration)

NODE_PROCESSOR(FunctionDecl)
{
    auto decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
    pObjectType ret_type;
    if (decl->type() == nullptr) {
        ret_type = ObjectType::get(PrimitiveType::Void);
    } else {
        ret_type = TRY(decl->type()->resolve_type());
        if (ret_type->is_custom() && decl->node_type() != SyntaxNodeType::IntrinsicDecl)
            ctx.add_custom_type(ret_type);
    }

    auto identifier = std::make_shared<BoundIdentifier>(decl->identifier(), ret_type);
    BoundIdentifiers bound_parameters;
    for (auto& parameter : decl->parameters()) {
        if (parameter->type() == nullptr)
            return SyntaxError { parameter->location(), "Untyped function parameter '{}'", parameter->name() };
        auto parameter_type = TRY(parameter->type()->resolve_type());
        bound_parameters.push_back(std::make_shared<BoundIdentifier>(parameter, parameter_type));
        if (parameter_type->is_custom() && decl->node_type() != SyntaxNodeType::IntrinsicDecl)
            ctx.add_custom_type(parameter_type);
    }
    std::shared_ptr<BoundFunctionDecl> bound_decl;
    switch (decl->node_type()) {
    case SyntaxNodeType::IntrinsicDecl:
        bound_decl = std::make_shared<BoundIntrinsicDecl>(std::dynamic_pointer_cast<IntrinsicDecl>(decl), decl->module(), identifier, bound_parameters);
        break;
    case SyntaxNodeType::NativeFunctionDecl:
        bound_decl = std::make_shared<BoundNativeFunctionDecl>(std::dynamic_pointer_cast<NativeFunctionDecl>(decl), decl->module(), identifier, bound_parameters);
        break;
    default:
        bound_decl = std::make_shared<BoundFunctionDecl>(decl, decl->module(), identifier, bound_parameters);
        break;
    }
    if (!ctx.in_struct_def())
        ctx.add_declared_function(bound_decl->name(), bound_decl);
    return bound_decl;
}

ALIAS_NODE_PROCESSOR(IntrinsicDecl, FunctionDecl)
ALIAS_NODE_PROCESSOR(NativeFunctionDecl, FunctionDecl)

NODE_PROCESSOR(FunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
    auto decl = TRY_AND_CAST(BoundFunctionDecl, func_def->declaration(), ctx);

    std::shared_ptr<Statement> func_block { nullptr };
    if (func_def->statement()) {
        auto& func_ctx = ctx.make_subcontext();
        for (auto& param : decl->parameters()) {
            auto dummy_decl = std::make_shared<VariableDeclaration>(param->location(), std::make_shared<Identifier>(param->location(), param->name()));
            TRY_RETURN(func_ctx.declare(param->name(), std::make_shared<BoundVariableDeclaration>(dummy_decl, param, nullptr)));
        }
        func_ctx.return_type = decl->type();
        func_block = TRY_AND_CAST(Statement, func_def->statement(), func_ctx);
    }
    return std::make_shared<BoundFunctionDef>(func_def, decl, func_block);
}

NODE_PROCESSOR(BoundFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
    if (!func_def->statement() || func_def->statement()->is_fully_bound())
        return tree;

    std::shared_ptr<Statement> func_block { nullptr };
    auto& func_ctx = ctx.make_subcontext();
    for (auto& param : func_def->declaration()->parameters()) {
        auto dummy_decl = std::make_shared<VariableDeclaration>(param->location(), std::make_shared<Identifier>(param->location(), param->name()));
        TRY_RETURN(func_ctx.declare(param->name(), std::make_shared<BoundVariableDeclaration>(dummy_decl, param, nullptr)));
    }
    func_ctx.return_type = func_def->declaration()->type();
    func_block = TRY_AND_CAST(Statement, func_def->statement(), func_ctx);
    return std::make_shared<BoundFunctionDef>(func_def->location(), func_def->declaration(), func_block);
}

NODE_PROCESSOR(BinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
    auto lhs_processed = TRY(process(expr->lhs(), ctx, result));
    auto lhs = std::dynamic_pointer_cast<BoundExpression>(lhs_processed);

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
        { TokenCode::OpenParen, BinaryOperator::Call },
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
        return SyntaxError { expr->location(), "Unknown operator '{}' for '{}'", expr->op().value(), lhs };

    auto rhs = expr->rhs();
    if (op == BinaryOperator::MemberAccess) {
        if (lhs == nullptr)
            return tree;

        // Right now members can only be variables (identifiers). Once that's not true anymore
        // this check needs to be changed.
        auto member_var = std::dynamic_pointer_cast<Variable>(rhs);
        if (member_var == nullptr)
            return SyntaxError { expr->location(), "Expression '{}' is not a member of '{}'", rhs, lhs };
        switch (lhs->type()->type()) {
        case PrimitiveType::Struct: {
            auto struct_type = lhs->type();
            if (auto field = struct_type->field(member_var->name()); field.type->type() != PrimitiveType::Unknown) {
                auto member_identifier = std::make_shared<BoundIdentifier>(rhs->location(), member_var->name(), field.type);
                return std::make_shared<BoundMemberAccess>(lhs, member_identifier);
            }
            if (struct_type->has_method(member_var->name()))
                return std::make_shared<UnboundMemberAccess>(lhs, member_var);
            return SyntaxError { expr->location(), "Expression '{}' is not a member of struct '{}' of type '{}'", rhs, lhs, lhs->type() };
        }
        case PrimitiveType::Module: {
            auto module = std::dynamic_pointer_cast<BoundModule>(lhs);
            if (member_var == nullptr)
                return SyntaxError { expr->location(), "Expression '{}' is not a member of module '{}'", rhs, lhs };
            if (auto var_decl_maybe = ctx.exported_variable(module->name(), member_var->name()); var_decl_maybe.has_value()) {
                auto var_decl = var_decl_maybe.value();
                if (std::dynamic_pointer_cast<BoundGlobalVariableDeclaration>(var_decl) == nullptr)
                    return SyntaxError { expr->location(), "Variable '{}' is local to module '{}' and cannot be accessed from the current module", var_decl->name(), module->name() };
                auto member_variable = std::make_shared<BoundVariable>(rhs->location(), member_var->name(), var_decl_maybe.value()->type());
                return std::make_shared<BoundMemberAccess>(lhs, member_variable);
            }
            return std::make_shared<UnboundMemberAccess>(lhs, member_var);
        }
        case PrimitiveType::Conditional: {
            if (member_var == nullptr || (member_var->name() != "value" && member_var->name() != "error"))
                return SyntaxError { expr->location(), "Expression '{}' is not a member of conditional '{}'", rhs, lhs };
            auto type = (member_var->name() == "value")
                ? lhs->type()->template_argument<std::shared_ptr<ObjectType>>("success_type")
                : lhs->type()->template_argument<std::shared_ptr<ObjectType>>("error_type");
            auto member_identifier = std::make_shared<BoundIdentifier>(rhs->location(), member_var->name(), type);
            return std::make_shared<BoundMemberAccess>(lhs, member_identifier);
        }
        case PrimitiveType::Type: {
            auto type_literal = std::dynamic_pointer_cast<BoundTypeLiteral>(lhs);
            if (member_var == nullptr)
                return SyntaxError { expr->location(), "Expression '{}' is not a member of enum '{}'", rhs, lhs };
            assert(type_literal->value()->type() == PrimitiveType::Enum); // FIXME Return syntax error
            auto values = type_literal->value()->template_argument_values<NVP>("values");
            for (auto const& v : values) {
                if (v.first == member_var->name())
                    return std::make_shared<BoundEnumValue>(lhs->location(), type_literal->value(), v.first, v.second);
            }
            return SyntaxError { expr->location(), "Expression '{}' is not a member of enum '{}' of type '{}'", rhs, lhs, type_literal->value() };
        }
        default:
            if (member_var == nullptr)
                return SyntaxError { expr->location(), "Expression '{}' is not a member of '{}'", rhs, lhs };
            return std::make_shared<UnboundMemberAccess>(lhs, member_var);
        }
    }

    auto rhs_bound = TRY_AND_TRY_CAST(BoundExpression, expr->rhs(), ctx);
    if (rhs_bound == nullptr) {
        ctx.add_unresolved(expr);
        return tree;
    }
    if (op == BinaryOperator::Call) {
        if (rhs_bound->type()->type() != PrimitiveType::List)
            return SyntaxError { expr->location(), "Cannot call {} with {}", lhs, expr->rhs() };
        auto arg_list = std::dynamic_pointer_cast<BoundExpressionList>(rhs_bound);
        auto ret = TRY(make_function_call(ctx, lhs_processed, arg_list));
        if (ret == nullptr) {
            ctx.add_unresolved(expr);
            return tree;
        }
        return ret;
    }

    if (lhs == nullptr) {
        ctx.add_unresolved(expr);
        return tree;
    }

    if (op == BinaryOperator::Subscript) {
        if (lhs->type()->type() != PrimitiveType::Array)
            return SyntaxError { lhs->location(), ErrorCode::CannotAccessMember, lhs };
        if (rhs_bound->type()->type() != PrimitiveType::SignedIntegerNumber)
            return SyntaxError { rhs->location(), ErrorCode::TypeMismatch, rhs, ObjectType::get(PrimitiveType::Int), rhs_bound->type() };
        if (rhs_bound->node_type() == SyntaxNodeType::BoundIntLiteral) {
            auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(rhs_bound);
            auto value = literal->value<int>();
            if ((value < 0) || (lhs->type()->template_argument<long>("size") <= value))
                return SyntaxError { rhs->location(), ErrorCode::IndexOutOfBounds, value, lhs->type()->template_argument<long>("size") };
        }
        return std::make_shared<BoundArrayAccess>(lhs, rhs_bound, lhs->type()->template_argument<std::shared_ptr<ObjectType>>("base_type"));
    }

    if (op == BinaryOperator::Assign || BinaryOperator_is_assignment(op)) {
        auto assignee = std::dynamic_pointer_cast<BoundVariableAccess>(lhs);
        if (assignee == nullptr)
            return SyntaxError { lhs->location(), ErrorCode::CannotAssignToRValue, lhs->to_string() };

        if (auto lhs_as_ident = std::dynamic_pointer_cast<BoundIdentifier>(assignee); lhs_as_ident) {
            auto var_decl_maybe = ctx.get(lhs_as_ident->name());
            if (!var_decl_maybe.has_value())
                return SyntaxError { lhs->location(), ErrorCode::UndeclaredVariable, lhs_as_ident->name() };
            auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(var_decl_maybe.value());
            if (var_decl == nullptr)
                return SyntaxError { lhs->location(), ErrorCode::CannotAssignToFunction, lhs_as_ident->name() };
            if (var_decl->is_const())
                return SyntaxError { lhs->location(), ErrorCode::CannotAssignToConstant, var_decl->name() };
        }

        if (auto lhs_as_memberaccess = std::dynamic_pointer_cast<BoundMemberAccess>(assignee);
            lhs_as_memberaccess != nullptr && lhs_as_memberaccess->node_type() != SyntaxNodeType::BoundMemberAssignment) {
            assignee = std::make_shared<BoundMemberAssignment>(lhs_as_memberaccess->structure(), lhs_as_memberaccess->member());
        }

        rhs_bound = TRY(make_expression_for_assignment(rhs_bound, assignee->type()));

        if (op == BinaryOperator::Assign)
            return std::make_shared<BoundAssignment>(expr->location(), assignee, rhs_bound);

        // +=, -= and friends: rewrite to a straight-up assignment to a binary
        auto new_rhs = std::make_shared<BoundBinaryExpression>(expr->location(),
            lhs, BinaryOperator_for_assignment_operator(op), rhs_bound, rhs_bound->type());
        return std::make_shared<BoundAssignment>(expr->location(), assignee, new_rhs);
    }

    if ((rhs_bound->node_type() == SyntaxNodeType::BoundIntLiteral) && (rhs_bound->type()->type() == lhs->type()->type()) && (rhs_bound->type()->size() > lhs->type()->size())) {
        rhs_bound = TRY(std::dynamic_pointer_cast<BoundIntLiteral>(rhs_bound)->cast(lhs->type()));
    } else if ((lhs->node_type() == SyntaxNodeType::BoundIntLiteral) && (rhs_bound->type()->type() == lhs->type()->type()) && (lhs->type()->size() > rhs_bound->type()->size())) {
        lhs = TRY(std::dynamic_pointer_cast<BoundIntLiteral>(lhs)->cast(rhs_bound->type()));
    }
    auto return_type = lhs->type()->return_type_of(to_operator(op), rhs_bound->type());
    if (return_type != nullptr) {
        if (return_type->is_custom())
            ctx.add_custom_type(return_type);
        return std::make_shared<BoundBinaryExpression>(expr, lhs, op, rhs_bound, return_type);
    }
    return SyntaxError { expr->location(), ErrorCode::ReturnTypeUnresolved, format("{} {} {}", lhs, op, rhs) };
}

NODE_PROCESSOR(UnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);

    struct UnaryOperatorMap {
        TokenCode code;
        UnaryOperator op;
    };

    constexpr static UnaryOperatorMap operator_map[10] = {
        { TokenCode::Asterisk, UnaryOperator::Dereference },
        { TokenCode::AtSign, UnaryOperator::AddressOf },
        { TokenCode::Plus, UnaryOperator::Identity },
        { TokenCode::Minus, UnaryOperator::Negate },
        { TokenCode::ExclamationPoint, UnaryOperator::LogicalInvert },
        { TokenCode::UnaryIncrement, UnaryOperator::UnaryIncrement },
        { TokenCode::UnaryDecrement, UnaryOperator::UnaryDecrement },
        { TokenCode::Tilde, UnaryOperator::BitwiseInvert },
        { TokenCode::Period, UnaryOperator::UnaryMemberAccess },
    };

    UnaryOperator op { UnaryOperator::InvalidUnary };
    for (auto ix : operator_map) {
        if (ix.code == expr->op().code()) {
            op = ix.op;
            break;
        }
    }
    if (op == UnaryOperator::InvalidUnary)
        return SyntaxError { expr->location(), ErrorCode::OperatorUnresolved, expr->op().value(), expr->operand()->to_string() };

    if (op == UnaryOperator::UnaryMemberAccess) {
        if (ctx.in_struct_def()) {
            if (auto op_identifier = std::dynamic_pointer_cast<Identifier>(expr->operand()); op_identifier != nullptr) {
                auto struct_type = ObjectType::get(ctx.scope_name());
                auto field = struct_type->field(op_identifier->name());
                if (field.type->type() == PrimitiveType::Unknown)
                    return SyntaxError { expr->location(), "Struct of type '{}' has no field '{}'", struct_type->name(), op_identifier->name() };
                return std::make_shared<BoundMemberAccess>(
                    std::make_shared<BoundVariable>(expr->location(), "$this", struct_type),
                    std::make_shared<BoundIdentifier>(op_identifier, field.type));
            } else {
                return SyntaxError { expr->location(), "Can only dereference struct fields using '.' in method definitions" };
            }
        }
        return SyntaxError { expr->location(), "Can only dereference struct fields using '.'" };
    }

    auto operand = TRY_AND_TRY_CAST(BoundExpression, expr->operand(), ctx);
    if (operand == nullptr) {
        ctx.add_unresolved(expr);
        return tree;
    }
    auto return_type = operand->type()->return_type_of(to_operator(op));
    if (return_type == nullptr)
        return SyntaxError { expr->location(), ErrorCode::ReturnTypeUnresolved, format("{} {}", op, operand) };
    if (return_type->is_custom())
        ctx.add_custom_type(return_type);
    return std::make_shared<BoundUnaryExpression>(expr, operand, op, return_type);
}

NODE_PROCESSOR(CastExpression)
{
    auto cast = std::dynamic_pointer_cast<CastExpression>(tree);
    auto expr = TRY_AND_TRY_CAST_RETURN(BoundExpression, cast->expression(), ctx, cast);
    auto type = TRY(cast->type()->resolve_type());
    if (expr->type()->can_cast_to(type) == CanCast::Never)
        return SyntaxError { cast->location(), "Cannot cast {} to {}", expr->type(), type };
    return std::make_shared<BoundCastExpression>(cast->location(), expr, type);
}

NODE_PROCESSOR(ExpressionList)
{
    auto list = std::dynamic_pointer_cast<ExpressionList>(tree);
    BoundExpressions bound_expressions;
    for (auto const& expr : list->expressions()) {
        auto bound_expr = TRY_AND_TRY_CAST_RETURN(BoundExpression, expr, ctx, tree);
        bound_expressions.push_back(bound_expr);
    }
    return std::make_shared<BoundExpressionList>(tree->location(), bound_expressions);
}

NODE_PROCESSOR(Pass)
{
    auto stmt = std::dynamic_pointer_cast<Pass>(tree);
    return std::make_shared<BoundPass>(stmt->location(), stmt->elided_statement());
}

NODE_PROCESSOR(Import)
{
    return std::make_shared<BoundPass>(tree->location(), std::dynamic_pointer_cast<Import>(tree));
}

NODE_PROCESSOR(Variable)
{
    auto variable = std::dynamic_pointer_cast<Variable>(tree);
    auto declaration_maybe = ctx.get(variable->name());
    if (!declaration_maybe.has_value()) {
        if (auto type = ObjectType::get(variable->name()); (type != nullptr) && type->type() != PrimitiveType::Unknown)
            return std::make_shared<BoundTypeLiteral>(variable->location(), type);
        if (auto module = ctx.module(variable->name()); module != nullptr)
            return module;
    } else {
        auto declaration = declaration_maybe.value();
        return std::make_shared<BoundVariable>(variable, declaration->type());
    }
    return tree;
}

NODE_PROCESSOR(UnboundMemberAccess)
{
    auto member_access = std::dynamic_pointer_cast<UnboundMemberAccess>(tree);
    if (auto module = std::dynamic_pointer_cast<BoundModule>(member_access->structure()); module != nullptr) {
        if (auto var_decl_maybe = ctx.exported_variable(module->name(), member_access->member()->name()); var_decl_maybe.has_value()) {
            auto member_variable = std::make_shared<BoundVariable>(member_access->member()->location(), member_access->member()->name(), var_decl_maybe.value()->type());
            return std::make_shared<BoundMemberAccess>(module, member_variable);
        }
    }
    ctx.add_unresolved(member_access);
    return tree;
}

NODE_PROCESSOR(IntLiteral)
{
    auto literal = std::dynamic_pointer_cast<IntLiteral>(tree);
    std::shared_ptr<ObjectType> type = ObjectType::get("s8");
    if (literal->is_typed()) {
        auto type_or_error = literal->type()->resolve_type();
        if (type_or_error.is_error())
            return SyntaxError { literal->location(), "Unknown type '{}'", literal->type() };
        type = type_or_error.value();
    } else {
        uint8_t sz = 8;
        auto value = token_value<int64_t>(literal->token()).value();
        while ((sz < 64) && value >= (1ul << sz))
            sz *= 2;
        type = ObjectType::get(format("s{}", sz));
    }
    return std::make_shared<BoundIntLiteral>(std::dynamic_pointer_cast<IntLiteral>(tree), type);
}

NODE_PROCESSOR(StringLiteral)
{
    return std::make_shared<BoundStringLiteral>(std::dynamic_pointer_cast<StringLiteral>(tree));
}

NODE_PROCESSOR(BooleanLiteral)
{
    return std::make_shared<BoundBooleanLiteral>(std::dynamic_pointer_cast<BooleanLiteral>(tree));
}

NODE_PROCESSOR(ExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
    auto expr = TRY_AND_TRY_CAST_RETURN(BoundExpression, expr_stmt->expression(), ctx, tree);
    if (expr->type()->type() != PrimitiveType::Void) {
        if (auto func_call = std::dynamic_pointer_cast<BoundFunctionCall>(expr); func_call != nullptr) {
            result.warn(SyntaxError { expr->location(), "Discarding return value of function '{}'", func_call->name() });
            switch (expr->node_type()) {
            case SyntaxNodeType::BoundFunctionCall: {
                expr = std::make_shared<BoundFunctionCall>(func_call, ObjectType::get(PrimitiveType::Void));
                break;
            }
            case SyntaxNodeType::BoundNativeFunctionCall: {
                auto native_call = std::dynamic_pointer_cast<BoundNativeFunctionCall>(expr);
                expr = std::make_shared<BoundNativeFunctionCall>(native_call, ObjectType::get(PrimitiveType::Void));
                break;
            }
            case SyntaxNodeType::BoundIntrinsicCall: {
                auto intrinsic_call = std::dynamic_pointer_cast<BoundIntrinsicCall>(expr);
                expr = std::make_shared<BoundIntrinsicCall>(intrinsic_call, ObjectType::get(PrimitiveType::Void));
                break;
            }
            default:
                break;
            }
        }
    }
    return std::make_shared<BoundExpressionStatement>(expr_stmt, expr);
}

NODE_PROCESSOR(Return)
{
    auto ret_stmt = std::dynamic_pointer_cast<Return>(tree);
    if (ctx.return_type) {
        auto bound_expr = TRY_AND_TRY_CAST_RETURN(BoundExpression, ret_stmt->expression(), ctx, tree);
        bound_expr = TRY(make_expression_for_assignment(bound_expr, ctx.return_type));
        return std::make_shared<BoundReturn>(ret_stmt, bound_expr, ret_stmt->return_error());
    } else {
        if (ret_stmt->expression())
            return SyntaxError { ret_stmt->location(), "Expected void return, got return value '{}'", ret_stmt->expression() };
        return std::make_shared<BoundReturn>(ret_stmt, nullptr, ret_stmt->return_error());
    }
}

NODE_PROCESSOR(IfStatement)
{
    auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
    BoundBranches bound_branches = PROCESS_BRANCHES(tree, if_stmt->branches(), ctx);
    pStatement bound_else_stmt = nullptr;
    if (if_stmt->else_stmt() != nullptr)
        bound_else_stmt = TRY_AND_TRY_CAST_RETURN(Statement, if_stmt->else_stmt(), ctx, tree);
    return std::make_shared<BoundIfStatement>(if_stmt, bound_branches, bound_else_stmt);
}

NODE_PROCESSOR(WhileStatement)
{
    auto stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
    auto bound_condition = TRY_AND_TRY_CAST_RETURN(BoundExpression, stmt->condition(), ctx, tree);
    auto bound_statement = TRY_AND_CAST(Statement, stmt->statement(), ctx);
    if (!bound_statement->is_fully_bound())
        return tree;
    return std::make_shared<BoundWhileStatement>(stmt, bound_condition, bound_statement);
}

NODE_PROCESSOR(ForStatement)
{
    auto stmt = std::dynamic_pointer_cast<ForStatement>(tree);

    bool must_declare_variable = true;
    auto t = stmt->variable()->type();
    std::shared_ptr<ObjectType> var_type { nullptr };
    if (t) {
        var_type = TRY(t->resolve_type());
    } else {
        auto var_decl_maybe = ctx.get(stmt->variable()->name());
        if (var_decl_maybe.has_value()) {
            auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(var_decl_maybe.value());
            if (var_decl != nullptr) {
                var_type = var_decl->type();
                must_declare_variable = false;
            }
        }
    }

    auto bound_range = TRY_AND_TRY_CAST_RETURN(BoundExpression, stmt->range(), ctx, tree);
    auto range_binary_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(bound_range);
    if (range_binary_expr == nullptr)
        return SyntaxError { stmt->location(), "Invalid for-loop range" };
    if (range_binary_expr->op() != BinaryOperator::Range)
        return SyntaxError { stmt->location(), "Invalid for-loop range" };
    auto range_type = range_binary_expr->lhs()->type();

    if (var_type && var_type->type() != PrimitiveType::Any && !range_type->is_assignable_to(var_type)) {
        auto int_literal = std::dynamic_pointer_cast<BoundIntLiteral>(range_binary_expr->lhs());
        if (int_literal != nullptr) {
            auto casted = TRY(int_literal->cast(var_type));
            range_binary_expr = std::make_shared<BoundBinaryExpression>(range_binary_expr->location(), casted,
                range_binary_expr->op(), range_binary_expr->rhs(), range_binary_expr->type());
        } else {
            return SyntaxError { stmt->location(), ErrorCode::TypeMismatch, stmt->variable()->name(), var_type, range_type };
        }
    }
    if (var_type == nullptr) {
        var_type = range_type;
    }

    BindContext for_ctx(ctx);
    auto bound_var_decl = std::make_shared<BoundVariableDeclaration>(stmt->location(),
        std::make_shared<BoundIdentifier>(stmt->location(), stmt->variable()->name(), var_type),
        false, nullptr);
    auto bound_var = std::make_shared<BoundVariable>(stmt->variable(), var_type);
    if (must_declare_variable)
        TRY_RETURN(for_ctx.declare(stmt->variable()->name(), bound_var_decl));
    auto bound_statement = TRY_AND_CAST(Statement, stmt->statement(), for_ctx);
    if (!bound_statement->is_fully_bound())
        return tree;
    return std::make_shared<BoundForStatement>(stmt, bound_var, range_binary_expr, bound_statement, must_declare_variable);
}

NODE_PROCESSOR(CaseStatement)
{
    auto branch = std::dynamic_pointer_cast<CaseStatement>(tree);
    auto bound_condition = TRY_AND_TRY_CAST_RETURN(BoundExpression, branch->condition(), ctx, tree);
    auto bound_statement = TRY_AND_CAST(Statement, branch->statement(), ctx);
    if (!bound_statement->is_fully_bound())
        return tree;
    return std::make_shared<BoundBranch>(branch, bound_condition, bound_statement);
}

NODE_PROCESSOR(DefaultCase)
{
    auto branch = std::dynamic_pointer_cast<DefaultCase>(tree);
    auto bound_statement = TRY_AND_CAST(Statement, branch->statement(), ctx);
    if (!bound_statement->is_fully_bound())
        return tree;
    return std::make_shared<BoundBranch>(branch, nullptr, bound_statement);
}

NODE_PROCESSOR(SwitchStatement)
{
    auto stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
    auto bound_expression = TRY_AND_TRY_CAST_RETURN(BoundExpression, stmt->expression(), ctx, tree);
    BoundBranches bound_branches = PROCESS_BRANCHES(tree, stmt->cases(), ctx);
    auto bound_default_case = PROCESS_BRANCH(tree, stmt->default_case(), ctx);
    return std::make_shared<BoundSwitchStatement>(stmt->location(), bound_expression, bound_branches, bound_default_case);
}

ProcessResult& bind_types(Config const& config, ProcessResult& result)
{
    BindContext root;
    auto new_unbound = std::numeric_limits<int>().max();
    int unbound;
    root.stage = 1;
    pSyntaxNode t = result.value();
    std::cout << "Type checking...\n";
    do {
        unbound = new_unbound;
        root.clear_unresolved();
        process(t, root, result);
        if (result.is_error())
            return result;
        t = result.value();
        assert(t != nullptr);
        auto compilation = std::dynamic_pointer_cast<BoundCompilation>(t);
        assert(compilation != nullptr);
        new_unbound = compilation->unbound_statements();
        std::cout << "Pass " << root.stage++ << ": " << new_unbound << " unbound statements" << '\n';
        if (config.cmdline_flag<bool>("dump-functions"))
            root.dump();
    } while (new_unbound > 0 && new_unbound < unbound);
    std::cout << "\n";

    if (new_unbound > 0) {
        if (config.cmdline_flag<bool>("show-tree"))
            std::cout << "\nNot all types bound:\n"
                      << t->to_xml() << "\n";
        std::cout << "\nUnresolved expressions:\n\n";
        for (auto const& unresolved : root.unresolved()) {
            std::cout << unresolved->to_string() << "\n";
        }
        result.error(SyntaxError { t->location(), "Cyclical dependencies or untyped objects remain" });
        return result;
    }

    if (config.cmdline_flag<bool>("show-tree"))
        std::cout << "\nTypes bound:\n"
                  << t->to_xml() << "\n";

    return result;
}

}
