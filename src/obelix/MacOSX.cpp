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

    void add_string(int id, std::string const& str)
    {
        text = format("{}str_{}:\n\t.ascii\t\"{}\"\n", text, id, str);
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
    void reset_register() { m_current_reg = 4; }

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
    ctx.assembly().add_instruction("mov", format("x0,{}", value));
    push(ctx, "x0");
}

void push_imm(MacOSXContext& ctx, uint8_t value)
{
    ctx.assembly().add_instruction("movb", format("w0,{}", value));
    push<uint8_t>(ctx, "w0");
}

template<typename T = long>
ErrorOr<void> push_var(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    ctx.assembly().add_instruction("ldr", format("x0,[fp,{}]", static_cast<T>(idx->to_long().value())));
    push<T>(ctx, "x0");
    return {};
}

template<>
ErrorOr<void> push_var<uint8_t>(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    ctx.assembly().add_instruction("movb", format("w0,[fp,{}]", static_cast<uint8_t>(idx->to_long().value())));
    push<uint8_t>(ctx, "w0");
    return {};
}

template<typename T = long>
ErrorOr<void> pop_var(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    pop<T>(ctx, "x0");
    ctx.assembly().add_instruction("str", format("x0,[fp,{}]", idx));
    return {};
}

template<>
ErrorOr<void> pop_var<uint8_t>(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    pop<uint8_t>(ctx, "w0");
    ctx.assembly().add_instruction("strb", format("w0,[fp,{}]", idx));
    return {};
}

ErrorOr<void> bool_unary_expression(MacOSXContext& ctx, UnaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::ExclamationPoint:
        ctx.assembly().add_instruction("eorb", "w0,w0,#0x01"); // a is 0b00000001 (a was true) or 0b00000000 (a was false)
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
        ctx.assembly().add_instruction("and", "x0,x0,x2");
        break;
    case TokenCode::LogicalOr:
        ctx.assembly().add_instruction("orr", "x0,x0,x2");
        break;
    case TokenCode::Hat:
        ctx.assembly().add_instruction("xor", "x0,x0,x2");
        break;
    case TokenCode::Equals: {
        ctx.assembly().add_instruction("eor", "x0,x0,x2");    // a is 0b00000000 (a == b) or 0b00000001 (a != b)
        ctx.assembly().add_instruction("eor", "x0,x0,#0x01"); // a is 0b00000001 (a == b) or 0b00000000 (a != b)
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
        ctx.assembly().add_instruction("mvn", "x0,x0");
        ctx.assembly().add_instruction("add", "x0,#0x01");
        break;
    }
    case TokenCode::Tilde:
        ctx.assembly().add_instruction("mvn", "x0,x0");
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
        ctx.assembly().add_instruction("add", "x0,x0,x2");
        break;
    case TokenCode::Minus:
        ctx.assembly().add_instruction("sub", "x0,x0,x2");
        break;
    case TokenCode::Asterisk:
        ctx.assembly().add_instruction("smull", "x0,x0,x2");
        break;
    case TokenCode::Slash:
        ctx.assembly().add_instruction("sdiv", "x0,x0,x2");
        break;
    case TokenCode::EqualsTo: {
        ctx.assembly().add_instruction("cmp", "x0,x2");
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bne", set_false);
        ctx.assembly().add_instruction("mov", "x0,#0x01");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("mov", "x0,#0x00");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::GreaterThan: {
        ctx.assembly().add_instruction("cmp", "x0,x2");
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_false);
        ctx.assembly().add_instruction("mov", "x0,#0x01");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("mov", "x0,#0x00");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::LessThan: {
        ctx.assembly().add_instruction("cmp", "x0,x2");
        auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_true);
        ctx.assembly().add_instruction("mov", "x0,#0x00");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_true);
        ctx.assembly().add_instruction("mov", "x0,#0x01");
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
        ctx.assembly().add_instruction("mvnb", "w0,w0");
        ctx.assembly().add_instruction("addb", "w0,#0x01");
        break;
    }
    case TokenCode::Tilde:
        ctx.assembly().add_instruction("mvnb", "w0,w0");
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> byte_byte_binary_expression(MacOSXContext& ctx, BinaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::Plus:
        ctx.assembly().add_instruction("andb", "w0,w0,w1");
        break;
    case TokenCode::Minus:
        ctx.assembly().add_instruction("subb", "w0,w0,w1");
        break;
    case TokenCode::Asterisk:
        ctx.assembly().add_instruction("smull", "x0,w0,w2");
        break;
    case TokenCode::Slash:
        ctx.assembly().add_instruction("sdiv", "w0,w0,w2");
        break;
    case TokenCode::EqualsTo: {
        ctx.assembly().add_instruction("cmpb", "w0,w1");
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bne", set_false);
        ctx.assembly().add_instruction("movb", "w0,#0x01");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("movb", "w0,#0x00");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::GreaterThan: {
        ctx.assembly().add_instruction("cmpb", "w0,w1");
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_false);
        ctx.assembly().add_instruction("movb", "w0,#0x01");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("movb", "w0,#0x00");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::LessThan: {
        ctx.assembly().add_instruction("cmpb", "w0,w1");
        auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_true);
        ctx.assembly().add_instruction("movb", "w0,#0x00");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_true);
        ctx.assembly().add_instruction("movb", "w0,#0x01");
        ctx.assembly().add_label(done);
        return {};
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> string_binary_expression(MacOSXContext& ctx, BinaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::Plus:
        break;
    case TokenCode::Asterisk:
        break;
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
            ctx.reset_register();
            TRY_RETURN(output_macosx_processor(argument, ctx));
            push(ctx, "x0");
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
        return tree;
    }

    case SyntaxNodeType::CompilerIntrinsic: {
        auto call = std::dynamic_pointer_cast<CompilerIntrinsic>(tree);
        if (call->name() == "write") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "w2,w1");
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,#1");
            ctx.assembly().syscall(0x04);
        }
        if (call->name() == "exit") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x01);
        }
        if (call->name() == "allocate") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,#0");
            ctx.assembly().add_instruction("mov", "w2,#0x03");
            ctx.assembly().add_instruction("mov", "w3,#0x1002");
            ctx.assembly().add_instruction("mov", "w4,#-1");
            ctx.assembly().add_instruction("mov", "w5,#-1");
            ctx.assembly().syscall(197);
        }
        return tree;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        if (expr->lhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->lhs()->to_string(0) };
        }
        if (expr->rhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->rhs()->to_string(0) };
        }
        auto rhs = TRY_AND_CAST(Expression, output_macosx_processor(expr->rhs(), ctx));
        if (ctx.current_register() >= 19)
            return Error { ErrorCode::InternalError, "FIXME: Maximum expression depth reached" };
        auto reg = format("x{}", ctx.current_register());
        ctx.next_register();
        ctx.assembly().add_instruction("mov", format("{},x0", reg));

        std::string len_reg;
        if (rhs->type() == ObelixType::TypeString) {
            if (ctx.current_register() >= 19)
                return Error { ErrorCode::InternalError, "FIXME: Maximum expression depth reached" };
            len_reg = format("w{}", ctx.current_register());
            ctx.next_register();
            ctx.assembly().add_instruction("mov", format("{},w1", reg));
        }

        auto lhs = TRY_AND_CAST(Expression, output_macosx_processor(expr->lhs(), ctx));

        ctx.assembly().add_instruction("mov", format("x2,{}", reg));
        if (rhs->type() == ObelixType::TypeString) {
            ctx.assembly().add_instruction("mov", format("x3,{}", len_reg));
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
        if (expr->lhs()->type() == ObelixType::TypeString) {
            TRY_RETURN(string_binary_expression(ctx, *expr));
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
            ctx.assembly().add_instruction("mov", format("x0,#{}", val->to_long().value()));
            break;
        }
        case ObelixType::TypeChar:
        case ObelixType::TypeByte:
        case ObelixType::TypeBoolean: {
            ctx.assembly().add_instruction("mov", format("w0,#{}", static_cast<uint8_t>(val->to_long().value())));
            break;
        }
        case ObelixType::TypeString: {
            auto str_id = Label::reserve_id();
            ctx.assembly().add_instruction("adr", format("x0,str_{}", str_id));
            ctx.assembly().add_instruction("mov", format("w1,#{}", val->to_string().length()));
            ctx.assembly().add_string(str_id, val->to_string());
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
            ctx.assembly().add_instruction("ldr", format("x0,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("ldr", format("x0,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("ldrbs", format("w0,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("ldrb", format("w0,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeString:
            ctx.assembly().add_instruction("ldr", format("x0,[fp,{}]", idx->to_long().value()));
            ctx.assembly().add_instruction("ldrw", format("w1,[fp,{}]", idx->to_long().value() + 8));
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

        // Evaluate the expression into w0:
        ctx.reset_register();
        TRY_RETURN(output_macosx_processor(assignment->expression(), ctx));

        switch (assignment->type()) {
        case ObelixType::TypeInt:
            ctx.assembly().add_instruction("str", format("x0,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("str", format("x0,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("strbs", format("x0,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("strb", format("x0,[fp,{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("str", format("x0,[fp,{}]", idx->to_long().value()));
            ctx.assembly().add_instruction("strw", format("w1,[fp,{}]", idx->to_long().value() + 8));
        }
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot emit assignments of type {} yet", assignment->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        ctx.reset_register();
        auto offset = (signed char)ctx.get("#offset").value()->to_long().value();
        ctx.set("#offset", make_obj<Integer>(offset + 16)); // FIXME Use type size
        ctx.declare(var_decl->variable().identifier(), make_obj<Integer>(offset));
        if (var_decl->expression() != nullptr) {
            TRY_RETURN(output_macosx_processor(var_decl->expression(), ctx));
        } else {
            switch (var_decl->expression()->type()) {
            case ObelixType::TypeString:
                ctx.assembly().add_instruction("mov", "w1,0");
                // fall through
            case ObelixType::TypeInt:
            case ObelixType::TypeUnsigned:
            case ObelixType::TypeByte:
            case ObelixType::TypeChar:
            case ObelixType::TypeBoolean:
                ctx.assembly().add_instruction("mov", "x0,0");
                break;
            default:
                return Error { ErrorCode::NotYetImplemented, format("Cannot initialize variables of type {} yet", var_decl->expression()->type()) };
            }
        }
        ctx.assembly().add_instruction("str", "x0,[sp],-16");
        if (var_decl->expression()->type() == ObelixType::TypeString) {
            ctx.assembly().add_instruction("strw", "w1,[sp,8]");
        }
        return tree;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        ctx.reset_register();
        TRY_RETURN(output_macosx_processor(expr_stmt->expression(), ctx));
        return tree;
    }

    case SyntaxNodeType::Return: {
        auto ret = std::dynamic_pointer_cast<Return>(tree);
        ctx.reset_register();
        TRY_RETURN(output_macosx_processor(ret->expression(), ctx));

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
            ctx.reset_register();
            auto cond = TRY_AND_CAST(Expression, output_macosx_processor(branch->condition(), ctx));
            ctx.assembly().add_instruction("cmpb", "w0,0x00");
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

ErrorOrNode extract_intrinsics_processor(std::shared_ptr<SyntaxNode> const& tree, Context<int>& ctx)
{
    switch (tree->node_type()) {
    case SyntaxNodeType::FunctionCall: {
        auto call = std::dynamic_pointer_cast<FunctionCall>(tree);

        if (call->name() == "allocate" || call->name() == "exit" || call->name() == "write") {
            return std::make_shared<CompilerIntrinsic>(call);
        }
        return tree;
    }
    default:
        return process_tree(tree, ctx, extract_intrinsics_processor);
    }
}

ErrorOrNode extract_intrinsics(std::shared_ptr<SyntaxNode> const& tree)
{
    Context<int> root;
    return extract_intrinsics_processor(tree, root);
}

ErrorOrNode output_macosx(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name)
{
    auto processed = TRY(extract_intrinsics(tree));

    Assembly assembly;
    MacOSXContext root(assembly);

    assembly.code = ".global _start\n" // Provide program starting address to linker
                    ".align 2\n\n"
                    "_start:\n"
                    "\tmov\tfp,sp\n"
                    "\tbl\tfunc_main\n"
                    "\tmov\tx16,#1\n"
                    "\tsvc\t0\n";

    auto ret = output_macosx_processor(processed, root);

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
