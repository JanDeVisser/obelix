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
#include <string>

namespace Obelix {

extern_logging_category(parser);

#define ENUMERATE_SYNTAXNODETYPES(S)  \
    S(SyntaxNode)                     \
    S(Statement)                      \
    S(Block)                          \
    S(FunctionBlock)                  \
    S(Compilation)                    \
    S(Module)                         \
    S(Expression)                     \
    S(Literal)                        \
    S(Identifier)                     \
    S(This)                           \
    S(BinaryExpression)               \
    S(UnaryExpression)                \
    S(Assignment)                     \
    S(CompilerIntrinsic)              \
    S(FunctionCall)                   \
    S(NativeFunctionCall)             \
    S(Import)                         \
    S(Pass)                           \
    S(Label)                          \
    S(Goto)                           \
    S(FunctionDecl)                   \
    S(NativeFunctionDecl)             \
    S(IntrinsicDecl)                  \
    S(FunctionDef)                    \
    S(ExpressionStatement)            \
    S(VariableDeclaration)            \
    S(Return)                         \
    S(Break)                          \
    S(Continue)                       \
    S(Branch)                         \
    S(IfStatement)                    \
    S(WhileStatement)                 \
    S(ForStatement)                   \
    S(CaseStatement)                  \
    S(DefaultCase)                    \
    S(SwitchStatement)                \
    S(FunctionParameter)              \
    S(MaterializedFunctionDecl)       \
    S(MaterializedFunctionDef)        \
    S(MaterializedVariableDecl)       \
    S(MaterializedNativeFunctionDecl) \
    S(MaterializedIntrinsicDecl)      \
    S(StatementExecutionResult)

enum class SyntaxNodeType {
#undef __SYNTAXNODETYPE
#define __SYNTAXNODETYPE(type) type,
    ENUMERATE_SYNTAXNODETYPES(__SYNTAXNODETYPE)
#undef __SYNTAXNODETYPE
};

constexpr char const* SyntaxNodeType_name(SyntaxNodeType type)
{
    switch (type) {
#undef __SYNTAXNODETYPE
#define __SYNTAXNODETYPE(type) \
    case SyntaxNodeType::type: \
        return #type;
        ENUMERATE_SYNTAXNODETYPES(__SYNTAXNODETYPE)
#undef __SYNTAXNODETYPE
    }
}

typedef std::vector<std::string> Strings;

class SyntaxNode;
class Module;

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
    SyntaxNode();
    SyntaxNode(SyntaxNode const& ancestor);
    virtual ~SyntaxNode() = default;

    [[nodiscard]] virtual std::string to_string(int) const = 0;
    [[nodiscard]] std::string to_string() const { return to_string(0); }
    [[nodiscard]] virtual SyntaxNodeType node_type() const = 0;

private:
    static int s_current_id;
};

using Nodes = std::vector<std::shared_ptr<SyntaxNode>>;

class Statement : public SyntaxNode {
public:
    Statement() = default;
};

class Expression : public SyntaxNode {
public:
    Expression()
        : SyntaxNode()
    {
    }

    explicit Expression(ObelixType type)
        : SyntaxNode()
        , m_type(type)
    {
    }

    [[nodiscard]] ObelixType type() const { return m_type; }
    [[nodiscard]] bool is_typed() { return m_type != ObelixType::TypeUnknown; }
    [[nodiscard]] virtual ErrorOr<std::optional<Obj>> to_object() const = 0;

private:
    ObelixType m_type { TypeUnknown };
};

class Import : public Statement {
public:
    explicit Import(std::string name)
        : Statement()
        , m_name(move(name))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Import; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}import {}", pad(indent), m_name);
    }

private:
    std::string m_name;
};

class Pass : public Statement {
public:
    Pass() = default;
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Pass; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{};", pad(indent));
    }
};

class Goto;

class Label : public Statement {
public:
    Label()
        : Statement()
        , m_label_id(reserve_id())
    {
    }

    explicit Label(std::shared_ptr<Goto> const& goto_stmt);

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Label; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}:", m_label_id);
    }

    [[nodiscard]] int label_id() const { return m_label_id; }

    static int reserve_id() { return m_current_id++; }

private:
    static int m_current_id;
    int m_label_id;
};

class Goto : public Statement {
public:
    Goto()
        : Statement()
        , m_label_id(Label::reserve_id())
    {
    }

    explicit Goto(std::shared_ptr<Label> const& label)
        : Statement()
        , m_label_id(label->label_id())
    {
    }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}goto {}", pad(indent), m_label_id);
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Goto; }
    [[nodiscard]] int label_id() const { return m_label_id; }

private:
    int m_label_id;
};

typedef std::vector<std::shared_ptr<Statement>> Statements;

class Block : public Statement {
public:
    explicit Block(Statements statements)
        : Statement()
        , m_statements(move(statements))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Block; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{}{\n", pad(indent));
        for (auto& statement : m_statements) {
            ret += statement->to_string(indent + 2);
            ret += "\n";
        }
        ret += format("{}}", pad(indent));
        return ret;
    }
    [[nodiscard]] Statements const& statements() const { return m_statements; }

protected:
    Statements m_statements {};
};

class FunctionBlock : public Block {
public:
    explicit FunctionBlock(Statements statements)
        : Block(move(statements))
    {
    }

    explicit FunctionBlock(std::shared_ptr<Statement> statement)
        : Block(Statements { move(statement) })
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionBlock; }
};

class Module : public Block {
public:
    Module(Statements const& statements, std::string name)
        : Block(statements)
        , m_name(move(name))
    {
    }

    Module(std::shared_ptr<Module> const& original, std::vector<std::shared_ptr<Statement>> const& statements)
        : Block(statements)
        , m_name(original->name())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Module; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{}module {}\n", pad(indent), m_name);
        ret += Block::to_string(indent);
        return ret;
    }

    [[nodiscard]] const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

using Modules = std::vector<std::shared_ptr<Module>>;

class Compilation : public Module {
public:
    Compilation(std::shared_ptr<Module> root, Modules modules)
        : Module(root->statements(), "")
        , m_modules(move(modules))
    {
    }

    Compilation(Statements statements, Modules modules)
        : Module(statements, "")
        , m_modules(move(modules))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Compilation; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        std::string ret = Module::to_string(indent);
        ret += "\n";
        for (auto& module : m_modules) {
            ret += module->to_string(indent);
            ret += "\n";
        }
        return ret;
    }
    [[nodiscard]] Modules const& modules() const { return m_modules; }

private:
    Modules m_modules;
};

class FunctionDecl : public SyntaxNode {
public:
    explicit FunctionDecl(Symbol identifier, Symbols parameters)
        : SyntaxNode()
        , m_identifier(std::move(identifier))
        , m_parameters(move(parameters))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionDecl; }
    [[nodiscard]] Symbol const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return identifier().identifier(); }
    [[nodiscard]] ObelixType type() const { return identifier().type(); }
    [[nodiscard]] Symbols const& parameters() const { return m_parameters; }
    [[nodiscard]] ObelixTypes parameter_types() const
    {
        ObelixTypes ret;
        for (auto& parameter : m_parameters) {
            ret.push_back(parameter.type());
        }
        return ret;
    }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}func {}({}) : {}",
            pad(indent), identifier().identifier(), parameters_to_string(), type());
    }

    [[nodiscard]] std::string label() const
    {
        std::string params;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                params += ",";
            first = false;
            params += ObelixType_name(param.type());
        }
        return format("{}({})", name(), params);
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
            ret += param.identifier();
        }
        return ret;
    }

    Symbol m_identifier;
    Symbols m_parameters;
};

class NativeFunctionDecl : public FunctionDecl {
public:
    explicit NativeFunctionDecl(std::shared_ptr<FunctionDecl> const& func_decl, std::string native_function)
        : FunctionDecl(func_decl->identifier(), func_decl->parameters())
        , m_native_function_name(move(native_function))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::NativeFunctionDecl; }

    [[nodiscard]] std::string const& native_function_name() const { return m_native_function_name; }

private:
    std::string m_native_function_name;
};

class IntrinsicDecl : public FunctionDecl {
public:
    explicit IntrinsicDecl(Symbol identifier, Symbols parameters)
        : FunctionDecl(std::move(identifier), move(parameters))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::IntrinsicDecl; }
};

class FunctionDef : public Statement {
public:
    explicit FunctionDef(std::shared_ptr<FunctionDecl> func_decl, std::shared_ptr<Statement> statement = nullptr)
        : Statement()
        , m_function_decl(move(func_decl))
        , m_statement(move(statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionDef; }
    [[nodiscard]] std::shared_ptr<FunctionDecl> const& declaration() const { return m_function_decl; }
    [[nodiscard]] Symbol const& identifier() const { return m_function_decl->identifier(); }
    [[nodiscard]] std::string const& name() const { return identifier().identifier(); }
    [[nodiscard]] ObelixType type() const { return identifier().type(); }
    [[nodiscard]] Symbols const& parameters() const { return m_function_decl->parameters(); }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = m_function_decl->to_string(indent);
        if (m_statement) {
            ret += '\n';
            ret += m_statement->to_string(indent + 2);
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
        : Statement()
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ExpressionStatement; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}{}", pad(indent), m_expression->to_string(indent));
    }

    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }

private:
    std::shared_ptr<Expression> m_expression;
};

class Identifier : public Expression {
public:
    explicit Identifier(std::string name)
        : Expression()
        , m_identifier(Symbol { move(name) })
    {
    }

    explicit Identifier(Symbol identifier)
        : Expression(identifier.type())
        , m_identifier(std::move(identifier))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Identifier; }
    [[nodiscard]] Symbol const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return m_identifier.identifier(); }
    [[nodiscard]] std::string to_string(int indent) const override { return format("{} : {}", name(), type()); }
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return std::optional<Obj> {}; }

private:
    Symbol m_identifier;
};

class Literal : public Expression {
public:
    explicit Literal(Token const& token)
        : Expression(token.to_object()->type()) // Ick
        , m_literal(token.to_object())
    {
    }

    explicit Literal(Obj value)
        : Expression(value.type())
        , m_literal(std::move(value))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Literal; }
    [[nodiscard]] std::string to_string(int indent) const override { return format("{} : {}", m_literal->to_string(), type()); }
    [[nodiscard]] Obj const& literal() const { return m_literal; }
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return literal(); }

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

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return "this";
    }

    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return std::optional<Obj> {}; }
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(std::shared_ptr<Expression> lhs, Token op, std::shared_ptr<Expression> rhs, ObelixType type = ObelixType::TypeUnknown)
        : Expression(type)
        , m_lhs(std::move(lhs))
        , m_operator(std::move(op))
        , m_rhs(std::move(rhs))
    {
    }

    BinaryExpression(std::shared_ptr<BinaryExpression> const& expr, ObelixType type)
        : Expression(type)
        , m_lhs(expr->lhs())
        , m_operator(expr->op())
        , m_rhs(expr->rhs())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BinaryExpression; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("({}) {} ({}) : {}", m_lhs->to_string(indent), m_operator.value(), m_rhs->to_string(indent), type());
    }
    ErrorOr<std::optional<Obj>> to_object() const override;

    [[nodiscard]] std::shared_ptr<Expression> const& lhs() const { return m_lhs; }
    [[nodiscard]] std::shared_ptr<Expression> const& rhs() const { return m_rhs; }
    [[nodiscard]] Token op() const { return m_operator; }

private:
    std::shared_ptr<Expression> m_lhs;
    Token m_operator;
    std::shared_ptr<Expression> m_rhs;
};

class Assignment : public Expression {
public:
    Assignment(std::shared_ptr<Identifier> identifier, std::shared_ptr<Expression> expression)
        : Expression(identifier->type())
        , m_identifier(identifier)
        , m_expression(expression)
    {
        assert(m_identifier->is_typed());
        assert(m_expression->is_typed());
        assert(m_identifier->type() == m_expression->type());
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Assignment; }
    [[nodiscard]] std::shared_ptr<Identifier> const& identifier() const { return m_identifier; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    ErrorOr<std::optional<Obj>> to_object() const override { return identifier()->to_object(); }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{} = {}", identifier()->to_string(0), expression()->to_string());
        return ret;
    }

private:
    std::shared_ptr<Identifier> m_identifier;
    std::shared_ptr<Expression> m_expression;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(Token op, std::shared_ptr<Expression> operand, ObelixType type = ObelixType::TypeUnknown)
        : Expression(type)
        , m_operator(std::move(op))
        , m_operand(std::move(operand))
    {
    }

    UnaryExpression(std::shared_ptr<UnaryExpression> const& expr, ObelixType type)
        : Expression(type)
        , m_operator(expr->op())
        , m_operand(expr->operand())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::UnaryExpression; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}({}) : {}", m_operator.value(), m_operand->to_string(), type());
    }

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

    [[nodiscard]] Token op() const { return m_operator; }
    [[nodiscard]] std::shared_ptr<Expression> const& operand() const { return m_operand; }

private:
    Token m_operator;
    std::shared_ptr<Expression> m_operand;
};

class FunctionCall : public Expression {
public:
    FunctionCall(std::string function, Expressions arguments)
        : Expression()
        , m_function(Symbol { move(function) })
        , m_arguments(move(arguments))
    {
    }

    explicit FunctionCall(std::string function)
        : Expression()
        , m_function(Symbol { move(function) })
        , m_arguments()
    {
    }

    FunctionCall(Symbol function, Expressions arguments)
        : Expression(function.type())
        , m_function(std::move(function))
        , m_arguments(move(arguments))
    {
    }

    explicit FunctionCall(Symbol function)
        : Expression(function.type())
        , m_function(std::move(function))
        , m_arguments()
    {
    }

    [[nodiscard]] std::string to_string(int) const override
    {
        return format("{}({}) : {}", name(), arguments_to_string(), type());
    }

    ErrorOr<std::optional<Obj>> to_object() const override
    {
        return std::optional<Obj> {};
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionCall; }
    [[nodiscard]] Symbol const& function() const { return m_function; }
    [[nodiscard]] std::string const& name() const { return m_function.identifier(); }
    [[nodiscard]] Expressions const& arguments() const { return m_arguments; }
    [[nodiscard]] ObelixTypes argument_types() const
    {
        ObelixTypes ret;
        for (auto& arg : arguments()) {
            ret.push_back(arg->type());
        }
        return ret;
    }

private:
    [[nodiscard]] std::string arguments_to_string() const
    {
        std::string ret;
        bool first = true;
        for (auto& arg : m_arguments) {
            if (!first)
                ret += ", ";
            first = false;
            ret += arg->to_string();
        }
        return ret;
    }

    Symbol m_function;
    Expressions m_arguments;
};

class NativeFunctionCall : public FunctionCall {
public:
    explicit NativeFunctionCall(std::shared_ptr<NativeFunctionDecl> decl, std::shared_ptr<FunctionCall> const& call)
        : FunctionCall(call->function(), call->arguments())
        , m_declaration(move(decl))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::NativeFunctionCall; }
    [[nodiscard]] std::shared_ptr<NativeFunctionDecl> const& declaration() const { return m_declaration; }

private:
    std::shared_ptr<NativeFunctionDecl> m_declaration;
};

class CompilerIntrinsic : public FunctionCall {
public:
    explicit CompilerIntrinsic(std::shared_ptr<FunctionCall> const& call)
        : FunctionCall(call->function(), call->arguments())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::CompilerIntrinsic; }
};

class VariableDeclaration : public Statement {
public:
    explicit VariableDeclaration(std::string variable, ObelixType type, std::shared_ptr<Expression> expr = nullptr, bool constant = false)
        : Statement()
        , m_variable(move(variable), type)
        , m_const(constant)
        , m_expression(move(expr))
    {
    }

    explicit VariableDeclaration(Symbol identifier, std::shared_ptr<Expression> expr = nullptr, bool constant = false)
        : Statement()
        , m_variable(std::move(identifier))
        , m_const(constant)
        , m_expression(move(expr))
    {
    }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{}{} {} : {}", pad(indent), (m_const) ? "const" : "var", m_variable.identifier(), m_variable.type());
        if (m_expression)
            ret = format("{} = {}", ret, m_expression->to_string());
        return ret;
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::VariableDeclaration; }
    [[nodiscard]] Symbol const& variable() const { return m_variable; }
    [[nodiscard]] std::string const& name() const { return variable().identifier(); }
    [[nodiscard]] ObelixType type() const { return variable().type(); }
    [[nodiscard]] bool is_typed() const { return variable().is_typed(); }
    [[nodiscard]] bool is_const() const { return m_const; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }

private:
    Symbol m_variable;
    bool m_const { false };
    std::shared_ptr<Expression> m_expression;
};

class Return : public Statement {
public:
    explicit Return(std::shared_ptr<Expression> expression)
        : Statement()
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}return {}", pad(indent), m_expression->to_string());
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Return; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }

private:
    std::shared_ptr<Expression> m_expression;
};

class Break : public Statement {
public:
    explicit Break()
        : Statement()
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Break; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}break", pad(indent));
    }
};

class Continue : public Statement {
public:
    explicit Continue()
        : Statement()
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Continue; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}continue", pad(indent));
    }
};

class Branch : public Statement {
public:
    Branch(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> statement)
        : Statement()
        , m_condition(move(condition))
        , m_statement(move(statement))
    {
    }

    explicit Branch(std::shared_ptr<Statement> statement)
        : Statement()
        , m_statement(move(statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Branch; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{}if ", pad(indent));
        if (m_condition)
            ret += m_condition->to_string();
        ret += '\n';
        ret += m_statement->to_string(indent + 2);
        return ret;
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
    explicit IfStatement(Branches branches)
        : Statement()
        , m_branches(move(branches))
    {
    }

    IfStatement(std::shared_ptr<Expression> condition,
        std::shared_ptr<Statement> if_stmt,
        Branches branches,
        std::shared_ptr<Statement> else_stmt)
        : Statement()
        , m_branches(move(branches))
    {
        m_branches.insert(m_branches.begin(), std::make_shared<Branch>(move(condition), move(if_stmt)));
        if (else_stmt) {
            m_branches.push_back(std::make_shared<Branch>(move(else_stmt)));
        }
    }

    IfStatement(std::shared_ptr<Expression> condition,
        std::shared_ptr<Statement> if_stmt,
        std::shared_ptr<Statement> else_stmt)
        : IfStatement(move(condition), move(if_stmt), Branches {}, move(else_stmt))
    {
    }

    IfStatement(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> if_stmt)
        : IfStatement(move(condition), move(if_stmt), nullptr)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::IfStatement; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        std::string ret;
        for (auto& branch : m_branches) {
            ret += branch->to_string(indent);
        }
        return ret;
    }

    [[nodiscard]] Branches const& branches() const { return m_branches; }

private:
    Branches m_branches {};
    std::shared_ptr<Statement> m_else {};
};

class WhileStatement : public Statement {
public:
    WhileStatement(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> stmt)
        : Statement()
        , m_condition(move(condition))
        , m_stmt(move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::WhileStatement; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}while {}\n{}", pad(indent), m_condition->to_string(), m_stmt->to_string(indent + 2));
    }
    [[nodiscard]] std::shared_ptr<Expression> const& condition() const { return m_condition; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_stmt; }

private:
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Statement> m_stmt;
};

class ForStatement : public Statement {
public:
    ForStatement(std::string variable, std::shared_ptr<Expression> range, std::shared_ptr<Statement> stmt)
        : Statement()
        , m_variable(move(variable))
        , m_range(move(range))
        , m_stmt(move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ForStatement; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}for {} in {}\n{}", pad(indent), m_variable, m_range->to_string(), m_stmt->to_string(indent + 2));
    }
    [[nodiscard]] std::string const& variable() const { return m_variable; }
    [[nodiscard]] std::shared_ptr<Expression> const& range() const { return m_range; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_stmt; }

private:
    std::string m_variable {};
    std::shared_ptr<Expression> m_range;
    std::shared_ptr<Statement> m_stmt;
};

class CaseStatement : public Branch {
public:
    explicit CaseStatement(std::shared_ptr<Expression> const& case_expression, std::shared_ptr<Statement> const& stmt)
        : Branch(case_expression, stmt)
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::CaseStatement; }
};

typedef std::vector<std::shared_ptr<CaseStatement>> CaseStatements;

class DefaultCase : public Branch {
public:
    explicit DefaultCase(std::shared_ptr<Statement> const& stmt)
        : Branch(stmt)
    {
    }
    DefaultCase(std::shared_ptr<Expression> const&, std::shared_ptr<Statement> const& stmt)
        : Branch(stmt)
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::DefaultCase; }
};

class SwitchStatement : public Statement {
public:
    explicit SwitchStatement(std::shared_ptr<Expression> switch_expression,
        CaseStatements case_statements, std::shared_ptr<DefaultCase> default_case)
        : Statement()
        , m_switch_expression(move(switch_expression))
        , m_cases(move(case_statements))
        , m_default(move(default_case))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::SwitchStatement; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{}switch ({}) {\n", pad(indent), m_switch_expression->to_string());
        for (auto& case_stmt : m_cases) {
            ret += case_stmt->to_string(indent);
        }
        if (m_default)
            ret += m_default->to_string(indent);
        ret += format("{}}", pad(indent));
        return ret;
    }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_switch_expression; }
    [[nodiscard]] CaseStatements const& cases() const { return m_cases; }
    [[nodiscard]] std::shared_ptr<DefaultCase> const& default_case() const { return m_default; }

private:
    std::shared_ptr<Expression> m_switch_expression;
    CaseStatements m_cases {};
    std::shared_ptr<DefaultCase> m_default {};
};

class FunctionParameter : public SyntaxNode {
public:
    FunctionParameter(Symbol parameter, int offset)
        : SyntaxNode()
        , m_identifier(std::move(parameter))
        , m_offset(offset)
    {
    }

    [[nodiscard]] Symbol const& identifier() const { return m_identifier; }
    [[nodiscard]] int offset() const { return m_offset; }
    [[nodiscard]] std::string const& name() const { return identifier().name(); }
    [[nodiscard]] ObelixType type() const { return identifier().type(); }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionParameter; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}param {} : {} @{}", pad(indent), m_identifier.name(), m_identifier.type(), offset());
    }

private:
    Symbol m_identifier;
    int m_offset;
};

using FunctionParameters = std::vector<std::shared_ptr<FunctionParameter>>;

class MaterializedFunctionDecl : public SyntaxNode {
public:
    explicit MaterializedFunctionDecl(Symbol identifier, FunctionParameters parameters)
        : SyntaxNode()
        , m_identifier(std::move(identifier))
        , m_parameters(move(parameters))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedFunctionDecl; }
    [[nodiscard]] Symbol const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return identifier().identifier(); }
    [[nodiscard]] ObelixType type() const { return identifier().type(); }
    [[nodiscard]] FunctionParameters const& parameters() const { return m_parameters; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}func {}({}) : {}",
            pad(indent), identifier().identifier(), parameters_to_string(), type());
    }

    [[nodiscard]] std::string label() const
    {
        std::string params;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                params += ",";
            first = false;
            params += ObelixType_name(param->type());
        }
        return format("{}({})", name(), params);
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
            ret += param->name();
        }
        return ret;
    }

    Symbol m_identifier;
    FunctionParameters m_parameters;
};

class MaterializedNativeFunctionDecl : public MaterializedFunctionDecl {
public:
    MaterializedNativeFunctionDecl(std::shared_ptr<MaterializedFunctionDecl> const& func_decl, std::string native_function)
        : MaterializedFunctionDecl(func_decl->identifier(), func_decl->parameters())
        , m_native_function_name(move(native_function))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedNativeFunctionDecl; }

    [[nodiscard]] std::string const& native_function_name() const { return m_native_function_name; }

private:
    std::string m_native_function_name;
};

class MaterializedIntrinsicDecl : public MaterializedFunctionDecl {
public:
    explicit MaterializedIntrinsicDecl(std::shared_ptr<MaterializedFunctionDecl> const& func_decl)
        : MaterializedFunctionDecl(func_decl->identifier(), func_decl->parameters())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedIntrinsicDecl; }
};

class MaterializedFunctionDef : public Statement {
public:
    MaterializedFunctionDef(std::shared_ptr<MaterializedFunctionDecl> func_decl, std::shared_ptr<Statement> statement, int stack_depth)
        : Statement()
        , m_function_decl(move(func_decl))
        , m_statement(move(statement))
        , m_stack_depth(stack_depth)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedFunctionDef; }
    [[nodiscard]] std::shared_ptr<MaterializedFunctionDecl> const& declaration() const { return m_function_decl; }
    [[nodiscard]] Symbol const& identifier() const { return m_function_decl->identifier(); }
    [[nodiscard]] std::string const& name() const { return identifier().identifier(); }
    [[nodiscard]] ObelixType type() const { return identifier().type(); }
    [[nodiscard]] FunctionParameters const& parameters() const { return m_function_decl->parameters(); }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }
    [[nodiscard]] int stack_depth() const { return m_stack_depth; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = m_function_decl->to_string(indent);
        if (m_statement) {
            ret += '\n';
            ret += m_statement->to_string(indent + 2);
        }
        return ret;
    }

protected:
    std::shared_ptr<MaterializedFunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
    int m_stack_depth;
};

class MaterializedVariableDecl : public Statement {
public:
    explicit MaterializedVariableDecl(std::shared_ptr<VariableDeclaration> const& var_decl, int offset)
        : Statement()
        , m_variable(var_decl->variable())
        , m_const(var_decl->is_const())
        , m_expression(var_decl->expression())
        , m_offset(offset)
    {
    }

    explicit MaterializedVariableDecl(Symbol identifier, int offset, std::shared_ptr<Expression> expr = nullptr, bool constant = false)
        : Statement()
        , m_variable(std::move(identifier))
        , m_const(constant)
        , m_expression(move(expr))
        , m_offset(offset)
    {
    }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{}{} {} : {}", pad(indent), (m_const) ? "const" : "var", m_variable.identifier(), m_variable.type());
        if (m_expression)
            ret = format("{} = {}", ret, m_expression->to_string());
        return ret;
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedVariableDecl; }
    [[nodiscard]] Symbol const& variable() const { return m_variable; }
    [[nodiscard]] std::string const& name() const { return variable().identifier(); }
    [[nodiscard]] ObelixType type() const { return variable().type(); }
    [[nodiscard]] bool is_typed() const { return variable().is_typed(); }
    [[nodiscard]] bool is_const() const { return m_const; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }
    [[nodiscard]] int offset() const { return m_offset; }

private:
    Symbol m_variable;
    bool m_const { false };
    std::shared_ptr<Expression> m_expression;
    int m_offset;
};

template<class T, class... Args>
std::shared_ptr<T> make_node(Args&&... args)
{
    auto ret = std::make_shared<T>(std::forward<Args>(args)...);
    debug(parser, "make_node<{}>", SyntaxNodeType_name(ret->node_type()));
    return ret;
}

ErrorOr<std::shared_ptr<SyntaxNode>> to_literal(std::shared_ptr<Expression> const& expr);

}
