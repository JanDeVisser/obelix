/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/MacOSX.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>
#include <oblasm/Image.h>
#include <oblasm/Instruction.h>

namespace Obelix {

extern_logging_category(parser);

struct Assembly {
    std::string code;
    std::string text;

    void add_instruction(std::string const& mnemonic, std::string const& arguments)
    {
        code = format("{}\t{}\t{}\n", code, mnemonic, arguments);
    }

    void add_instruction(std::string const& mnemonic)
    {
        code = format("{}\t{}\n", code, mnemonic);
    }

    void add_label(std::string const& label)
    {
        code = format("{}{}:\n", code, label);
    }

    void add_string(std::string const& label, std::string const& str)
    {
        text = format("{}{}:\n\t.asciz\t\"{}\"\n", text, label, str);
    }

    void syscall(int id)
    {
        add_instruction("mov", format("x16, #{}", id));
        add_instruction("svc", "0");
    }
};

class MacOSXContext : public Context<Obj> {
public:
    MacOSXContext(MacOSXContext& parent)
        : Context<Obj>(parent)
        , m_assembly(parent.assembly())
    {
        auto offset_maybe = get("#offset");
        assert(offset_maybe.has_value());
        declare("#offset", get("#offset").value());
    }

    explicit MacOSXContext(MacOSXContext* parent)
        : Context<Obj>(parent)
        , m_assembly(parent->assembly())
    {
        auto offset_maybe = get("#offset");
        assert(offset_maybe.has_value());
        declare("#offset", get("#offset").value());
    }

    explicit MacOSXContext(Assembly& assembly)
        : Context()
        , m_assembly(assembly)
    {
        declare("#offset", make_obj<Integer>(0));
    }

    [[nodiscard]] Assembly& assembly() const { return m_assembly; }
    [[nodiscard]] int current_register() const { return m_current_reg; }
    void next_register() { m_current_reg++; }
    void set_base_register(int reg) { m_current_reg = reg; }

private:
    Assembly& m_assembly;
    int m_current_reg;
};

template<typename T = long>
void push(MacOSXContext& ctx, char const* reg)
{
    ctx.assembly().add_instruction("str", format("{},[sp],-16", reg));
}

template<>
void push<uint8_t>(MacOSXContext& ctx, char const* reg)
{
    ctx.assembly().add_instruction("strb", format("{},[sp],-16", reg));
}

template<typename T = long>
void pop(MacOSXContext& ctx, char const* reg)
{
    ctx.assembly().add_instruction("ldr", format("{},[sp,16]!", reg));
}

template<>
void pop<uint8_t>(MacOSXContext& ctx, char const* reg)
{
    ctx.assembly().add_instruction("ldrb", format("{},[sp,16]!", reg));
}

void push_imm(MacOSXContext& ctx, long value)
{
    ctx.assembly().add_instruction("mov", format("x8,{}", value));
    push(ctx, "x8");
}

void push_imm(MacOSXContext& ctx, uint8_t value)
{
    ctx.assembly().add_instruction("movb", format("w8,{}", value));
    push<uint8_t>(ctx, "w8");
}

template<typename T = long>
ErrorOr<void> push_var(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    ctx.assembly().add_instruction("ldr", format("x8,[fp,{}]", static_cast<T>(idx->to_long().value())));
    push<T>(ctx, "x8");
    return {};
}

template<>
ErrorOr<void> push_var<uint8_t>(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    ctx.assembly().add_instruction("movb", format("w8,[fp,{}]", static_cast<uint8_t>(idx->to_long().value())));
    push<uint8_t>(ctx, "w8");
    return {};
}

template<typename T = long>
ErrorOr<void> pop_var(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    pop<T>(ctx, "x8");
    ctx.assembly().add_instruction("str", format("x8,[fp,{}]", idx));
    return {};
}

template<>
ErrorOr<void> pop_var<uint8_t>(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    pop<uint8_t>(ctx, "w8");
    ctx.assembly().add_instruction("strb", format("w8,[fp,{}]", idx));
    return {};
}

template<typename ExpressionType>
ErrorOr<void> push_expression_return_value(MacOSXContext& ctx, ExpressionType const& expr)
{
    switch (expr.type()) {
    case ObelixType::TypeInt:
    case ObelixType::TypeUnsigned:
        push(ctx, "x8");
        break;
    case ObelixType::TypeBoolean:
    case ObelixType::TypeByte:
    case ObelixType::TypeChar:
        push<uint8_t>(ctx, "w8");
        break;
    default:
        Error { ErrorCode::SyntaxError, format("Unexpected return type {} for expression {}", expr.type(), expr.to_string(0)) };
    }
    return {};
}

ErrorOr<void> bool_unary_expression(MacOSXContext& ctx, UnaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::ExclamationPoint:
        ctx.assembly().add_instruction("eorb", "w8,w8,#0x01"); // a is 0b00000001 (a was true) or 0b00000000 (a was false)
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> bool_bool_binary_expression(MacOSXContext& ctx, BinaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::LogicalAnd:
        ctx.assembly().add_instruction("and", "x8,x8,x9");
        break;
    case TokenCode::LogicalOr:
        ctx.assembly().add_instruction("orr", "x8,x8,x9");
        break;
    case TokenCode::Hat:
        ctx.assembly().add_instruction("xor", "x8,x8,x9");
        break;
    case TokenCode::Equals: {
        ctx.assembly().add_instruction("eor", "x8,x8,x9");    // a is 0b00000000 (a == b) or 0b00000001 (a != b)
        ctx.assembly().add_instruction("eor", "x8,x8,#0x01"); // a is 0b00000001 (a == b) or 0b00000000 (a != b)
        break;
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> int_unary_expression(MacOSXContext& ctx, UnaryExpression const& expr)
{
    if (expr.op().code() == TokenCode::Plus)
        return {};
    switch (expr.op().code()) {
    case TokenCode::Minus: {
        if (expr.operand()->type() == ObelixType::TypeUnsigned)
            return Error { ErrorCode::SyntaxError, "Cannot negate unsigned numbers" };

        // Perform two's complement. Negate and add 1:
        ctx.assembly().add_instruction("mvn", "x8,x8");
        ctx.assembly().add_instruction("add", "x8,#0x01");
        break;
    }
    case TokenCode::Tilde:
        ctx.assembly().add_instruction("mvn", "x8,x8");
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> int_int_binary_expression(MacOSXContext& ctx, BinaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::Plus:
        ctx.assembly().add_instruction("add", "x8,x8,x9");
        break;
    case TokenCode::Minus:
        ctx.assembly().add_instruction("sub", "x8,x8,x9");
        break;
    case TokenCode::EqualsTo: {
        ctx.assembly().add_instruction("cmp", "x8,x9");
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bne", set_false);
        ctx.assembly().add_instruction("mov", "x8,#0x01");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("mov", "x8,#0x00");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::GreaterThan: {
        ctx.assembly().add_instruction("cmp", "x8,x9");
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_false);
        ctx.assembly().add_instruction("mov", "x8,#0x01");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("mov", "x8,#0x00");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::LessThan: {
        ctx.assembly().add_instruction("cmp", "x8,x9");
        auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_true);
        ctx.assembly().add_instruction("mov", "x8,#0x00");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_true);
        ctx.assembly().add_instruction("mov", "x8,#0x01");
        ctx.assembly().add_label(done);
        return {};
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> byte_unary_expression(MacOSXContext& ctx, UnaryExpression const& expr)
{
    if (expr.op().code() == TokenCode::Plus)
        return {};
    switch (expr.op().code()) {
    case TokenCode::Minus: {
        if (expr.operand()->type() == ObelixType::TypeUnsigned)
            return Error { ErrorCode::SyntaxError, "Cannot negate unsigned numbers" };

        // Perform two's complement. Negate and add 1:
        ctx.assembly().add_instruction("mvnb", "w8,w8");
        ctx.assembly().add_instruction("addb", "w8,#0x01");
        break;
    }
    case TokenCode::Tilde:
        ctx.assembly().add_instruction("mvnb", "w8,w8");
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return push_expression_return_value(ctx, expr);
}

ErrorOr<void> byte_byte_binary_expression(MacOSXContext& ctx, BinaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::Plus:
        ctx.assembly().add_instruction("andb", "w8,w8,w9");
        break;
    case TokenCode::Minus:
        ctx.assembly().add_instruction("subb", "w8,w8,w9");
        break;
    case TokenCode::EqualsTo: {
        ctx.assembly().add_instruction("cmpb", "w8,w9");
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bne", set_false);
        ctx.assembly().add_instruction("movb", "w8,#0x01");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("movb", "w8,#0x00");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::GreaterThan: {
        ctx.assembly().add_instruction("cmpb", "w8,w9");
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_false);
        ctx.assembly().add_instruction("movb", "w8,#0x01");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("movb", "w8,#0x00");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::LessThan: {
        ctx.assembly().add_instruction("cmpb", "w8,w9");
        auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_true);
        ctx.assembly().add_instruction("movb", "w8,#0x00");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_true);
        ctx.assembly().add_instruction("movb", "w8,#0x01");
        ctx.assembly().add_label(done);
        return {};
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOrNode output_macosx_processor(std::shared_ptr<SyntaxNode> const& tree, MacOSXContext& ctx)
{
    switch (tree->node_type()) {
    case SyntaxNodeType::FunctionDecl: {
        auto func_decl = std::dynamic_pointer_cast<FunctionDecl>(tree);

        // Set fp offsets of function parameters:
        auto ix = 48;
        for (auto& parameter : func_decl->parameters()) {
            ctx.declare(parameter.name(), make_obj<Integer>(ix));
            ix += 16;
        }

        ctx.assembly().add_label(format("func_{}", func_decl->name()));

        // Push the return value:
        push(ctx, "lr");

        // Set fp to the current sp. Now a return is setting sp back to fp,
        // and popping lr followed by ret.
        ctx.assembly().add_instruction("mov", "fp,sp");
        return tree;
    }

        /*
         *  +------------------- +
         *  | Caller function fp |
         *  +--------------------+
         *  |     argument n     |
         *  +--------------------+  <---- Temp fp
         *  |    argument n-1    |
         *  +------------------- +
         *  |       ....         |
         *  +--------------------+
         *  |     argument 1     |
         *  +--------------------+   <- fp[48]
         *  |       Temp fp      |
         *  +--------------------+   <- fp[32]
         *  |  return addr (lr)  |
         *  +--------------------+
         *  |    local var #1    |
         *  +--------------------+   <---- Called function fp
         *  |       ....         |
         *  +--------------------+
         */

    case SyntaxNodeType::FunctionCall: {
        auto call = std::dynamic_pointer_cast<FunctionCall>(tree);

        // Push fp, and set temp fp to sp:
        push(ctx, "fp");
        ctx.assembly().add_instruction("mov", "fp,sp");

        for (auto it = call->arguments().rbegin(); it != call->arguments().rend(); it++) {
            auto argument = *it;
            ctx.set_base_register(10);
            TRY_RETURN(output_macosx_processor(argument, ctx));
            push(ctx, "x8");
        }

        // Push temp fp.
        push(ctx, "fp");

        // Call function:
        ctx.assembly().add_instruction("bl", format("func_{}", call->name()));

        // Pop temp fp:
        pop(ctx, "fp");

        // Load sp with temp fp. This will discard all function arguments
        ctx.assembly().add_instruction("mov", "sp,fp");

        // Pop caller fp:
        pop(ctx, "fp");

        // Move return value from x0 to x8:
        ctx.assembly().add_instruction("mov", "x8,x0");
        return tree;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        auto rhs = TRY_AND_CAST(Expression, output_macosx_processor(expr->rhs(), ctx));
        if (ctx.current_register() >= 19)
            return Error { ErrorCode::InternalError, "FIXME: Maximum expression depth reached" };
        auto reg = format("x{}", ctx.current_register());
        ctx.next_register();
        ctx.assembly().add_instruction("mov", format("{},x8", reg));
        auto lhs = TRY_AND_CAST(Expression, output_macosx_processor(expr->lhs(), ctx));
        ctx.assembly().add_instruction("mov", format("x9,{}", reg));

        if (expr->lhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->lhs()->to_string(0) };
        }
        if (expr->rhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->rhs()->to_string(0) };
        }

        if ((expr->lhs()->type() == ObelixType::TypeInt && expr->lhs()->type() == ObelixType::TypeInt) || (expr->lhs()->type() == ObelixType::TypeUnsigned && expr->lhs()->type() == ObelixType::TypeUnsigned)) {
            TRY_RETURN(int_int_binary_expression(ctx, *expr));
        }
        if ((expr->lhs()->type() == ObelixType::TypeByte && expr->lhs()->type() == ObelixType::TypeByte) || (expr->lhs()->type() == ObelixType::TypeChar && expr->lhs()->type() == ObelixType::TypeChar)) {
            TRY_RETURN(byte_byte_binary_expression(ctx, *expr));
        }
        if (expr->lhs()->type() == ObelixType::TypeBoolean && expr->lhs()->type() == ObelixType::TypeBoolean) {
            TRY_RETURN(bool_bool_binary_expression(ctx, *expr));
        }
        return tree;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, output_macosx_processor(expr->operand(), ctx));

        if (operand->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, operand->to_string(0) };
        }

        if (operand->type() == ObelixType::TypeInt || operand->type() == ObelixType::TypeUnsigned) {
            TRY_RETURN(int_unary_expression(ctx, *expr));
        }
        if (operand->type() == ObelixType::TypeByte || operand->type() == ObelixType::TypeChar) {
            TRY_RETURN(byte_unary_expression(ctx, *expr));
        }
        if (operand->type() == ObelixType::TypeBoolean) {
            TRY_RETURN(bool_unary_expression(ctx, *expr));
        }
        return tree;
    }

    case SyntaxNodeType::Literal: {
        auto literal = std::dynamic_pointer_cast<Literal>(tree);
        auto val_maybe = TRY(literal->to_object());
        assert(val_maybe.has_value());
        auto val = val_maybe.value();
        switch (val.type()) {
        case ObelixType::TypeInt:
        case ObelixType::TypeUnsigned: {
            ctx.assembly().add_instruction("mov", format("x8,#{}", val->to_long().value()));
            break;
        }
        case ObelixType::TypeChar:
        case ObelixType::TypeByte:
        case ObelixType::TypeBoolean: {
            ctx.assembly().add_instruction("mov", format("w8,#{}", static_cast<uint8_t>(val->to_long().value())));
            break;
        }
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit literals of type {} yet", ObelixType_name(val.type())));
        }
        return tree;
    }

    case SyntaxNodeType::Identifier: {
        auto identifier = std::dynamic_pointer_cast<Identifier>(tree);
        auto idx_maybe = ctx.get(identifier->name());
        if (!idx_maybe.has_value())
            return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", identifier->name()) };
        auto idx = idx_maybe.value();

        switch (identifier->type()) {
        case ObelixType::TypeInt:
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("ldr", format("x8,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeByte:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("ldrb", format("w8,[fp,{}]", idx->to_long().value()));
            break;
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot push values of variables of type {} yet", identifier->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::Assignment: {
        auto assignment = std::dynamic_pointer_cast<Assignment>(tree);

        auto idx_maybe = ctx.get(assignment->name());
        if (!idx_maybe.has_value())
            return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", assignment->name()) };
        auto idx = idx_maybe.value();

        // Evaluate the expression into w8:
        ctx.set_base_register(10);
        TRY_RETURN(output_macosx_processor(assignment->expression(), ctx));

        switch (assignment->type()) {
        case ObelixType::TypeInt:
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("str", format("x8,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("strb", format("x8,[fp,{}]", idx->to_long().value()));
            break;
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot emit assignments of type {} yet", assignment->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        ctx.set_base_register(10);
        auto offset = (signed char)ctx.get("#offset").value()->to_long().value();
        ctx.set("#offset", make_obj<Integer>(offset + 16)); // FIXME Use type size
        ctx.declare(var_decl->variable().identifier(), make_obj<Integer>(offset));
        if (var_decl->expression() != nullptr) {
            TRY_RETURN(output_macosx_processor(var_decl->expression(), ctx));
        } else {
            switch (var_decl->expression()->type()) {
            case ObelixType::TypeInt:
            case ObelixType::TypeUnsigned:
            case ObelixType::TypeByte:
            case ObelixType::TypeChar:
            case ObelixType::TypeBoolean:
                ctx.assembly().add_instruction("mov", "x8,0");
                break;
            default:
                return Error { ErrorCode::NotYetImplemented, format("Cannot initialize variables of type {} yet", var_decl->expression()->type()) };
            }
        }
        ctx.assembly().add_instruction("str", "x8,[sp],-16");
        return tree;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        ctx.set_base_register(10);
        TRY_RETURN(output_macosx_processor(expr_stmt->expression(), ctx));
    }

    case SyntaxNodeType::Return: {
        auto ret = std::dynamic_pointer_cast<Return>(tree);
        ctx.set_base_register(10);
        TRY_RETURN(output_macosx_processor(ret->expression(), ctx));

        // Move return value to x0 register:
        ctx.assembly().add_instruction("mov", "x0,x8");

        // Load sp with the current value of bp. This will discard all local variables
        ctx.assembly().add_instruction("mov", "sp,fp");

        // Pop return address into lr:
        pop(ctx, "lr");

        // Return:
        ctx.assembly().add_instruction("ret");
        return tree;
    }

    case SyntaxNodeType::Label: {
        auto label = std::dynamic_pointer_cast<Obelix::Label>(tree);
        ctx.assembly().add_label(format("lbl_{}", label->label_id()));
        return tree;
    }

    case SyntaxNodeType::Goto: {
        auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
        ctx.assembly().add_instruction("b", format("lbl_{}", goto_stmt->label_id()));
        return tree;
    }

    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);

        auto end_label = Obelix::Label::reserve_id();
        auto count = if_stmt->branches().size() - 1;
        for (auto& branch : if_stmt->branches()) {
            auto else_label = (count) ? Obelix::Label::reserve_id() : end_label;
            ctx.set_base_register(10);
            auto cond = TRY_AND_CAST(Expression, output_macosx_processor(branch->condition(), ctx));
            ctx.assembly().add_instruction("cmp", "x8,0x00");
            ctx.assembly().add_instruction("bne", format("lbl_{}", else_label));
            auto stmt = TRY_AND_CAST(Statement, output_macosx_processor(branch->statement(), ctx));
            if (count) {
                ctx.assembly().add_instruction("b", format("lbl_{}", end_label));
                ctx.assembly().add_label(format("lbl_{}", else_label));
            }
            count--;
        }
        ctx.assembly().add_label(format("lbl_{}", end_label));
        return tree;
    }

    default:
        return process_tree(tree, ctx, output_macosx_processor);
    }
}

ErrorOrNode output_macosx(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name)
{
    Assembly assembly;
    MacOSXContext root(assembly);

    assembly.code = ".global _start\n" // Provide program starting address to linker
                    ".align 2\n\n";
    assembly.add_label("_start");
    assembly.add_instruction("mov", "fp,sp");
    push(root, "fp");
    assembly.add_instruction("mov", "fp,sp");
    push(root, "fp");
    assembly.add_instruction("bl", "func_main");
    assembly.syscall(1); // Service command code 1 terminates this program

    auto ret = output_macosx_processor(tree, root);

    if (ret.is_error()) {
        return ret;
    }

    printf("%s\n%s\n", assembly.code.c_str(), assembly.text.c_str());

    std::string bare_file_name = file_name;
    if (auto dot = file_name.find_last_of('.'); dot != std::string::npos) {
        bare_file_name = file_name.substr(0, dot);
    }
    auto assembly_file = format("{}.s", bare_file_name);
    FILE* f = fopen(assembly_file.c_str(), "w");
    if (f) {
        fprintf(f, "%s\n%s\n", assembly.code.c_str(), assembly.text.c_str());
        fclose(f);

        if (!system(format("as -o {}.o {}.s", bare_file_name, bare_file_name).c_str())) {
            system(format("ld -o {} {}.o -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -e _start -arch arm64 -L.", bare_file_name, bare_file_name).c_str());
        }
    }
    return ret;
}

}
