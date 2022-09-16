/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstddef>
#include <memory>

#include <obelix/Intrinsics.h>
#include <obelix/Syntax.h>
#include <obelix/Type.h>

namespace Obelix {

class BoundExpression : public SyntaxNode {
public:
    BoundExpression()
        : SyntaxNode()
    {
    }

    explicit BoundExpression(Token token, std::shared_ptr<ObjectType> type)
        : SyntaxNode(std::move(token))
        , m_type(std::move(type))
    {
    }

    explicit BoundExpression(std::shared_ptr<Expression> const& expr, std::shared_ptr<ObjectType> type)
        : SyntaxNode(expr->token())
        , m_type(move(type))
    {
    }

    explicit BoundExpression(std::shared_ptr<BoundExpression> const& expr)
        : SyntaxNode(expr->token())
        , m_type(expr->type())
    {
    }

    explicit BoundExpression(Token token, PrimitiveType type)
        : SyntaxNode(std::move(token))
        , m_type(ObjectType::get(type))
    {
    }

    ~BoundExpression()
    {
    }

    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const { return m_type; }
    [[nodiscard]] std::string const& type_name() const { return m_type->name(); }
    [[nodiscard]] std::string attributes() const override { return format(R"(type="{}")", type()->name()); }

private:
    std::shared_ptr<ObjectType> m_type { nullptr };
};

using BoundExpressions = std::vector<std::shared_ptr<BoundExpression>>;

class BoundVariableAccess : public BoundExpression {
public:
    BoundVariableAccess(std::shared_ptr<Expression> reference, std::shared_ptr<ObjectType> type)
        : BoundExpression(reference, move(type))
    {
    }

    BoundVariableAccess(Token token, std::shared_ptr<ObjectType> type)
        : BoundExpression(token, move(type))
    {
    }
};

class BoundIdentifier : public BoundVariableAccess {
public:
    BoundIdentifier(std::shared_ptr<Identifier> const& identifier, std::shared_ptr<ObjectType> type)
        : BoundVariableAccess(identifier, move(type))
        , m_identifier(identifier->name())
    {
    }

    BoundIdentifier(Token token, std::string identifier, std::shared_ptr<ObjectType> type)
        : BoundVariableAccess(std::move(token), move(type))
        , m_identifier(move(identifier))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundIdentifier; }
    [[nodiscard]] std::string const& name() const { return m_identifier; }
    [[nodiscard]] std::string attributes() const override { return format(R"(name="{}" type="{}")", name(), type()); }
    [[nodiscard]] std::string to_string() const override { return format("{}: {}", name(), type()->to_string()); }

private:
    std::string m_identifier;
};

using BoundIdentifiers = std::vector<std::shared_ptr<BoundIdentifier>>;

class BoundVariable : public BoundIdentifier {
public:
    BoundVariable(std::shared_ptr<Variable> const& identifier, std::shared_ptr<ObjectType> type)
        : BoundIdentifier(move(identifier), move(type))
    {
    }

    BoundVariable(Token token, std::string name, std::shared_ptr<ObjectType> type)
        : BoundIdentifier(std::move(token), move(name), move(type))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundVariable; }
};

class BoundMemberAccess : public BoundVariableAccess {
public:
    BoundMemberAccess(std::shared_ptr<BoundExpression> strukt, std::shared_ptr<BoundIdentifier> member)
        : BoundVariableAccess(strukt->token(), member->type())
        , m_struct(move(strukt))
        , m_member(move(member))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundMemberAccess; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& structure() const { return m_struct; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& member() const { return m_member; }
    [[nodiscard]] std::string attributes() const override { return format(R"(type="{}")", type()); }
    [[nodiscard]] Nodes children() const override { return { m_struct, m_member }; }
    [[nodiscard]] std::string to_string() const override { return format("{}.{}: {}", structure(), member(), type_name()); }

private:
    std::shared_ptr<BoundExpression> m_struct;
    std::shared_ptr<BoundIdentifier> m_member;
};

class BoundArrayAccess : public BoundVariableAccess {
public:
    BoundArrayAccess(std::shared_ptr<BoundExpression> array, std::shared_ptr<BoundExpression> index, std::shared_ptr<ObjectType> type)
        : BoundVariableAccess(array->token(), move(type))
        , m_array(move(array))
        , m_index(move(index))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundArrayAccess; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& array() const { return m_array; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& subscript() const { return m_index; }
    [[nodiscard]] std::string attributes() const override { return format(R"(type="{}")", type()); }
    [[nodiscard]] Nodes children() const override { return { m_array, m_index}; }
    [[nodiscard]] std::string to_string() const override { return format("{}[{}]: {}", array(), subscript(), type_name()); }

private:
    std::shared_ptr<BoundExpression> m_array;
    std::shared_ptr<BoundExpression> m_index;
};

class BoundFunctionDecl : public Statement {
public:
    explicit BoundFunctionDecl(std::shared_ptr<SyntaxNode> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : Statement(decl->token())
        , m_identifier(move(identifier))
        , m_parameters(move(parameters))
    {
    }

    explicit BoundFunctionDecl(std::shared_ptr<BoundFunctionDecl> const& decl)
        : Statement(decl->token())
        , m_identifier(decl->identifier())
        , m_parameters(decl->parameters())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundFunctionDecl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return identifier()->type(); }
    [[nodiscard]] std::string type_name() const { return type()->name(); }
    [[nodiscard]] BoundIdentifiers const& parameters() const { return m_parameters; }
    [[nodiscard]] ObjectTypes parameter_types() const
    {
        ObjectTypes ret;
        for (auto& parameter : m_parameters) {
            ret.push_back(parameter->type());
        }
        return ret;
    }

    [[nodiscard]] std::string attributes() const override { return format(R"(name="{}" return_type="{}")", name(), type_name()); }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto& param : m_parameters) {
            ret.push_back(param);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        return format("func {}({}): {}", name(), parameters_to_string(), type());
    }

protected:
    explicit BoundFunctionDecl(std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : Statement(Token {})
        , m_identifier(move(identifier))
        , m_parameters(move(parameters))
    {
    }

    [[nodiscard]] std::string parameters_to_string() const
    {
        std::string ret;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                ret += ", ";
            first = false;
            ret += param->name() + ": " + param->type_name();
        }
        return ret;
    }

private:
    std::shared_ptr<BoundIdentifier> m_identifier;
    BoundIdentifiers m_parameters;
};

class BoundNativeFunctionDecl : public BoundFunctionDecl {
public:
    explicit BoundNativeFunctionDecl(std::shared_ptr<NativeFunctionDecl> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : BoundFunctionDecl(decl, move(identifier), move(parameters))
        , m_native_function_name(decl->native_function_name())
    {
    }

    explicit BoundNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : BoundFunctionDecl(decl, move(identifier), move(parameters))
        , m_native_function_name(decl->native_function_name())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundNativeFunctionDecl; }
    [[nodiscard]] std::string const& native_function_name() const { return m_native_function_name; }
    [[nodiscard]] std::string attributes() const override { return format("{} native_function=\"{}\"", BoundFunctionDecl::attributes(), native_function_name()); }
    [[nodiscard]] std::string to_string() const override { return format("{} -> \"{}\"", BoundFunctionDecl::to_string(), native_function_name()); }

private:
    std::string m_native_function_name;
};

class BoundIntrinsicDecl : public BoundFunctionDecl {
public:
    BoundIntrinsicDecl(std::shared_ptr<SyntaxNode> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : BoundFunctionDecl(decl, move(identifier), move(parameters))
    {
    }

    BoundIntrinsicDecl(std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : BoundFunctionDecl(move(identifier), move(parameters))
    {
    }

    explicit BoundIntrinsicDecl(std::shared_ptr<BoundFunctionDecl> const& decl)
        : BoundFunctionDecl(decl)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundIntrinsicDecl; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("intrinsic {}({}): {}", name(), parameters_to_string(), type());
    }
};

class BoundFunctionDef : public Statement {
public:
    explicit BoundFunctionDef(std::shared_ptr<FunctionDef> const& orig_def, std::shared_ptr<BoundFunctionDecl> func_decl, std::shared_ptr<Statement> statement = nullptr)
        : Statement(orig_def->token())
        , m_function_decl(move(func_decl))
        , m_statement(move(statement))
    {
    }

    explicit BoundFunctionDef(Token token, std::shared_ptr<BoundFunctionDecl> func_decl, std::shared_ptr<Statement> statement = nullptr)
        : Statement(std::move(token))
        , m_function_decl(move(func_decl))
        , m_statement(move(statement))
    {
    }

    explicit BoundFunctionDef(std::shared_ptr<BoundFunctionDef> const& orig_def, std::shared_ptr<Statement> statement = nullptr)
        : Statement(orig_def->token())
        , m_function_decl(orig_def->declaration())
        , m_statement(move(statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundFunctionDef; }
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> const& declaration() const { return m_function_decl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const { return m_function_decl->identifier(); }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return identifier()->type(); }
    [[nodiscard]] BoundIdentifiers const& parameters() const { return m_function_decl->parameters(); }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }
    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        ret.push_back(m_function_decl);
        if (m_statement) {
            ret.push_back(m_statement);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        auto ret = m_function_decl->to_string();
        if (m_statement) {
            ret += "\n";
            ret += m_statement->to_string();
        }
        return ret;
    }

protected:
    std::shared_ptr<BoundFunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
};

class BoundLiteral : public BoundExpression {
public:
    BoundLiteral(Token token, std::shared_ptr<ObjectType> type)
        : BoundExpression(std::move(token), move(type))
    {
    }

    [[nodiscard]] virtual long int_value() const
    {
        fatal("Called int_value() on '{}'", node_type());
    }

    [[nodiscard]] virtual std::string string_value() const
    {
        fatal("Called string_value() on '{}'", node_type());
    }

    [[nodiscard]] virtual bool bool_value() const
    {
        fatal("Called bool_value() on '{}'", node_type());
    }
};

using BoundLiterals = std::vector<std::shared_ptr<BoundLiteral>>;

class BoundIntLiteral : public BoundLiteral {
public:
    explicit BoundIntLiteral(std::shared_ptr<IntLiteral> const& literal, std::shared_ptr<ObjectType> type = nullptr)
        : BoundLiteral(literal->token(), (type) ? move(type) : ObjectType::get("s64"))
    {
        auto int_maybe = token_value<long>(token());
        if (int_maybe.is_error())
            fatal("Error instantiating BoundIntLiteral: {}", int_maybe.error());
        m_int = int_maybe.value();
    }

    explicit BoundIntLiteral(std::shared_ptr<BoundIntLiteral> const& literal, std::shared_ptr<ObjectType> type = nullptr)
        : BoundLiteral(literal->token(), (type) ? move(type) : ObjectType::get("s64"))
    {
    }

    BoundIntLiteral(Token t, long value, std::shared_ptr<ObjectType> type)
        : BoundLiteral(std::move(t), move(type))
        , m_int(value)
    {
    }

    BoundIntLiteral(Token t, unsigned long value, std::shared_ptr<ObjectType> type)
        : BoundLiteral(std::move(t), move(type))
        , m_int(value)
    {
    }

    BoundIntLiteral(Token t, long value)
        : BoundLiteral(std::move(t), ObjectType::get("s64"))
        , m_int(value)
    {
    }

    BoundIntLiteral(Token t, unsigned long value)
        : BoundLiteral(std::move(t), ObjectType::get("u64"))
        , m_int(value)
    {
    }

    BoundIntLiteral(Token t, int value)
        : BoundLiteral(std::move(t), ObjectType::get("s32"))
        , m_int(value)
    {
    }

    BoundIntLiteral(Token t, unsigned value)
        : BoundLiteral(std::move(t), ObjectType::get("u32"))
        , m_int(value)
    {
    }

    BoundIntLiteral(Token t, short value)
        : BoundLiteral(std::move(t), ObjectType::get("s16"))
        , m_int(value)
    {
    }

    BoundIntLiteral(Token t, unsigned char value)
        : BoundLiteral(std::move(t), ObjectType::get("u8"))
        , m_int(value)
    {
    }

    BoundIntLiteral(Token t, signed char value)
        : BoundLiteral(std::move(t), ObjectType::get("s8"))
        , m_int(value)
    {
    }

    BoundIntLiteral(Token t, unsigned short value)
        : BoundLiteral(std::move(t), ObjectType::get("u16"))
        , m_int(value)
    {
    }

    ErrorOr<std::shared_ptr<BoundIntLiteral>, SyntaxError> cast(std::shared_ptr<ObjectType> const&) const;

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundIntLiteral; }
    [[nodiscard]] std::string attributes() const override { return format(R"(value="{}" type="{}")", value(), type()); }
    [[nodiscard]] std::string to_string() const override { return format("{}: {}", value(), type()); }

    template<typename IntType = int>
    [[nodiscard]] IntType value() const { return static_cast<IntType>(m_int); }

    [[nodiscard]] long int_value() const override
    {
        return value<long>();
    }

private:
    long m_int;
};

class BoundStringLiteral : public BoundLiteral {
public:
    explicit BoundStringLiteral(std::shared_ptr<StringLiteral> const& literal)
        : BoundStringLiteral(literal->token(), literal->token().value())
    {
    }

    explicit BoundStringLiteral(Token t, std::string value)
        : BoundLiteral(std::move(t), get_type<std::string>())
        , m_string(move(value))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundStringLiteral; }
    [[nodiscard]] std::string attributes() const override { return format(R"(value="{}" type="{}")", value(), type()); }
    [[nodiscard]] std::string to_string() const override { return format("{}: {}", value(), type()); }
    [[nodiscard]] std::string const& value() const { return m_string; }

    [[nodiscard]] std::string string_value() const override
    {
        return value();
    }

private:
    std::string m_string;
};

class BoundBooleanLiteral : public BoundLiteral {
public:
    explicit BoundBooleanLiteral(std::shared_ptr<BooleanLiteral> const& literal)
        : BoundBooleanLiteral(literal->token(), token_value<bool>(literal->token()).value())
    {
    }

    explicit BoundBooleanLiteral(Token t, bool value)
        : BoundLiteral(std::move(t), get_type<bool>())
        , m_value(value)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundBooleanLiteral; }
    [[nodiscard]] std::string attributes() const override { return format(R"(value="{}" type="{}")", value(), type()); }
    [[nodiscard]] std::string to_string() const override { return format("{}: {}", value(), type()); }
    [[nodiscard]] bool value() const { return m_value; }

    [[nodiscard]] bool bool_value() const override
    {
        return value();
    }

private:
    bool m_value;
};

class BoundTypeLiteral : public BoundLiteral {
public:
    explicit BoundTypeLiteral(Token t, std::shared_ptr<ObjectType> type)
        : BoundLiteral(std::move(t), get_type<ObjectType>())
        , m_type_value(move(type))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundTypeLiteral; }
    [[nodiscard]] std::string attributes() const override { return format(R"(type="{}")", type()); }
    [[nodiscard]] std::string to_string() const override { return type()->to_string(); }
    [[nodiscard]] std::shared_ptr<ObjectType> const& value() const { return m_type_value; }

private:
    std::shared_ptr<ObjectType> m_type_value;
};

class BoundEnumValue : public BoundExpression {
public:
    BoundEnumValue(Token token, std::shared_ptr<ObjectType> enum_type, std::string label, long value)
        : BoundExpression(std::move(token), move(enum_type))
        , m_value(value)
        , m_label(move(label))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundEnumValue; }
    [[nodiscard]] long const& value() const { return m_value; }
    [[nodiscard]] std::string const& label() const { return m_label; }
    [[nodiscard]] std::string attributes() const override { return format(R"(label="{}" value="{}")", label(), value()); }
    [[nodiscard]] std::string to_string() const override { return format(R"({}: {})", label(), value()); }

private:
    long m_value;
    std::string m_label;
};

class BoundBinaryExpression : public BoundExpression {
public:
    BoundBinaryExpression(std::shared_ptr<BinaryExpression> const& expr, std::shared_ptr<BoundExpression> lhs, BinaryOperator op, std::shared_ptr<BoundExpression> rhs, std::shared_ptr<ObjectType> type = nullptr)
        : BoundExpression(expr, move(type))
        , m_lhs(std::move(lhs))
        , m_operator(op)
        , m_rhs(std::move(rhs))
    {
    }

    BoundBinaryExpression(Token token, std::shared_ptr<BoundExpression> lhs, BinaryOperator op, std::shared_ptr<BoundExpression> rhs, std::shared_ptr<ObjectType> type)
        : BoundExpression(std::move(token), move(type))
        , m_lhs(std::move(lhs))
        , m_operator(op)
        , m_rhs(std::move(rhs))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundBinaryExpression; }
    [[nodiscard]] std::string attributes() const override { return format(R"(operator="{}" type="{}")", m_operator, type()); }
    [[nodiscard]] Nodes children() const override { return { m_lhs, m_rhs }; }
    [[nodiscard]] std::string to_string() const override { return format("({} {} {}): {}", lhs(), op(), rhs(), type()); }

    [[nodiscard]] std::shared_ptr<BoundExpression> const& lhs() const { return m_lhs; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& rhs() const { return m_rhs; }
    [[nodiscard]] BinaryOperator op() const { return m_operator; }

private:
    std::shared_ptr<BoundExpression> m_lhs;
    BinaryOperator m_operator;
    std::shared_ptr<BoundExpression> m_rhs;
};

class BoundUnaryExpression : public BoundExpression {
public:
    BoundUnaryExpression(std::shared_ptr<UnaryExpression> const& expr, std::shared_ptr<BoundExpression> operand, UnaryOperator op, std::shared_ptr<ObjectType> type = nullptr)
        : BoundExpression(expr, move(type))
        , m_operator(op)
        , m_operand(std::move(operand))
    {
    }

    BoundUnaryExpression(Token token, std::shared_ptr<BoundExpression> operand, UnaryOperator op, std::shared_ptr<ObjectType> type = nullptr)
        : BoundExpression(std::move(token), move(type))
        , m_operator(op)
        , m_operand(std::move(operand))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundUnaryExpression; }
    [[nodiscard]] std::string attributes() const override { return format(R"(operator="{}" type="{}")", m_operator, type()); }
    [[nodiscard]] Nodes children() const override { return { m_operand }; }
    [[nodiscard]] std::string to_string() const override { return format("{} ({}): {}", op(), operand()->to_string(), type()); }
    [[nodiscard]] UnaryOperator op() const { return m_operator; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& operand() const { return m_operand; }

private:
    UnaryOperator m_operator;
    std::shared_ptr<BoundExpression> m_operand;
};

class BoundFunctionCall : public BoundExpression {
public:
    BoundFunctionCall(std::shared_ptr<FunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundFunctionDecl> decl)
        : BoundExpression(call, decl->type())
        , m_name(call->name())
        , m_arguments(move(arguments))
        , m_declaration(move(decl))
    {
    }

    BoundFunctionCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundFunctionDecl> const& decl = nullptr)
        : BoundExpression(call)
        , m_name(call->name())
        , m_arguments(move(arguments))
        , m_declaration((decl) ? decl : call->declaration())
    {
    }

    BoundFunctionCall(Token token, std::shared_ptr<BoundFunctionDecl> const& decl, BoundExpressions arguments)
        : BoundExpression(std::move(token), decl->type())
        , m_name(decl->name())
        , m_arguments(move(arguments))
        , m_declaration(decl)
    {
    }

    [[nodiscard]] std::string attributes() const override
    {
        return format(R"(name="{}" type="{}")", name(), type());
    }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto& arg : m_arguments) {
            ret.push_back(arg);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        Strings args;
        for (auto& arg : m_arguments) {
            args.push_back(arg->to_string());
        }
        return format("{}({}): {}", name(), join(args, ","), type());
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundFunctionCall; }
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] BoundExpressions const& arguments() const { return m_arguments; }
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> const& declaration() const { return m_declaration; }

    [[nodiscard]] ObjectTypes argument_types() const
    {
        ObjectTypes ret;
        for (auto& arg : arguments()) {
            ret.push_back(arg->type());
        }
        return ret;
    }

private:
    std::string m_name;
    BoundExpressions m_arguments;
    std::shared_ptr<BoundFunctionDecl> m_declaration;
};

class BoundNativeFunctionCall : public BoundFunctionCall {
public:
    BoundNativeFunctionCall(std::shared_ptr<FunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundNativeFunctionDecl> decl)
        : BoundFunctionCall(call, move(arguments), move(decl))
    {
    }

    BoundNativeFunctionCall(std::shared_ptr<BoundNativeFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundFunctionDecl> const& decl = nullptr)
        : BoundFunctionCall(call, move(arguments), decl)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundNativeFunctionCall; }
};

class BoundIntrinsicCall : public BoundFunctionCall {
public:
    BoundIntrinsicCall(std::shared_ptr<FunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundIntrinsicDecl> decl, IntrinsicType intrinsic)
        : BoundFunctionCall(call, move(arguments), move(decl))
        , m_intrinsic(intrinsic)
    {
    }

    BoundIntrinsicCall(std::shared_ptr<BoundIntrinsicCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundIntrinsicDecl> const& decl = nullptr)
        : BoundFunctionCall(call, move(arguments), decl)
        , m_intrinsic(call->intrinsic())
    {
    }

    BoundIntrinsicCall(Token token, std::shared_ptr<BoundIntrinsicDecl> const& decl, BoundExpressions arguments, IntrinsicType intrinsic_type)
        : BoundFunctionCall(std::move(token), decl, move(arguments))
        , m_intrinsic(intrinsic_type)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundIntrinsicCall; }
    [[nodiscard]] IntrinsicType intrinsic() const { return m_intrinsic; }

private:
    IntrinsicType m_intrinsic;
};

class BoundExpressionStatement : public Statement {
public:
    BoundExpressionStatement(std::shared_ptr<ExpressionStatement> const& stmt, std::shared_ptr<BoundExpression> expression)
        : Statement(stmt->token())
        , m_expression(move(expression))
    {
    }

    BoundExpressionStatement(Token token, std::shared_ptr<BoundExpression> expression)
        : Statement(std::move(token))
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundExpressionStatement; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }
    [[nodiscard]] Nodes children() const override { return { m_expression }; }
    [[nodiscard]] std::string to_string() const override { return m_expression->to_string(); }

private:
    std::shared_ptr<BoundExpression> m_expression;
};

class BoundStructDefinition : public Statement {
public:
    BoundStructDefinition(std::shared_ptr<StructDefinition> const& struct_def, std::shared_ptr<ObjectType> type)
        : Statement(struct_def->token())
        , m_name(struct_def->name())
        , m_type(move(type))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundStructDefinition; }
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return m_type; }
    [[nodiscard]] std::string attributes() const override { return format(R"(name="{}")", name()); }

    [[nodiscard]] std::string to_string() const override
    {
        auto ret = format("struct {} {", name());
        for (auto const& field : type()->fields()) {
            ret += " ";
            ret += format("{}: {}", field.name, field.type->to_string());
        }
        ret += " }";
        return ret;
    }

private:
    std::string m_name;
    std::shared_ptr<ObjectType> m_type;
};

class BoundEnumValueDef : public SyntaxNode {
public:
    BoundEnumValueDef(Token token, std::string label, long value)
        : SyntaxNode(std::move(token))
        , m_value(value)
        , m_label(move(label))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundEnumValueDef; }
    [[nodiscard]] long const& value() const { return m_value; }
    [[nodiscard]] std::string const& label() const { return m_label; }
    [[nodiscard]] std::string attributes() const override { return format(R"(label="{}" value="{}")", label(), value()); }
    [[nodiscard]] std::string to_string() const override { return format(R"({}: {})", label(), value()); }

private:
    long m_value;
    std::string m_label;
};

using BoundEnumValueDefs = std::vector<std::shared_ptr<BoundEnumValueDef>>;

class BoundEnumDef : public Statement {
public:
    BoundEnumDef(std::shared_ptr<EnumDef> const& enum_def, std::shared_ptr<ObjectType> type, BoundEnumValueDefs values)
        : Statement(enum_def->token())
        , m_name(enum_def->name())
        , m_type(move(type))
        , m_values(move(values))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundEnumDef; }
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return m_type; }
    [[nodiscard]] BoundEnumValueDefs const& values() const { return m_values; }
    [[nodiscard]] std::string attributes() const override { return format(R"(name="{}")", name()); }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto const& value : m_values) {
            ret.push_back(value);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        auto values = m_type->template_argument_values<NVP>("values");
        return format("enum {} { {} }", name(), join(values, ", ", [](NVP const& value) {
            return format("{}: {}", value.first, value.second);
        }));
    }

private:
    std::string m_name;
    std::shared_ptr<ObjectType> m_type;
    BoundEnumValueDefs m_values;
};

class BoundVariableDeclaration : public Statement {
public:
    explicit BoundVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
        : Statement(decl->token())
        , m_variable(move(variable))
        , m_const(decl->is_const())
        , m_expression(move(expr))
    {
    }

    explicit BoundVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
        : Statement(std::move(token))
        , m_variable(move(variable))
        , m_const(is_const)
        , m_expression(move(expr))
    {
    }

    [[nodiscard]] std::string attributes() const override
    {
        return format(R"(name="{}" type="{}" is_const="{}")", name(), type(), is_const());
    }

    [[nodiscard]] Nodes children() const override
    {
        if (m_expression)
            return { m_expression };
        return {};
    }

    [[nodiscard]] std::string to_string() const override
    {
        auto ret = format("{} {}: {}", (is_const()) ? "const" : "var", name(), type());
        if (m_expression)
            ret += format(" = {}", m_expression->to_string());
        return ret;
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundVariableDeclaration; }
    [[nodiscard]] std::string const& name() const { return m_variable->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return m_variable->type(); }
    [[nodiscard]] bool is_const() const { return m_const; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& variable() const { return m_variable; }

private:
    std::shared_ptr<BoundIdentifier> m_variable;
    bool m_const { false };
    std::shared_ptr<BoundExpression> m_expression;
};

class BoundStaticVariableDeclaration : public BoundVariableDeclaration {
public:
    BoundStaticVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
        : BoundVariableDeclaration(decl, move(variable), move(expr))
    {
    }

    BoundStaticVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
        : BoundVariableDeclaration(std::move(token), move(variable), is_const, move(expr))
    {
    }

    [[nodiscard]] std::string to_string() const override { return "static " + BoundVariableDeclaration::to_string(); }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundStaticVariableDeclaration; }
};

class BoundLocalVariableDeclaration : public BoundVariableDeclaration {
public:
    BoundLocalVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
        : BoundVariableDeclaration(decl, move(variable), move(expr))
    {
    }

    BoundLocalVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
        : BoundVariableDeclaration(std::move(token), move(variable), is_const, move(expr))
    {
    }

    [[nodiscard]] std::string to_string() const override { return "global " + BoundVariableDeclaration::to_string(); }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundLocalVariableDeclaration; }
};

class BoundGlobalVariableDeclaration : public BoundVariableDeclaration {
public:
    BoundGlobalVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
        : BoundVariableDeclaration(decl, move(variable), move(expr))
    {
    }

    BoundGlobalVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
        : BoundVariableDeclaration(std::move(token), move(variable), is_const, move(expr))
    {
    }

    [[nodiscard]] std::string to_string() const override { return "global " + BoundVariableDeclaration::to_string(); }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundGlobalVariableDeclaration; }
};

class BoundReturn : public Statement {
public:
    BoundReturn(std::shared_ptr<SyntaxNode> const& ret, std::shared_ptr<BoundExpression> expression)
        : Statement(ret->token())
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] Nodes children() const override
    {
        if (m_expression)
            return { m_expression };
        return {};
    }

    [[nodiscard]] std::string to_string() const override
    {
        std::string ret = "return";
        if (m_expression)
            ret = format("{} {}", ret, m_expression->to_string());
        return ret;
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundReturn; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }

private:
    std::shared_ptr<BoundExpression> m_expression;
};

class BoundBranch : public Statement {
public:
    BoundBranch(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> bound_statement)
        : Statement(node->token())
        , m_condition(move(condition))
        , m_statement(move(bound_statement))
    {
    }

    BoundBranch(Token token, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> bound_statement)
        : Statement(std::move(token))
        , m_condition(move(condition))
        , m_statement(move(bound_statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundBranch; }
    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        if (m_condition)
            ret.push_back(m_condition);
        ret.push_back(m_statement);
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        if (m_condition)
            return format("if ({})\n{}", m_condition->to_string(), m_statement->to_string());
        return format("else\n{}", m_statement->to_string());
    }

    [[nodiscard]] std::shared_ptr<BoundExpression> const& condition() const { return m_condition; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }

private:
    std::shared_ptr<BoundExpression> m_condition { nullptr };
    std::shared_ptr<Statement> m_statement;
};

using BoundBranches = std::vector<std::shared_ptr<BoundBranch>>;

class BoundIfStatement : public Statement {
public:
    BoundIfStatement(std::shared_ptr<IfStatement> const& if_stmt, BoundBranches branches, std::shared_ptr<Statement> else_stmt);
    BoundIfStatement(Token token, BoundBranches branches, std::shared_ptr<Statement> else_stmt = nullptr);
    [[nodiscard]] SyntaxNodeType node_type() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] BoundBranches const& branches() const;
private:
    BoundBranches m_branches {};
    std::shared_ptr<Statement> m_else {};
};

class BoundWhileStatement : public Statement {
public:
    BoundWhileStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> stmt);
    [[nodiscard]] SyntaxNodeType node_type() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& condition() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;
    [[nodiscard]] std::string to_string() const override;
private:
    std::shared_ptr<BoundExpression> m_condition;
    std::shared_ptr<Statement> m_stmt;
};

class BoundForStatement : public Statement {
public:
    BoundForStatement(std::shared_ptr<ForStatement> const& orig_for_stmt, std::shared_ptr<BoundVariable> variable, std::shared_ptr<BoundExpression> range, std::shared_ptr<Statement> stmt, bool must_declare)
        : Statement(orig_for_stmt->token())
        , m_variable(move(variable))
        , m_range(move(range))
        , m_stmt(move(stmt))
        , m_must_declare_variable(must_declare)
    {
    }

    BoundForStatement(std::shared_ptr<BoundForStatement> const& orig_for_stmt, std::shared_ptr<BoundVariable> variable, std::shared_ptr<BoundExpression> range, std::shared_ptr<Statement> stmt)
        : Statement(orig_for_stmt->token())
        , m_variable(move(variable))
        , m_range(move(range))
        , m_stmt(move(stmt))
        , m_must_declare_variable(orig_for_stmt->must_declare_variable())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundForStatement; }
    [[nodiscard]] std::string attributes() const override { return format(R"(variable="{}")", m_variable); }
    [[nodiscard]] Nodes children() const override { return { m_range, m_stmt }; }
    [[nodiscard]] std::shared_ptr<BoundVariable> const& variable() const { return m_variable; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& range() const { return m_range; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_stmt; }
    [[nodiscard]] bool must_declare_variable() const { return m_must_declare_variable; }

    [[nodiscard]] std::string to_string() const override
    {
        return format("for ({} in {})\n{}", m_variable, m_range->to_string(), m_stmt->to_string());
    }

private:
    std::shared_ptr<BoundVariable> m_variable;
    std::shared_ptr<BoundExpression> m_range;
    std::shared_ptr<Statement> m_stmt;
    bool m_must_declare_variable { false };
};

class BoundCaseStatement : public BoundBranch {
public:
    BoundCaseStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> case_expression, std::shared_ptr<Statement> stmt)
        : BoundBranch(node, move(case_expression), move(stmt))
    {
    }

    BoundCaseStatement(Token token, std::shared_ptr<BoundExpression> case_expression, std::shared_ptr<Statement> stmt)
        : BoundBranch(std::move(token), move(case_expression), move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundCaseStatement; }
};

typedef std::vector<std::shared_ptr<BoundCaseStatement>> BoundCaseStatements;

class BoundDefaultCase : public BoundBranch {
public:
    BoundDefaultCase(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Statement> const& stmt)
        : BoundBranch(node, nullptr, move(stmt))
    {
    }

    BoundDefaultCase(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> const&, std::shared_ptr<Statement> const& stmt)
        : BoundBranch(node, nullptr, move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundDefaultCase; }
};

class BoundSwitchStatement : public Statement {
public:
    BoundSwitchStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> switch_expr, BoundCaseStatements cases, std::shared_ptr<BoundDefaultCase> default_case)
        : Statement(node->token())
        , m_switch_expression(move(switch_expr))
        , m_cases(move(cases))
        , m_default(move(default_case))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::SwitchStatement; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_switch_expression; }
    [[nodiscard]] BoundCaseStatements const& cases() const { return m_cases; }
    [[nodiscard]] std::shared_ptr<BoundDefaultCase> const& default_case() const { return m_default; }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        ret.push_back(m_switch_expression);
        for (auto& case_stmt : m_cases) {
            ret.push_back(case_stmt);
        }
        if (m_default)
            ret.push_back(m_default);
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        auto ret = format("switch ({}) {{\n", expression()->to_string());
        for (auto& case_stmt : m_cases) {
            ret += '\n';
            ret += case_stmt->to_string();
        }
        if (m_default) {
            ret += '\n';
            ret += m_default->to_string();
        }
        return ret + "}";
    }

private:
    std::shared_ptr<BoundExpression> m_switch_expression;
    BoundCaseStatements m_cases {};
    std::shared_ptr<BoundDefaultCase> m_default {};
};

class BoundAssignment : public BoundExpression {
public:
    BoundAssignment(Token token, std::shared_ptr<BoundVariableAccess> assignee, std::shared_ptr<BoundExpression> expression)
        : BoundExpression(std::move(token), assignee->type())
        , m_assignee(move(assignee))
        , m_expression(move(expression))
    {
        debug(parser, "m_identifier->type() = {} m_expression->type() = {}", m_assignee->type(), m_expression->type());
        assert(*(m_assignee->type()) == *(m_expression->type()));
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundAssignment; }
    [[nodiscard]] std::shared_ptr<BoundVariableAccess> const& assignee() const { return m_assignee; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }
    [[nodiscard]] Nodes children() const override { return { assignee(), expression() }; }
    [[nodiscard]] std::string to_string() const override { return format("{} = {}", assignee()->to_string(), expression()->to_string()); }

private:
    std::shared_ptr<BoundVariableAccess> m_assignee;
    std::shared_ptr<BoundExpression> m_expression;
};

}
