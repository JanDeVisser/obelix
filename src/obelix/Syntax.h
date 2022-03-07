/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <core/List.h>
#include <core/Logging.h>
#include <core/NativeFunction.h>
#include <core/Range.h>
#include <core/Type.h>
#include <lexer/Token.h>
#include <obelix/Symbol.h>
#include <obelix/SyntaxNodeType.h>
#include <string>

namespace Obelix {

extern_logging_category(parser);

typedef std::vector<std::string> Strings;

class SyntaxNode;
class Module;
class ExpressionType;
using Nodes = std::vector<std::shared_ptr<SyntaxNode>>;
using Types = std::vector<std::shared_ptr<ExpressionType>>;

static inline std::string pad(int num)
{
    std::string ret;
    for (auto ix = 0; ix < num; ++ix)
        ret += ' ';
    return ret;
}

#define TO_OBJECT(expr)                                     \
    ({                                                      \
        auto _to_obj_temp_maybe = TRY((expr)->to_object()); \
        if (!_to_obj_temp_maybe.has_value())                \
            return std::optional<Obj> {};                   \
        _to_obj_temp_maybe.value();                         \
    })

class SyntaxNode {
public:
    explicit SyntaxNode(Token token = {});
    virtual ~SyntaxNode() = default;

    [[nodiscard]] virtual std::string text_contents() const
    {
        return "";
    }

    [[nodiscard]] virtual std::string attributes() const
    {
        return "";
    }

    [[nodiscard]] virtual Nodes children() const
    {
        return Nodes {};
    }

    [[nodiscard]] std::string to_xml(int indent) const
    {
        auto ret = format("{}<{}", pad(indent), node_type());
        auto attrs = attributes();
        auto child_nodes = children();
        auto text = text_contents();
        if (!attrs.empty()) {
            ret += " " + attrs;
        }
        if (text.empty() && child_nodes.empty())
            return ret + "/>";
        ret += ">\n";
        for (auto& child : child_nodes) {
            ret += child->to_xml(indent + 2);
            ret += "\n";
        }
        return ret + format("{}\n{}</{}>", text, pad(indent), node_type());
    }

    [[nodiscard]] virtual std::string to_string() const { return SyntaxNodeType_name(node_type()); }

    [[nodiscard]] std::string to_xml() const { return to_xml(0); }
    [[nodiscard]] virtual SyntaxNodeType node_type() const = 0;
    [[nodiscard]] Token const& token() const { return m_token; }

private:
    Token m_token;
};

class Statement : public SyntaxNode {
public:
    Statement() = default;
    explicit Statement(Token token)
        : SyntaxNode(std::move(token))
    {
    }
};

using ExpressionTypes = std::vector<std::shared_ptr<ExpressionType>>;

class ExpressionType : public SyntaxNode {
public:
    ExpressionType(Token token, std::string type_name, ExpressionTypes template_arguments)
        : SyntaxNode(std::move(token))
        , m_type_name(move(type_name))
        , m_template_args(move(template_arguments))
    {
    }

    explicit ExpressionType(Token token, std::string type_name)
        : SyntaxNode(std::move(token))
        , m_type_name(move(type_name))
    {
    }

    explicit ExpressionType(Token token, ObelixType type)
        : SyntaxNode(std::move(token))
        , m_type_name(ObelixType_name(type))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ExpressionType; }
    [[nodiscard]] bool is_template_instantiation() const { return !m_template_args.empty(); }
    [[nodiscard]] std::string const& type_name() const { return m_type_name; }
    [[nodiscard]] ExpressionTypes const& template_arguments() const { return m_template_args; }
    [[nodiscard]] std::string attributes() const override { return format(R"(type_name="{}")", type_name()); }
    [[nodiscard]] ErrorOr<std::shared_ptr<ObjectType>> resolve_type() const
    {
        ObjectTypes args;
        for (auto& arg : template_arguments()) {
            auto arg_type_or_error = arg->resolve_type();
            if (arg_type_or_error.is_error())
                return arg_type_or_error.error();
            args.push_back(arg_type_or_error.value());
        }
        return ObjectType::resolve(type_name(), args);
    }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto& parameter : template_arguments()) {
            ret.push_back(parameter);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        auto ret = type_name();
        auto glue = '<';
        for (auto& parameter : template_arguments()) {
            ret += glue;
            glue = ',';
            ret += parameter->to_string();
        }
        if (is_template_instantiation())
            ret += '>';
        return ret;
    }

private:
    std::string m_type_name;
    ExpressionTypes m_template_args {};
};

class Expression : public SyntaxNode {
public:
    explicit Expression(Token token = {}, std::shared_ptr<ExpressionType> type = nullptr)
        : SyntaxNode(std::move(token))
        , m_type(move(type))
    {
    }

    [[nodiscard]] std::shared_ptr<ExpressionType> const& type() const { return m_type; }
    [[nodiscard]] std::string type_name() const { return (m_type) ? type()->type_name() : "[Unresolved]"; }
    [[nodiscard]] bool is_typed() { return m_type != nullptr; }
    [[nodiscard]] virtual ErrorOr<std::optional<Obj>> to_object() const = 0;
    [[nodiscard]] std::string attributes() const override { return format(R"(type="{}")", type_name()); }

private:
    std::shared_ptr<ExpressionType> m_type { nullptr };
};

class Identifier : public Expression {
public:
    explicit Identifier(Token token, std::string name, std::shared_ptr<ExpressionType> type = nullptr)
        : Expression(std::move(token), move(type))
        , m_identifier(move(name))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Identifier; }
    [[nodiscard]] std::string const& name() const { return m_identifier; }
    [[nodiscard]] std::string attributes() const override { return format(R"(name="{}" type="{}")", name(), type()->to_string()); }
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return std::optional<Obj> {}; }
    [[nodiscard]] std::string to_string() const override { return format("{}: {}", name(), type()); }

private:
    std::string m_identifier;
};

using Identifiers = std::vector<std::shared_ptr<Identifier>>;

class Import : public Statement {
public:
    explicit Import(Token token, std::string name)
        : Statement(std::move(token))
        , m_name(move(name))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Import; }
    [[nodiscard]] std::string attributes() const override { return format("module=\"{}\"", m_name); }
    [[nodiscard]] std::string to_string() const override { return format("import {}", m_name); }
    [[nodiscard]] std::string const& name() const { return m_name; }

private:
    std::string m_name;
};

class Pass : public Statement {
public:
    explicit Pass(Token token)
        : Statement(std::move(token))
    {
    }

    explicit Pass(std::shared_ptr<Statement> elided_statement)
        : Statement(elided_statement->token())
        , m_elided_statement(move(elided_statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Pass; }

    [[nodiscard]] std::string text_contents() const override
    {
        if (!m_elided_statement)
            return "";
        return format("{}/* {} */", m_elided_statement->to_string());
    }

    [[nodiscard]] std::string to_string() const override { return text_contents(); }

private:
    std::shared_ptr<Statement> m_elided_statement;
};

class Goto;

class Label : public Statement {
public:
    explicit Label(Token token = {})
        : Statement(std::move(token))
        , m_label_id(reserve_id())
    {
    }

    explicit Label(std::shared_ptr<Goto> const& goto_stmt);

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Label; }
    [[nodiscard]] std::string attributes() const override { return format("id=\"{}\"", m_label_id); }
    [[nodiscard]] std::string to_string() const override { return format("{}:", label_id()); }
    [[nodiscard]] int label_id() const { return m_label_id; }
    static int reserve_id() { return m_current_id++; }

private:
    static int m_current_id;
    int m_label_id;
};

class Goto : public Statement {
public:
    explicit Goto(Token token = {}, std::shared_ptr<Label> const& label = nullptr)
        : Statement(std::move(token))
        , m_label_id((label) ? label->label_id() : Label::reserve_id())
    {
    }

    [[nodiscard]] std::string attributes() const override { return format("label=\"{}\"", m_label_id); }
    [[nodiscard]] std::string to_string() const override { return format("goto {}", label_id()); }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Goto; }
    [[nodiscard]] int label_id() const { return m_label_id; }

private:
    int m_label_id;
};

typedef std::vector<std::shared_ptr<Statement>> Statements;

class Block : public Statement {
public:
    explicit Block(Token token, Statements statements)
        : Statement(std::move(token))
        , m_statements(move(statements))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Block; }
    [[nodiscard]] Statements const& statements() const { return m_statements; }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto& statement : m_statements) {
            ret.push_back(statement);
        }
        return ret;
    }
    [[nodiscard]] std::string to_string() const override
    {
        std::string ret = "{\n";
        for (auto& statement : m_statements) {
            ret += statement->to_string() + "\n";
        }
        return ret + "}";
    }

protected:
    Statements m_statements {};
};

class FunctionBlock : public Block {
public:
    explicit FunctionBlock(Token token, Statements statements)
        : Block(std::move(token), move(statements))
    {
    }

    explicit FunctionBlock(Token token, std::shared_ptr<Statement> statement)
        : Block(std::move(token), Statements { move(statement) })
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionBlock; }
};

class Module : public Block {
public:
    Module(Statements const& statements, std::string name)
        : Block({}, statements)
        , m_name(move(name))
    {
    }

    Module(Token token, Statements const& statements, std::string name)
        : Block(std::move(token), statements)
        , m_name(move(name))
    {
    }

    Module(std::shared_ptr<Module> const& original, std::vector<std::shared_ptr<Statement>> const& statements)
        : Block(original->token(), statements)
        , m_name(original->name())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Module; }
    [[nodiscard]] std::string attributes() const override { return format("name=\"{}\"", m_name); }
    [[nodiscard]] std::string to_string() const override
    {
        return format("module {} {}", name(), Block::to_string());
    }

    [[nodiscard]] const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

using Modules = std::vector<std::shared_ptr<Module>>;

class Compilation : public Module {
public:
    Compilation(std::shared_ptr<Module> const& root, Modules modules)
        : Module(root->statements(), "")
        , m_modules(move(modules))
    {
    }

    Compilation(Statements const& statements, Modules modules)
        : Module(statements, "")
        , m_modules(move(modules))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Compilation; }
    [[nodiscard]] Modules const& modules() const { return m_modules; }
    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto& module : m_modules) {
            ret.push_back(module);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        std::string ret = Module::to_string();
        for (auto& module : m_modules) {
            ret += "\n";
            ret += module->to_string();
        }
        return ret;
    }

private:
    Modules m_modules;
};

class FunctionDecl : public SyntaxNode {
public:
    explicit FunctionDecl(Token token, std::shared_ptr<Identifier> identifier, Identifiers parameters)
        : SyntaxNode(std::move(token))
        , m_identifier(move(identifier))
        , m_parameters(move(parameters))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionDecl; }
    [[nodiscard]] std::shared_ptr<Identifier> const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ExpressionType> type() const { return identifier()->type(); }
    [[nodiscard]] std::string type_name() const { return (type()) ? type()->type_name() : "[Unresolved]"; }
    [[nodiscard]] Identifiers const& parameters() const { return m_parameters; }
    [[nodiscard]] ExpressionTypes parameter_types() const
    {
        ExpressionTypes ret;
        for (auto& parameter : m_parameters) {
            ret.push_back(parameter->type());
        }
        return ret;
    }

    [[nodiscard]] std::string attributes() const override { return format(R"(name="{}" return_type="{}")", name(), type_name()); }

    [[nodiscard]] std::string text_contents() const override
    {
        std::string ret;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                ret += "\n";
            first = false;
            ret += format(R"(<Parameter name="{}" type="{}"/>)", param->name(), param->type_name());
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        return format("func {}({}): {}", name(), parameters_to_string(), type());
    }

private:
    [[nodiscard]] std::string parameters_to_string() const
    {
        std::string ret;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                ret += ", ";
            first = false;
            ret += param->name() + ": " + param->type()->to_string();
        }
        return ret;
    }

    std::shared_ptr<Identifier> m_identifier;
    Identifiers m_parameters;
};

class NativeFunctionDecl : public FunctionDecl {
public:
    explicit NativeFunctionDecl(Token token, std::shared_ptr<Identifier> identifier, Identifiers parameters, std::string native_function)
        : FunctionDecl(std::move(token), move(identifier), move(parameters))
        , m_native_function_name(move(native_function))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::NativeFunctionDecl; }
    [[nodiscard]] std::string const& native_function_name() const { return m_native_function_name; }
    [[nodiscard]] std::string attributes() const override { return format("{} native_function=\"{}\"", FunctionDecl::attributes(), native_function_name()); }
    [[nodiscard]] std::string to_string() const override { return format("{} -> \"{}\"", FunctionDecl::to_string(), native_function_name()); }

private:
    std::string m_native_function_name;
};

class IntrinsicDecl : public FunctionDecl {
public:
    explicit IntrinsicDecl(Token token, std::shared_ptr<Identifier> identifier, Identifiers parameters)
        : FunctionDecl(std::move(token), move(identifier), move(parameters))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::IntrinsicDecl; }
    [[nodiscard]] std::string to_string() const override { return format("{} intrinsic", FunctionDecl::to_string()); }
};

class FunctionDef : public Statement {
public:
    explicit FunctionDef(Token token, std::shared_ptr<FunctionDecl> func_decl, std::shared_ptr<Statement> statement = nullptr)
        : Statement(std::move(token))
        , m_function_decl(move(func_decl))
        , m_statement(move(statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionDef; }
    [[nodiscard]] std::shared_ptr<FunctionDecl> const& declaration() const { return m_function_decl; }
    [[nodiscard]] std::shared_ptr<Identifier> const& identifier() const { return m_function_decl->identifier(); }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ExpressionType> type() const { return identifier()->type(); }
    [[nodiscard]] Identifiers const& parameters() const { return m_function_decl->parameters(); }
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
    std::shared_ptr<FunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
};

class ExpressionStatement : public Statement {
public:
    explicit ExpressionStatement(std::shared_ptr<Expression> expression)
        : Statement(expression->token())
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ExpressionStatement; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }
    [[nodiscard]] Nodes children() const override { return { m_expression }; }
    [[nodiscard]] std::string to_string() const override { return m_expression->to_string(); }

private:
    std::shared_ptr<Expression> m_expression;
};

class Literal : public Expression {
public:
    explicit Literal(Token const& token)
        : Expression(token, std::make_shared<ExpressionType>(token, token.to_object()->type()))
        , m_literal(token.to_object())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Literal; }
    [[nodiscard]] std::string text_contents() const override { return format("{}", m_literal->to_string()); }
    [[nodiscard]] Obj const& literal() const { return m_literal; }
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return literal(); }
    [[nodiscard]] std::string to_string() const override { return format("{}: {}", literal()->to_string(), type()); }

private:
    Obj m_literal;
};

typedef std::vector<std::shared_ptr<Expression>> Expressions;

class This : public Expression {
public:
    explicit This()
        : Expression()
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::This; }

    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return std::optional<Obj> {}; }
    [[nodiscard]] std::string to_string() const override { return "this"; }
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(std::shared_ptr<Expression> lhs, Token op, std::shared_ptr<Expression> rhs, std::shared_ptr<ExpressionType> type = nullptr)
        : Expression(op, move(type))
        , m_lhs(std::move(lhs))
        , m_operator(std::move(op))
        , m_rhs(std::move(rhs))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BinaryExpression; }
    [[nodiscard]] std::string attributes() const override { return format(R"(operator="{}" type="{}")", m_operator.value(), type()); }
    [[nodiscard]] Nodes children() const override { return { m_lhs, m_rhs }; }
    [[nodiscard]] std::string to_string() const override { return format("{} {} {}", lhs()->to_string(), op().value(), rhs()->to_string()); }
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return std::optional<Obj> {}; }

    [[nodiscard]] std::shared_ptr<Expression> const& lhs() const { return m_lhs; }
    [[nodiscard]] std::shared_ptr<Expression> const& rhs() const { return m_rhs; }
    [[nodiscard]] Token op() const { return m_operator; }

private:
    std::shared_ptr<Expression> m_lhs;
    Token m_operator;
    std::shared_ptr<Expression> m_rhs;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(Token op, std::shared_ptr<Expression> operand, std::shared_ptr<ExpressionType> type = nullptr)
        : Expression(op, move(type))
        , m_operator(std::move(op))
        , m_operand(std::move(operand))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::UnaryExpression; }
    [[nodiscard]] std::string attributes() const override { return format(R"(operator="{}" type="{}")", m_operator.value(), type()); }
    [[nodiscard]] Nodes children() const override { return { m_operand }; }
    [[nodiscard]] std::string to_string() const override { return format("{} {}", op().value(), operand()->to_string()); }
    [[nodiscard]] Token op() const { return m_operator; }
    [[nodiscard]] std::shared_ptr<Expression> const& operand() const { return m_operand; }

    ErrorOr<std::optional<Obj>> to_object() const override
    {
        auto operand = TO_OBJECT(m_operand);
        if (operand->is_exception())
            return operand;
        auto ret_maybe = operand->evaluate(m_operator.value());
        if (!ret_maybe.has_value())
            return make_obj<Exception>(ErrorCode::OperatorUnresolved, m_operator.value(), operand);
        return ret_maybe.value();
    }

private:
    Token m_operator;
    std::shared_ptr<Expression> m_operand;
};

class FunctionCall : public Expression {
public:
    FunctionCall(Token token, std::string function, Expressions arguments = {})
        : Expression(std::move(token))
        , m_name(move(function))
        , m_arguments(move(arguments))
    {
    }

    [[nodiscard]] std::string attributes() const override
    {
        return format(R"(name="{}" type="{}")", name(), type());
    }

    [[nodiscard]] std::string text_contents() const override
    {
        std::string ret = "<Arguments";
        if (m_arguments.empty())
            return ret + "/>";
        ret += ">";
        for (auto& arg : m_arguments) {
            ret += '\n';
            ret += arg->to_xml(2);
        }
        return ret += "</Arguments>";
    }

    [[nodiscard]] std::string to_string() const override
    {
        Strings args;
        for (auto& arg : m_arguments) {
            args.push_back(arg->to_string());
        }
        return format("{}({}): {}", name(), join(args, ","), type());
    }

    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override
    {
        return std::optional<Obj> {};
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionCall; }
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] Expressions const& arguments() const { return m_arguments; }
    [[nodiscard]] ExpressionTypes argument_types() const
    {
        ExpressionTypes ret;
        for (auto& arg : arguments()) {
            ret.push_back(arg->type());
        }
        return ret;
    }

private:
    std::string m_name;
    Expressions m_arguments;
};

#if 0
class NativeFunctionCall : public FunctionCall {
public:
    explicit NativeFunctionCall(std::shared_ptr<NativeFunctionDecl> decl, std::shared_ptr<FunctionCall> const& call)
        : FunctionCall(call->function(), call->arguments())
        , m_declaration(move(decl))
    {
    }

    NativeFunctionCall(std::shared_ptr<NativeFunctionDecl> decl, Symbol function, Expressions arguments)
        : FunctionCall(std::move(function), move(arguments))
        , m_declaration(move(decl))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::NativeFunctionCall; }
    [[nodiscard]] std::shared_ptr<NativeFunctionDecl> const& declaration() const { return m_declaration; }

private:
    std::shared_ptr<NativeFunctionDecl> m_declaration;
};

class IntrinsicCall : public FunctionCall {
public:
    explicit IntrinsicCall(std::shared_ptr<FunctionCall> const& call)
        : FunctionCall(call->function(), call->arguments())
    {
    }

    IntrinsicCall(Symbol function, Expressions arguments)
        : FunctionCall(std::move(function), move(arguments))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::CompilerIntrinsic; }
};
#endif

class VariableDeclaration : public Statement {
public:
    VariableDeclaration(Token token, std::shared_ptr<Identifier> identifier, std::shared_ptr<Expression> expr = nullptr, bool constant = false)
        : Statement(std::move(token))
        , m_identifier(move(identifier))
        , m_const(constant)
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

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::VariableDeclaration; }
    [[nodiscard]] std::shared_ptr<Identifier> const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return m_identifier->name(); }
    [[nodiscard]] std::shared_ptr<ExpressionType> const& type() const { return m_identifier->type(); }
    [[nodiscard]] bool is_typed() const { return type() != nullptr; }
    [[nodiscard]] bool is_const() const { return m_const; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }

private:
    std::shared_ptr<Identifier> m_identifier;
    bool m_const { false };
    std::shared_ptr<Expression> m_expression;
};

class Return : public Statement {
public:
    explicit Return(Token token, std::shared_ptr<Expression> expression)
        : Statement(std::move(token))
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

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Return; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }

private:
    std::shared_ptr<Expression> m_expression;
};

class Break : public Statement {
public:
    explicit Break(Token token)
        : Statement(std::move(token))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Break; }
    [[nodiscard]] std::string to_string() const override { return "break"; }
};

class Continue : public Statement {
public:
    explicit Continue(Token token)
        : Statement(std::move(token))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Continue; }
    [[nodiscard]] std::string to_string() const override { return "continue"; }
};

class Branch : public Statement {
public:
    Branch(Token token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> statement)
        : Statement(std::move(token))
        , m_condition(move(condition))
        , m_statement(move(statement))
    {
    }

    Branch(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> statement)
        : Statement(node->token())
        , m_condition(move(condition))
        , m_statement(move(statement))
    {
    }

    explicit Branch(Token token, std::shared_ptr<Statement> statement)
        : Statement(std::move(token))
        , m_statement(move(statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Branch; }
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

    [[nodiscard]] std::shared_ptr<Expression> const& condition() const { return m_condition; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }

private:
    std::shared_ptr<Expression> m_condition { nullptr };
    std::shared_ptr<Statement> m_statement;
};

typedef std::vector<std::shared_ptr<Branch>> Branches;

class IfStatement : public Statement {
public:
    IfStatement(Token token, Branches branches)
        : Statement(std::move(token))
        , m_branches(move(branches))
    {
    }

    IfStatement(Token token, std::shared_ptr<Expression> condition,
        std::shared_ptr<Statement> if_stmt,
        Branches branches,
        std::shared_ptr<Statement> else_stmt)
        : Statement(std::move(token))
        , m_branches(move(branches))
    {
        m_branches.insert(m_branches.begin(), std::make_shared<Branch>(if_stmt->token(), move(condition), move(if_stmt)));
        if (else_stmt) {
            m_branches.push_back(std::make_shared<Branch>(else_stmt->token(), move(else_stmt)));
        }
    }

    IfStatement(Token token, std::shared_ptr<Expression> condition,
        std::shared_ptr<Statement> if_stmt,
        std::shared_ptr<Statement> else_stmt)
        : IfStatement(std::move(token), move(condition), move(if_stmt), Branches {}, move(else_stmt))
    {
    }

    IfStatement(Token token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> if_stmt)
        : IfStatement(std::move(token), move(condition), move(if_stmt), nullptr)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::IfStatement; }
    [[nodiscard]] std::shared_ptr<Statement> const& else_stmt() const { return m_else; }
    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto& branch : m_branches) {
            ret.push_back(branch);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        std::string ret;
        bool first;
        for (auto& branch : m_branches) {
            if (first)
                ret = branch->to_string();
            else
                ret += "el" + branch->to_string();
            first = false;
        }
        if (m_else)
            ret += "else\n" + m_else->to_string();
        return ret;
    }

    [[nodiscard]] Branches const& branches() const { return m_branches; }

private:
    Branches m_branches {};
    std::shared_ptr<Statement> m_else {};
};

class WhileStatement : public Statement {
public:
    WhileStatement(Token token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> stmt)
        : Statement(std::move(token))
        , m_condition(move(condition))
        , m_stmt(move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::WhileStatement; }
    [[nodiscard]] Nodes children() const override { return { m_condition, m_stmt }; }
    [[nodiscard]] std::shared_ptr<Expression> const& condition() const { return m_condition; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_stmt; }

    [[nodiscard]] std::string to_string() const override
    {
        return format("while ({})\n{}", m_condition->to_string());
    }

private:
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Statement> m_stmt;
};

class ForStatement : public Statement {
public:
    ForStatement(Token token, std::string variable, std::shared_ptr<Expression> range, std::shared_ptr<Statement> stmt)
        : Statement(std::move(token))
        , m_variable(move(variable))
        , m_range(move(range))
        , m_stmt(move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ForStatement; }
    [[nodiscard]] std::string attributes() const override { return format(R"(variable="{}")", m_variable); }
    [[nodiscard]] Nodes children() const override { return { m_range, m_stmt }; }
    [[nodiscard]] std::string const& variable() const { return m_variable; }
    [[nodiscard]] std::shared_ptr<Expression> const& range() const { return m_range; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_stmt; }

    [[nodiscard]] std::string to_string() const override
    {
        return format("for ({} in {})\n{}", m_variable, m_range->to_string(), m_stmt->to_string());
    }

private:
    std::string m_variable {};
    std::shared_ptr<Expression> m_range;
    std::shared_ptr<Statement> m_stmt;
};

class CaseStatement : public Branch {
public:
    explicit CaseStatement(Token token, std::shared_ptr<Expression> const& case_expression, std::shared_ptr<Statement> const& stmt)
        : Branch(std::move(token), case_expression, stmt)
    {
    }

    explicit CaseStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Expression> const& case_expression, std::shared_ptr<Statement> const& stmt)
        : Branch(node, case_expression, stmt)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::CaseStatement; }
};

typedef std::vector<std::shared_ptr<CaseStatement>> CaseStatements;

class DefaultCase : public Branch {
public:
    DefaultCase(Token token, std::shared_ptr<Statement> const& stmt)
        : Branch(std::move(token), stmt)
    {
    }

    DefaultCase(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Statement> const& stmt)
        : Branch(node, nullptr, stmt)
    {
    }

    DefaultCase(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Expression> const&, std::shared_ptr<Statement> const& stmt)
        : Branch(node, nullptr, stmt)
    {
    }

    //    DefaultCase(std::shared_ptr<Expression> const&, std::shared_ptr<Statement> const& stmt)
    //        : Branch(stmt)
    //    {
    //    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::DefaultCase; }
};

class SwitchStatement : public Statement {
public:
    explicit SwitchStatement(Token token, std::shared_ptr<Expression> switch_expression,
        CaseStatements case_statements, std::shared_ptr<DefaultCase> default_case)
        : Statement(std::move(token))
        , m_switch_expression(move(switch_expression))
        , m_cases(move(case_statements))
        , m_default(move(default_case))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::SwitchStatement; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_switch_expression; }
    [[nodiscard]] CaseStatements const& cases() const { return m_cases; }
    [[nodiscard]] std::shared_ptr<DefaultCase> const& default_case() const { return m_default; }

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
    std::shared_ptr<Expression> m_switch_expression;
    CaseStatements m_cases {};
    std::shared_ptr<DefaultCase> m_default {};
};

template<class T, class... Args>
std::shared_ptr<T> make_node(Args&&... args)
{
    auto ret = std::make_shared<T>(std::forward<Args>(args)...);
    debug(parser, "{}: {}", SyntaxNodeType_name(ret->node_type()), ret->to_string());
    return ret;
}

template<>
struct Converter<SyntaxNode*> {
    static std::string to_string(SyntaxNode* const val)
    {
        return val->to_string();
    }

    static double to_double(SyntaxNode* const)
    {
        return NAN;
    }

    static unsigned long to_long(SyntaxNode* const)
    {
        return 0;
    }
};

ErrorOr<std::shared_ptr<SyntaxNode>> to_literal(std::shared_ptr<Expression> const& expr);

}
