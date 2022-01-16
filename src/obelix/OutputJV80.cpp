/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <fcntl.h>

#include <obelix/OutputJV80.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>
#include <oblasm/Image.h>
#include <oblasm/Instruction.h>

namespace Obelix {

extern_logging_category(parser);

using namespace Obelix::Assembler;

class OutputJV80Context : public Context<Obj> {
public:
    OutputJV80Context(OutputJV80Context& parent)
        : Context<Obj>(parent)
        , m_image(parent.image())
        , m_code(parent.code())
    {
        auto offset_maybe = get("#offset");
        assert(offset_maybe.has_value());
        declare("#offset", get("#offset").value());
    }

    explicit OutputJV80Context(OutputJV80Context* parent)
        : Context<Obj>(parent)
        , m_image(parent->image())
        , m_code(parent->code())
    {
        auto offset_maybe = get("#offset");
        assert(offset_maybe.has_value());
        declare("#offset", get("#offset").value());
    }

    explicit OutputJV80Context(Obelix::Assembler::Image& image)
        : Context()
        , m_image(image)
        , m_code(m_image.get_segment(0))
    {
        declare("#offset", make_obj<Integer>(0));
    }

    Obelix::Assembler::Image& image() { return m_image; }
    std::shared_ptr<Obelix::Assembler::Segment> const& code() { return m_code; }

private:
    Obelix::Assembler::Image& m_image;
    std::shared_ptr<Obelix::Assembler::Segment> m_code;
};

void add_instruction(Segment& code, Mnemonic mnemonic, Register reg)
{
    code.add(std::make_shared<Instruction>(mnemonic, Argument { .addressing_mode = AMRegister, .reg = reg }));
}

void add_instruction(Segment& code, Mnemonic mnemonic, std::string label)
{
    code.add(std::make_shared<Instruction>(mnemonic,
        Argument { .addressing_mode = Assembler::AMImmediate, .immediate_type = Argument::ImmediateType::Label, .label = label }));
}

void add_instruction(Segment& code, Mnemonic mnemonic, uint16_t constant)
{
    code.add(std::make_shared<Instruction>(mnemonic,
        Argument { .addressing_mode = Assembler::AMImmediate, .immediate_type = Argument::ImmediateType::Constant, .constant = constant }));
}

void add_instruction(Segment& code, Mnemonic mnemonic, Register dest, Register src)
{
    code.add(std::make_shared<Instruction>(mnemonic,
        Argument { .addressing_mode = AMRegister, .reg = dest },
        Argument { .addressing_mode = AMRegister, .reg = src }));
}

void add_instruction(Segment& code, Mnemonic mnemonic, Register dest, uint16_t constant)
{
    code.add(std::make_shared<Instruction>(mnemonic,
        Argument { .addressing_mode = AMRegister, .reg = dest },
        Argument { .addressing_mode = Assembler::AMImmediate, .immediate_type = Argument::ImmediateType::Constant, .constant = constant }));
}

void add_indexed_instruction(Segment& code, Mnemonic mnemonic, Register dest, uint8_t idx)
{
    code.add(std::make_shared<Instruction>(mnemonic,
        Argument { .addressing_mode = Assembler::AMIndexed, .constant = idx, .reg = dest }));
}

template<typename ExpressionType>
ErrorOr<void> push_expression_return_value(Obelix::Assembler::Segment& code, ExpressionType const& expr)
{
    switch (expr.type()) {
    case ObelixType::TypeInt:
    case ObelixType::TypeUnsigned:
        code.add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .addressing_mode = AMRegister, .reg = Register::ab }));
        break;
    case ObelixType::TypeBoolean:
    case ObelixType::TypeByte:
    case ObelixType::TypeChar:
        code.add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .addressing_mode = AMRegister, .reg = Register::a }));
        break;
    default:
        Error { ErrorCode::SyntaxError, format("Unexpected return type {} for expression {}", expr.type(), expr.to_string(0)) };
    }
    return {};
}

ErrorOr<void> bool_unary_expression(Image& image, Obelix::Assembler::Segment& code, UnaryExpression const& expr)
{
    add_instruction(code, Mnemonic::POP, Register::a);
    switch (expr.op().code()) {
    case TokenCode::ExclamationPoint:
        add_instruction(code, Mnemonic::XOR, Register::a, 0x01); // a is 0b00000001 (a was true) or 0b00000000 (a was false)
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return push_expression_return_value(code, expr);
}

ErrorOr<void> bool_bool_binary_expression(Image& image, Obelix::Assembler::Segment& code, BinaryExpression const& expr)
{
    add_instruction(code, Mnemonic::POP, Register::b);
    add_instruction(code, Mnemonic::POP, Register::a);
    switch (expr.op().code()) {
    case TokenCode::LogicalAnd:
        add_instruction(code, Mnemonic::AND, Register::a, Register::b);
        break;
    case TokenCode::LogicalOr:
        add_instruction(code, Mnemonic::OR, Register::a, Register::b);
        break;
    case TokenCode::Hat:
        add_instruction(code, Mnemonic::XOR, Register::a, Register::b);
        break;
    case TokenCode::Equals: {
        add_instruction(code, Mnemonic::XOR, Register::a, Register::b); // a is 0b00000000 (a == b) or 0b00000001 (a != b)
        add_instruction(code, Mnemonic::XOR, Register::a, 0x01);        // a is 0b00000001 (a == b) or 0b00000000 (a != b)
        break;
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return push_expression_return_value(code, expr);
}

ErrorOr<void> int_unary_expression(Image& image, Obelix::Assembler::Segment& code, UnaryExpression const& expr)
{
    if (expr.op().code() == TokenCode::Plus)
        return {};
    add_instruction(code, Mnemonic::POP, Register::ab);
    switch (expr.op().code()) {
    case TokenCode::Minus: {
        if (expr.operand()->type() == ObelixType::TypeUnsigned)
            return Error { ErrorCode::SyntaxError, "Cannot negate unsigned numbers" };

        // Perform two's complement. Negate and add 1:
        add_instruction(code, Mnemonic::NOT, Register::a);
        add_instruction(code, Mnemonic::NOT, Register::b);
        add_instruction(code, Mnemonic::CLR, Register::d);
        add_instruction(code, Mnemonic::MOV, Register::c, 0x01);
        add_instruction(code, Mnemonic::ADD, Register::ab, Register::cd);
        break;
    }
    case TokenCode::Tilde:
        add_instruction(code, Mnemonic::NOT, Register::a);
        add_instruction(code, Mnemonic::NOT, Register::b);
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return push_expression_return_value(code, expr);
}

ErrorOr<void> int_int_binary_expression(Image& image, Obelix::Assembler::Segment& code, BinaryExpression const& expr)
{
    add_instruction(code, Mnemonic::POP, Register::cd);
    add_instruction(code, Mnemonic::POP, Register::ab);
    switch (expr.op().code()) {
    case TokenCode::Plus:
        add_instruction(code, Mnemonic::ADD, Register::ab, Register::cd);
        break;
    case TokenCode::Minus:
        add_instruction(code, Mnemonic::SUB, Register::ab, Register::cd);
        break;
    case TokenCode::EqualsTo: {
        add_instruction(code, Mnemonic::CMP, Register::b, Register::d);
        auto push_false = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JNZ, push_false);
        add_instruction(code, Mnemonic::CMP, Register::a, Register::c);
        add_instruction(code, Mnemonic::JNZ, push_false);
        add_instruction(code, Mnemonic::PUSH, 0x01);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JMP, done);
        image.add(std::make_shared<Assembler::Label>(push_false, image.current_address()));
        add_instruction(code, Mnemonic::PUSH, 0x00);
        image.add(std::make_shared<Assembler::Label>(done, image.current_address()));
        return {};
    }
    case TokenCode::GreaterThan: {
        add_instruction(code, Mnemonic::CMP, Register::b, Register::d);
        auto push_false = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JNC, push_false);
        add_instruction(code, Mnemonic::CMP, Register::a, Register::c);
        add_instruction(code, Mnemonic::JNC, push_false);
        add_instruction(code, Mnemonic::PUSH, 0x01);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JMP, done);
        image.add(std::make_shared<Assembler::Label>(push_false, image.current_address()));
        add_instruction(code, Mnemonic::PUSH, 0x00);
        image.add(std::make_shared<Assembler::Label>(done, image.current_address()));
        return {};
    }
    case TokenCode::LessThan: {
        add_instruction(code, Mnemonic::CMP, Register::b, Register::d);
        auto push_false = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JC, push_false);
        add_instruction(code, Mnemonic::CMP, Register::a, Register::c);
        add_instruction(code, Mnemonic::JC, push_false);
        add_instruction(code, Mnemonic::PUSH, 0x01);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JMP, done);
        image.add(std::make_shared<Assembler::Label>(push_false, image.current_address()));
        add_instruction(code, Mnemonic::PUSH, 0x00);
        image.add(std::make_shared<Assembler::Label>(done, image.current_address()));
        return {};
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return push_expression_return_value(code, expr);
}

ErrorOr<void> byte_unary_expression(Image& image, Obelix::Assembler::Segment& code, UnaryExpression const& expr)
{
    if (expr.op().code() == TokenCode::Plus)
        return {};
    add_instruction(code, Mnemonic::POP, Register::ab);
    switch (expr.op().code()) {
    case TokenCode::Minus: {
        if (expr.operand()->type() == ObelixType::TypeChar)
            return Error { ErrorCode::SyntaxError, "Cannot negate unsigned numbers" };

        // Perform two's complement. Negate and add 1:
        add_instruction(code, Mnemonic::NOT, Register::a);
        add_instruction(code, Mnemonic::INC, Register::a, 0x01);
        break;
    }
    case TokenCode::Tilde:
        add_instruction(code, Mnemonic::NOT, Register::a);
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return push_expression_return_value(code, expr);
}

ErrorOr<void> byte_byte_binary_expression(Image& image, Obelix::Assembler::Segment& code, BinaryExpression const& expr)
{
    add_instruction(code, Mnemonic::POP, Register::b);
    add_instruction(code, Mnemonic::POP, Register::a);
    switch (expr.op().code()) {
    case TokenCode::Plus:
        add_instruction(code, Mnemonic::ADD, Register::a, Register::b);
        break;
    case TokenCode::Minus:
        add_instruction(code, Mnemonic::SUB, Register::a, Register::b);
        break;
    case TokenCode::Equals: {
        add_instruction(code, Mnemonic::CMP, Register::a, Register::b);
        auto push_false = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JNZ, push_false);
        add_instruction(code, Mnemonic::PUSH, 0x01);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JMP, done);
        image.add(std::make_shared<Assembler::Label>(push_false, image.current_address()));
        add_instruction(code, Mnemonic::PUSH, 0x00);
        image.add(std::make_shared<Assembler::Label>(done, image.current_address()));
        return {};
    }
    case TokenCode::GreaterThan: {
        add_instruction(code, Mnemonic::CMP, Register::a, Register::b);
        auto push_false = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JC, push_false);
        add_instruction(code, Mnemonic::PUSH, 0x01);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JMP, done);
        image.add(std::make_shared<Assembler::Label>(push_false, image.current_address()));
        add_instruction(code, Mnemonic::PUSH, 0x00);
        image.add(std::make_shared<Assembler::Label>(done, image.current_address()));
        return {};
    }
    case TokenCode::LessThan: {
        add_instruction(code, Mnemonic::CMP, Register::a, Register::b);
        auto push_false = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JNC, push_false);
        add_instruction(code, Mnemonic::PUSH, 0x01);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        add_instruction(code, Mnemonic::JMP, done);
        image.add(std::make_shared<Assembler::Label>(push_false, image.current_address()));
        add_instruction(code, Mnemonic::PUSH, 0x00);
        image.add(std::make_shared<Assembler::Label>(done, image.current_address()));
        return {};
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return push_expression_return_value(code, expr);
}

ErrorOrNode output_jv80_processor(std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx)
{
    auto code = ctx.code();

    switch (tree->node_type()) {
    case SyntaxNodeType::FunctionDecl: {
        auto func_decl = std::dynamic_pointer_cast<FunctionDecl>(tree);

        auto ix = -8;
        for (auto& parameter : func_decl->parameters()) {
            ctx.declare(parameter.name(), make_obj<Integer>(ix));
            ix -= 2;
        }

        ctx.image().add(std::make_shared<Obelix::Assembler::Label>(format("func_{}", func_decl->name()), ctx.image().current_address()));
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
        code->add(std::make_shared<Instruction>(Mnemonic::MOV,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp },
            Argument { .addressing_mode = AMRegister, .reg = Register::sp }));
        return tree;
    }

        /*
         *  +--------------------+
         *  |       ....         |
         *  +--------------------+
         *  |    local var #1    |
         *  +--------------------+   <---- Called function BP
         *  |      Temp bp       |
         *  +------------------- +
         *  |     Return addr    |
         *  +--------------------+   <- bp[-4]
         *  |    return value    |
         *  +--------------------+   <- bp[-6]
         *  |     argument 1     |
         *  +--------------------+
         *  |       ....         |
         *  +--------------------+
         *  |    argument n-1    |
         *  +------------------- +
         *  |     argument n     |
         *  +--------------------+   <---- Temp BP
         *  | Caller function bp |
         *  +--------------------+
         */

    case SyntaxNodeType::FunctionCall: {
        auto processed = TRY(process_tree(tree, ctx, output_jv80_processor));
        auto call = std::dynamic_pointer_cast<FunctionCall>(processed);

        code->add(std::make_shared<Instruction>(Mnemonic::PUSH,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
        code->add(std::make_shared<Instruction>(Mnemonic::MOV,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp },
            Argument { .addressing_mode = AMRegister, .reg = Register::sp }));

        // The arguments will be pushed in reverse order when the argument list was processed.
        // Reserve spot for return code on stack (will be bp[-4])
        code->add(std::make_shared<Instruction>(Mnemonic::PUSHW,
            Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Constant, .constant = 0 }));

        // Call function:
        auto function = call->name();
        auto label = format("func_{}", function);
        code->add(std::make_shared<Instruction>(Mnemonic::CALL,
            Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Label, .label = label }));
        // Pop return value:
        code->add(std::make_shared<Instruction>(Mnemonic::POP,
            Argument { .addressing_mode = AMRegister, .reg = Register::di }));

        // Load sp with the popped value of bp. This will discard all function arguments
        code->add(std::make_shared<Instruction>(Mnemonic::MOV,
            Argument { .addressing_mode = AMRegister, .reg = Register::sp },
            Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
        code->add(std::make_shared<Instruction>(Mnemonic::POP,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp }));

        code->add(std::make_shared<Instruction>(Mnemonic::PUSH,
            Argument { .addressing_mode = AMRegister, .reg = Register::di }));
        return processed;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto processed = TRY(process_tree(tree, ctx, output_jv80_processor));
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(processed);

        if (expr->lhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->lhs()->to_string(0) };
        }
        if (expr->rhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->rhs()->to_string(0) };
        }

        if ((expr->lhs()->type() == ObelixType::TypeInt && expr->lhs()->type() == ObelixType::TypeInt) || (expr->lhs()->type() == ObelixType::TypeUnsigned && expr->lhs()->type() == ObelixType::TypeUnsigned)) {
            TRY_RETURN(int_int_binary_expression(ctx.image(), *code, *expr));
        }
        if ((expr->lhs()->type() == ObelixType::TypeByte && expr->lhs()->type() == ObelixType::TypeByte) || (expr->lhs()->type() == ObelixType::TypeChar && expr->lhs()->type() == ObelixType::TypeChar)) {
            TRY_RETURN(byte_byte_binary_expression(ctx.image(), *code, *expr));
        }
        if (expr->lhs()->type() == ObelixType::TypeBoolean && expr->lhs()->type() == ObelixType::TypeBoolean) {
            TRY_RETURN(bool_bool_binary_expression(ctx.image(), *code, *expr));
        }
        return tree;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto processed = TRY(process_tree(tree, ctx, output_jv80_processor));
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(processed);

        if (expr->operand()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->operand()->to_string(0) };
        }

        if (expr->operand()->type() == ObelixType::TypeInt || expr->operand()->type() == ObelixType::TypeUnsigned) {
            TRY_RETURN(int_unary_expression(ctx.image(), *code, *expr));
        }
        if (expr->operand()->type() == ObelixType::TypeByte || expr->operand()->type() == ObelixType::TypeChar) {
            TRY_RETURN(byte_unary_expression(ctx.image(), *code, *expr));
        }
        if (expr->operand()->type() == ObelixType::TypeBoolean) {
            TRY_RETURN(bool_unary_expression(ctx.image(), *code, *expr));
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
            add_instruction(*code, Mnemonic::PUSHW, static_cast<uint16_t>(val->to_long().value()));
            break;
        }
        case ObelixType::TypeChar:
        case ObelixType::TypeByte:
        case ObelixType::TypeBoolean: {
            add_instruction(*code, Mnemonic::PUSH, static_cast<uint8_t>(val->to_long().value()));
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
            add_indexed_instruction(*code, Mnemonic::PUSH, Register::bp, static_cast<uint8_t>(idx->to_long().value()));
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeByte:
        case ObelixType::TypeBoolean:
            add_indexed_instruction(*code, Mnemonic::PUSH, Register::bp, static_cast<uint8_t>(idx->to_long().value()));
            add_instruction(*code, Mnemonic::POP, Register::cd);
            add_instruction(*code, Mnemonic::PUSH, Register::c);
            break;
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot push values of variables of type {} yet", identifier->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::Assignment: {
        auto assignment = std::dynamic_pointer_cast<Assignment>(tree);
        TRY_RETURN(output_jv80_processor(assignment->expression(), ctx));
        auto idx_maybe = ctx.get(assignment->name());
        if (!idx_maybe.has_value())
            return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", assignment->name()) };
        auto idx = (uint16_t)idx_maybe.value();

        switch (assignment->type()) {
        case ObelixType::TypeInt:
        case ObelixType::TypeUnsigned:
            add_instruction(*code, Mnemonic::POP, Register::di);
            code->add(std::make_shared<Instruction>(Mnemonic::MOV,
                Argument { .addressing_mode = Assembler::AMIndexed, .constant = (uint16_t)idx, .reg = Register::bp },
                Argument { .addressing_mode = Assembler::AMRegister, .reg = Register::di }));
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
        case ObelixType::TypeByte:
            add_instruction(*code, Mnemonic::POP, Register::a);
            code->add(std::make_shared<Instruction>(Mnemonic::MOV,
                Argument { .addressing_mode = Assembler::AMIndexed, .constant = (uint16_t)idx, .reg = Register::bp },
                Argument { .addressing_mode = Assembler::AMRegister, .reg = Register::a }));
            break;
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot emit assignments of type {} yet", assignment->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto processed = TRY(process_tree(tree, ctx, output_jv80_processor));
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(processed);
        auto offset = (signed char)ctx.get("#offset").value()->to_long().value();
        ctx.set("#offset", make_obj<Integer>(offset + 2)); // FIXME Use type size
        ctx.declare(var_decl->variable().identifier(), make_obj<Integer>(offset));
        if (var_decl->expression() == nullptr) {
            switch (var_decl->expression()->type()) {
            case ObelixType::TypeInt:
            case ObelixType::TypeUnsigned:
            case ObelixType::TypeByte:
            case ObelixType::TypeChar:
            case ObelixType::TypeBoolean:
                code->add(std::make_shared<Instruction>(Mnemonic::PUSHW, Argument { .addressing_mode = AMImmediate, .constant = 0 }));
                break;
            default:
                return Error { ErrorCode::NotYetImplemented, format("Cannot initialize variables of type {} yet", var_decl->expression()->type()) };
            }
        }
        return tree;
    };

    case SyntaxNodeType::Return: {
        auto processed = TRY(process_tree(tree, ctx, output_jv80_processor));
        code->add(std::make_shared<Instruction>(Mnemonic::POP,
            Argument { .addressing_mode = AMRegister, .reg = Register::di }));
        code->add(std::make_shared<Instruction>(Mnemonic::MOV,
            Argument { .addressing_mode = Assembler::AMIndexed, .constant = (uint16_t)-6, .reg = Register::bp },
            Argument { .addressing_mode = Assembler::AMRegister, .reg = Register::di }));
        // Load sp with the current value of bp. This will discard all local variables
        code->add(std::make_shared<Instruction>(Mnemonic::MOV,
            Argument { .addressing_mode = AMRegister, .reg = Register::sp },
            Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
        // Pop bp:
        code->add(std::make_shared<Instruction>(Mnemonic::POP,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
        code->add(std::make_shared<Instruction>(Mnemonic::RET));
        return tree;
    }

    case SyntaxNodeType::Label: {
        auto label = std::dynamic_pointer_cast<Obelix::Label>(tree);
        ctx.image().add(std::make_shared<Obelix::Assembler::Label>(format("lbl_{}", label->label_id()), ctx.image().current_address()));
        return tree;
    }

    case SyntaxNodeType::Goto: {
        auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
        code->add(std::make_shared<Instruction>(Mnemonic::JMP,
            Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Label, .label = format("lbl_{}", goto_stmt->label_id()) }));
        return tree;
    }

    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);

        for (auto& branch : if_stmt->branches()) {
            auto else_label = Obelix::Label::reserve_id();
            auto cond = TRY_AND_CAST(Expression, output_jv80_processor(branch->condition(), ctx));
            add_instruction(*code, Mnemonic::POP, Register::a);
            add_instruction(*code, Mnemonic::CMP, Register::a, 0x00);
            code->add(std::make_shared<Instruction>(Mnemonic::JNZ, // Jump if false (zero)
                Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Label, .label = format("lbl_{}", else_label) }));
            auto stmt = TRY_AND_CAST(Statement, output_jv80_processor(branch->statement(), ctx));
            ctx.image().add(std::make_shared<Obelix::Assembler::Label>(format("lbl_{}", else_label), ctx.image().current_address()));
        }
        return tree;
    }

    default:
        return process_tree(tree, ctx, output_jv80_processor);
    }
}

ErrorOrNode output_jv80(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name)
{
    using namespace Obelix::Assembler;

    Image image(0xC000);
    OutputJV80Context root(image);
    auto code = root.code();

    code->add(std::make_shared<Instruction>(Mnemonic::MOV,
        Argument { .addressing_mode = AMRegister, .reg = Register::sp },
        Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Constant, .constant = 0xc000 }));
    code->add(std::make_shared<Instruction>(Mnemonic::MOV,
        Argument { .addressing_mode = AMRegister, .reg = Register::bp },
        Argument { .addressing_mode = AMRegister, .reg = Register::sp }));

    code->add(std::make_shared<Instruction>(Mnemonic::PUSH,
        Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
    code->add(std::make_shared<Instruction>(Mnemonic::MOV,
        Argument { .addressing_mode = AMRegister, .reg = Register::bp },
        Argument { .addressing_mode = AMRegister, .reg = Register::sp }));

    code->add(std::make_shared<Instruction>(Mnemonic::PUSHW,
        Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Constant, .constant = 0 }));

    // Call main:
    code->add(std::make_shared<Instruction>(Mnemonic::CALL,
        Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Label, .label = "func_main" }));
    // Pop return value:
    code->add(std::make_shared<Instruction>(Mnemonic::POP,
        Argument { .addressing_mode = AMRegister, .reg = Register::di }));
    code->add(std::make_shared<Instruction>(Mnemonic::MOV,
        Argument { .addressing_mode = AMRegister, .reg = Register::sp },
        Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
    code->add(std::make_shared<Instruction>(Mnemonic::POP,
        Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
    code->add(std::make_shared<Instruction>(Mnemonic::HLT));

    auto ret = output_jv80_processor(tree, root);

    if (ret.has_value() && !file_name.empty()) {
        if (image.assemble().empty()) {
            for (auto& err : image.errors()) {
                fprintf(stderr, "%s\n", err.c_str());
            }
            return Error { ErrorCode::SyntaxError, "Assembler error(s)" };
        }
        image.list(true);
        TRY(image.write(file_name));
    }
    return ret;
}

}
