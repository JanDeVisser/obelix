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

ErrorOrNode output_jv80(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name)
{
    using namespace Obelix::Assembler;

    using OutputJV80Context = Context<std::string>;
    OutputJV80Context::ProcessorMap output_jv80_map;
    Image image(0xFFFF);
    auto code = image.get_segment(0);
    auto data = std::make_shared<Segment>(0xC000);
    image.add(code);
    image.add(data);
    int count = 0;

    code->add(std::make_shared<Instruction>(Mnemonic::MOV,
        Argument { .type = Argument::ArgumentType::Register, .reg = Register::sp },
        Argument { .type = Argument::ArgumentType::Constant, .constant = 0x3c00 }));

    output_jv80_map[SyntaxNodeType::Module] = [](std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx) -> ErrorOrNode {
        return tree;
    };

    output_jv80_map[SyntaxNodeType::BinaryExpression] = [&code](std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx) -> ErrorOrNode {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .type = Argument::ArgumentType::Register, .reg = Register::d }));
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .type = Argument::ArgumentType::Register, .reg = Register::c }));
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .type = Argument::ArgumentType::Register, .reg = Register::b }));
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .type = Argument::ArgumentType::Register, .reg = Register::a }));
        switch (expr->op().code()) {
        case TokenCode::Plus:
            code->add(std::make_shared<Instruction>(Mnemonic::ADD,
                Argument { .type = Argument::ArgumentType::Register, .reg = Register::ab },
                Argument { .type = Argument::ArgumentType::Register, .reg = Register::cd }));
            break;
        case TokenCode::Minus:
            code->add(std::make_shared<Instruction>(Mnemonic::SUB,
                Argument { .type = Argument::ArgumentType::Register, .reg = Register::ab },
                Argument { .type = Argument::ArgumentType::Register, .reg = Register::cd }));
            break;
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr->op().value()));
        }
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .type = Argument::ArgumentType::Register, .reg = Register::a }));
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .type = Argument::ArgumentType::Register, .reg = Register::b }));
        return tree;
    };

    output_jv80_map[SyntaxNodeType::Literal] = [&code](std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx) -> ErrorOrNode {
        auto literal = std::dynamic_pointer_cast<Literal>(tree);
        auto val_maybe = TRY(literal->to_object());
        assert(val_maybe.has_value());
        auto val = val_maybe.value();
        switch (val.type()) {
        case ObelixType::TypeInt: {
            uint16_t v = static_cast<uint16_t>(val->to_long().value());
            code->add(std::make_shared<Instruction>(Mnemonic::MOV,
                Argument { .type = Argument::ArgumentType::Register, .reg = Register::si },
                Argument { .type = Argument::ArgumentType::Constant, .constant = v }));
            code->add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .type = Argument::ArgumentType::Register, .reg = Register::si }));
            break;
        }
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit literals of type {} yet", ObelixType_name(val.type())));
        }
        return tree;
    };

    output_jv80_map[SyntaxNodeType::Identifier] = [&code](std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx) -> ErrorOrNode {
        auto identifier = std::dynamic_pointer_cast<Identifier>(tree);
        code->add(std::make_shared<Instruction>(
            Mnemonic::MOV,
            Argument { .type = Argument::ArgumentType::Register, .reg = Register::si },
            Argument { .indirect = true, .type = Argument::ArgumentType::Label, .label = format("var_{}", identifier->name()) }));
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .type = Argument::ArgumentType::Register, .reg = Register::si }));
        return tree;
    };

    output_jv80_map[SyntaxNodeType::VariableDeclaration] = [&image, &code, &data, &count](std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx) -> ErrorOrNode {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto label = format("var_{}_{}", count++, var_decl->variable().identifier());
        image.add(*data,
            std::make_shared<Assembler::Label>(label, data->current_address()));
        auto bytes = std::make_shared<Bytes>(Mnemonic::DW);
        bytes->append((uint8_t)0);
        data->add(bytes);

        if (var_decl->expression() != nullptr) {
            code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .type = Argument::ArgumentType::Register, .reg = Register::si }));
            code->add(std::make_shared<Instruction>(
                Mnemonic::MOV,
                Argument { .indirect = true, .type = Argument::ArgumentType::Label, .label = format("var_{}", var_decl->variable().identifier()) },
                Argument { .type = Argument::ArgumentType::Register, .reg = Register::si }));
        }
        return tree;
    };

    output_jv80_map[SyntaxNodeType::Return] = [&code](std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx) -> ErrorOrNode {
        auto return_stmt = std::dynamic_pointer_cast<Return>(tree);
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .type = Argument::ArgumentType::Register, .reg = Register::di }));
        code->add(std::make_shared<Instruction>(Mnemonic::HLT));
        return tree;
    };

    OutputJV80Context root(output_jv80_map);

    auto ret = process_tree(tree, root);

    if (ret.has_value() && !file_name.empty()) {
        if (image.assemble().empty()) {
            for (auto& err : image.errors()) {
                fprintf(stderr, "%s\n", err.c_str());
            }
            return Error { ErrorCode::SyntaxError, "Assembler error(s)" };
        }
        TRY(image.write(file_name));
    }
    return ret;
}

}
