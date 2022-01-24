/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <config.h>
#include <core/Logging.h>
#include <obelix/Intrinsics.h>
#include <obelix/MacOSX.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

struct Assembly {
    std::string code;
    std::string text;
    std::string data;

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
        text = format("{}.align 2\nstr_{}:\n\t.string\t\"{}\"\n", text, id, str);
    }

    void add_comment(std::string const& comment)
    {
        auto c = comment;
        for (auto pos = c.find('\n'); pos != std::string::npos; pos = c.find('\n'))
            c[pos] = ' ';
        code = format("{}\n\t; {}\n", code, c);
    }

    void add_data(std::string const& label, std::string d)
    {
        if (data.empty())
            data = ".data\n\n";
        data = format("{}\n.align 2\n{}:\t{}", data, label, d);
    }

    void syscall(int id)
    {
        add_instruction("mov", format("x16, #{}", id));
        add_instruction("svc", "#0x80");
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

    int claim_register()
    {
        if (m_available_registers.empty()) {
            fatal("Registers exhausted");
        }
        auto ret = m_available_registers.back();
        m_available_registers.pop_back();
        return ret;
    }

    void release_register(int reg)
    {
        m_available_registers.push_back(reg);
    }

    void release_all()
    {
        m_available_registers = { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };
    }

private:
    Assembly& m_assembly;
    std::vector<int> m_available_registers { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };
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
        ctx.assembly().add_instruction("neg", "x0,x0");
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
        ctx.assembly().add_instruction("mov", "w0,wzr");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::GreaterThan: {
        ctx.assembly().add_instruction("cmp", "x0,x2");
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b.le", set_false);
        ctx.assembly().add_instruction("mov", "x0,#0x01");
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("mov", "w0,wzr");
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::LessThan: {
        ctx.assembly().add_instruction("cmp", "x0,x2");
        auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b.lt", set_true);
        ctx.assembly().add_instruction("mov", "w0,wzr");
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
        ctx.assembly().add_instruction("cmp", "w0,w1");
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
        ctx.assembly().add_instruction("cmp", "w0,w1");
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
        ctx.assembly().add_instruction("cmp", "w0,w1");
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
        ctx.assembly().add_comment(func_decl->to_string(0));

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

    case SyntaxNodeType::NativeFunctionDef: {
        auto native_func_def = std::dynamic_pointer_cast<NativeFunctionDef>(tree);

        // Emit function prolog:
        auto func_decl = TRY_AND_CAST(FunctionDecl, output_macosx_processor(native_func_def->declaration(), ctx));

        // Load arguments in registers w0,w1, etc. Strings use two registers.
        auto reg = 0;
        for (auto& parameter : func_decl->parameters()) {
            auto ix = ctx.get(parameter.name()).value()->to_long().value();
            ctx.assembly().add_instruction("ldr", format("x{},[fp,{}]", reg++, ix));
            if (parameter.type() == ObelixType::TypeString)
                ctx.assembly().add_instruction("ldr", format("x{},[fp,{}]", reg++, ix + 8));
        }

        // Call the native function
        ctx.assembly().add_instruction("bl", native_func_def->native_function_name());

        // Load sp with the current value of bp. This will discard all local variables
        ctx.assembly().add_instruction("mov", "sp,fp");

        // Pop return address into lr:
        pop(ctx, "lr");

        // Return:
        ctx.assembly().add_instruction("ret");
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
            ctx.release_all();
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
        ctx.release_all();

        if (call->name() == "allocate") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,xzr");
            ctx.assembly().add_instruction("mov", "w2,#3");
            ctx.assembly().add_instruction("mov", "w3,#0x1002");
            ctx.assembly().add_instruction("mov", "w4,#-1");
            ctx.assembly().add_instruction("mov", "x5,xzr");
            ctx.assembly().syscall(0xC5);
        }

        if (call->name() == "close") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x06);
        }

        if (call->name() == "fputs") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            auto fd_reg = ctx.claim_register();
            ctx.assembly().add_instruction("mov", format("w{},w0", fd_reg));
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            ctx.assembly().add_instruction("mov", "w2,w1");
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", format("w0,w{}", fd_reg));
            ctx.assembly().syscall(0x04);
        }

        if (call->name() == "itoa") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x2,x0");
            ctx.assembly().add_instruction("sub", "sp,sp,32");
            ctx.assembly().add_instruction("add", "x0,sp,16");
            ctx.assembly().add_instruction("mov", "x1,#32");
            ctx.assembly().add_instruction("mov", "w3,#10");
            ctx.assembly().add_instruction("bl", "to_string");
            ctx.assembly().add_instruction("add", "sp,sp,32");
        }

        if (call->name() == "exit") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x01);
        }

        if (call->name() == "eputs") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "w2,w1");
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,#0x02");
            ctx.assembly().syscall(0x04);
        }

        if (call->name() == "fsize") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("sub", format("x1,sp,#-{}", sizeof(struct stat)));
            ctx.assembly().syscall(189);
            ctx.assembly().add_instruction("cmp", "x0,#0x00");
            auto lbl = format("lbl_{}", Label::reserve_id());
            ctx.assembly().add_instruction("bne", lbl);
            ctx.assembly().add_instruction("ldr", format("x0,[sp,-{}]", sizeof(struct stat) - offsetof(struct stat, st_size)));
            ctx.assembly().add_label(lbl);
        }

        if (call->name() == "memset") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(2), ctx));
            int len_reg = ctx.claim_register();
            ctx.assembly().add_instruction("mov", format("x{},x0", len_reg));
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            int char_reg = ctx.claim_register();
            ctx.assembly().add_instruction("mov", format("x{},x0", char_reg));
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));

            int count_reg = ctx.claim_register();
            ctx.assembly().add_instruction("mov", format("x{},xzr", count_reg));

            auto loop = format("lbl_{}", Label::reserve_id());
            auto skip = format("lbl_{}", Label::reserve_id());
            ctx.assembly().add_label(loop);
            ctx.assembly().add_instruction("cmp", format("x{},x{}", count_reg, len_reg));
            ctx.assembly().add_instruction("b.ge", skip);

            int ptr_reg = ctx.claim_register();
            ctx.assembly().add_instruction("add", format("x{},x0,x{}", ptr_reg, count_reg));
            ctx.assembly().add_instruction("strb", format("w{},[x{}]", char_reg, ptr_reg));
            ctx.assembly().add_instruction("add", format("x{},x{},#1", count_reg, count_reg));
            ctx.assembly().add_instruction("b", loop);
            ctx.assembly().add_label(skip);
            ctx.release_register(ptr_reg);
            ctx.release_register(count_reg);
            ctx.release_register(len_reg);
            ctx.release_register(char_reg);
        }

        if (call->name() == "open") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            auto flags_reg = ctx.claim_register();
            ctx.assembly().add_instruction("mov", format("x{},x0", flags_reg));
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", format("x1,x{}", flags_reg));
            ctx.release_register(flags_reg);
            ctx.assembly().syscall(0x05);
        }

        if (call->name() == "putchar") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("strb", "w0,[sp],-16");
            ctx.assembly().add_instruction("mov", "x0,#1");    // x0: stdin
            ctx.assembly().add_instruction("add", "x1,sp,16"); // x1: 16 bytes up from SP
            ctx.assembly().add_instruction("mov", "x2,#1");    // x2: Number of characters
            ctx.assembly().syscall(0x04);
            ctx.assembly().add_instruction("add", "sp,sp,16");
        }

        if (call->name() == "puts") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "w2,w1");
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,#1");
            ctx.assembly().syscall(0x04);
        }

        if (call->name() == "read") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(2), ctx));
            auto len_reg = ctx.claim_register();
            ctx.assembly().add_instruction("mov", format("x{},x0", len_reg));
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            auto buf_reg = ctx.claim_register();
            ctx.assembly().add_instruction("mov", format("x{},x0", buf_reg));
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", format("x2,x{}", len_reg));
            ctx.assembly().add_instruction("mov", format("x1,x{}", buf_reg));
            ctx.release_register(buf_reg);
            ctx.release_register(len_reg);
            ctx.assembly().syscall(0x03);
        }

        if (call->name() == "write") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(2), ctx));
            ctx.assembly().add_instruction("mov", "x2,x0");
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            ctx.assembly().add_instruction("mov", "x1,x0");
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x04);
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
        auto reg_num = ctx.claim_register();
        ctx.assembly().add_instruction("mov", format("x{},x0", reg_num));

        int len_reg;
        if (rhs->type() == ObelixType::TypeString) {
            len_reg = ctx.claim_register();
            ctx.assembly().add_instruction("mov", format("w{},w1", len_reg));
        }

        auto lhs = TRY_AND_CAST(Expression, output_macosx_processor(expr->lhs(), ctx));

        ctx.assembly().add_instruction("mov", format("x2,x{}", reg_num));
        ctx.release_register(reg_num);
        if (rhs->type() == ObelixType::TypeString) {
            ctx.assembly().add_instruction("mov", format("x3,x{}", len_reg));
            ctx.release_register(len_reg);
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
        case ObelixType::TypePointer:
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
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
            ctx.assembly().add_instruction("ldr", format("x0,[fp,-{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("ldr", format("x0,[fp,-{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("ldrbs", format("w0,[fp,-{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("ldrb", format("w0,[fp,-{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeString:
            ctx.assembly().add_instruction("ldr", format("x0,[fp,-{}]", idx->to_long().value()));
            ctx.assembly().add_instruction("ldrw", format("w1,[fp,-{}]", idx->to_long().value() + 8));
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
        ctx.release_all();
        TRY_RETURN(output_macosx_processor(assignment->expression(), ctx));

        switch (assignment->type()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
            ctx.assembly().add_instruction("str", format("x0,[fp,-{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("str", format("x0,[fp,-{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("strbs", format("x0,[fp,-{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("strb", format("x0,[fp,-{}]", idx->to_long().value()));
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("str", format("x0,[fp,-{}]", idx->to_long().value()));
            ctx.assembly().add_instruction("strw", format("w1,[fp,-{}]", idx->to_long().value() + 8));
        }
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot emit assignments of type {} yet", assignment->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        ctx.assembly().add_comment(var_decl->to_string(0));
        ctx.release_all();
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
            case ObelixType::TypePointer:
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
        ctx.assembly().add_comment(expr_stmt->to_string(0));
        ctx.release_all();
        TRY_RETURN(output_macosx_processor(expr_stmt->expression(), ctx));
        return tree;
    }

    case SyntaxNodeType::Return: {
        auto ret = std::dynamic_pointer_cast<Return>(tree);
        ctx.assembly().add_comment(ret->to_string(0));
        ctx.release_all();
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
        ctx.assembly().add_comment(label->to_string(0));
        ctx.assembly().add_label(format("lbl_{}", label->label_id()));
        return tree;
    }

    case SyntaxNodeType::Goto: {
        auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
        ctx.assembly().add_comment(goto_stmt->to_string(0));
        ctx.assembly().add_instruction("b", format("lbl_{}", goto_stmt->label_id()));
        return tree;
    }

    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);

        auto end_label = Obelix::Label::reserve_id();
        auto count = if_stmt->branches().size() - 1;
        for (auto& branch : if_stmt->branches()) {
            auto else_label = (count) ? Obelix::Label::reserve_id() : end_label;
            if (branch->condition()) {
                ctx.assembly().add_comment(format("if ({})", branch->condition()->to_string(0)));
                ctx.release_all();
                auto cond = TRY_AND_CAST(Expression, output_macosx_processor(branch->condition(), ctx));
                ctx.assembly().add_instruction("cmp", "w0,0x00");
                ctx.assembly().add_instruction("b.eq", format("lbl_{}", else_label));
            } else {
                ctx.assembly().add_comment("else");
            }
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

        if (is_intrinsic(call->name())) {
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

    assembly.code = ".global _start\n"
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

        std::string obl_dir = (getenv("OBL_DIR")) ? getenv("OBL_DIR") : OBELIX_DIR;
        auto as_cmd = format("as -o {}.o {}.s", bare_file_name, bare_file_name);
        auto ld_cmd = format("ld -o {} {}.o -loblrt -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -e _start -arch arm64 -L{}/lib",
            bare_file_name, bare_file_name, obl_dir);

        std::cout << "[CMD] " << as_cmd << "\n";
        if (!system(as_cmd.c_str())) {
            std::cout << "[CMD] " << ld_cmd << "\n";
            system(ld_cmd.c_str());
        }
    }
    return ret;
}

}
