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

class OutputJV80Context : public Context<Obj> {
public:
    OutputJV80Context(OutputJV80Context& parent)
        : Context<Obj>(parent)
    {
        auto offset_maybe = get("#offset");
        assert(offset_maybe.has_value());
        declare("#offset", get("#offset").value());
    }

    explicit OutputJV80Context(OutputJV80Context* parent)
        : Context<Obj>(parent)
    {
        auto offset_maybe = get("#offset");
        assert(offset_maybe.has_value());
        declare("#offset", get("#offset").value());
    }

    explicit OutputJV80Context(ProcessorMap const& map)
        : Context(map)
    {
        declare("#offset", make_obj<Integer>(0));
    }
};

ErrorOrNode output_jv80(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name)
{
    using namespace Obelix::Assembler;

    OutputJV80Context::ProcessorMap output_jv80_map;
    Image image(0xC000);
    auto code = image.get_segment(0);
    //    auto data = std::make_shared<Segment>(0xC000);
    //    image.add(data);

    code->add(std::make_shared<Instruction>(Mnemonic::MOV,
        Argument { .addressing_mode = AMRegister, .reg = Register::sp },
        Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Constant, .constant = 0xc000 }));
    code->add(std::make_shared<Instruction>(Mnemonic::MOV,
        Argument { .addressing_mode = AMRegister, .reg = Register::bp },
        Argument { .addressing_mode = AMRegister, .reg = Register::sp }));

    output_jv80_map[SyntaxNodeType::Module] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        return tree;
    };

    output_jv80_map[SyntaxNodeType::Block] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        return tree;
    };

    output_jv80_map[SyntaxNodeType::BinaryExpression] = [&code](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .addressing_mode = AMRegister, .reg = Register::d }));
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .addressing_mode = AMRegister, .reg = Register::c }));
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .addressing_mode = AMRegister, .reg = Register::b }));
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .addressing_mode = AMRegister, .reg = Register::a }));
        switch (expr->op().code()) {
        case TokenCode::Plus:
            code->add(std::make_shared<Instruction>(Mnemonic::ADD,
                Argument { .addressing_mode = AMRegister, .reg = Register::ab },
                Argument { .addressing_mode = AMRegister, .reg = Register::cd }));
            break;
        case TokenCode::Minus:
            code->add(std::make_shared<Instruction>(Mnemonic::SUB,
                Argument { .addressing_mode = AMRegister, .reg = Register::ab },
                Argument { .addressing_mode = AMRegister, .reg = Register::cd }));
            break;
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr->op().value()));
        }
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .addressing_mode = AMRegister, .reg = Register::a }));
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .addressing_mode = AMRegister, .reg = Register::b }));
        return tree;
    };

    output_jv80_map[SyntaxNodeType::Literal] = [&code](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto literal = std::dynamic_pointer_cast<Literal>(tree);
        auto val_maybe = TRY(literal->to_object());
        assert(val_maybe.has_value());
        auto val = val_maybe.value();
        switch (val.type()) {
        case ObelixType::TypeInt: {
            uint16_t v = static_cast<uint16_t>(val->to_long().value());
            code->add(std::make_shared<Instruction>(Mnemonic::PUSHW,
                Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Constant, .constant = v }));
            break;
        }
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit literals of type {} yet", ObelixType_name(val.type())));
        }
        return tree;
    };

    output_jv80_map[SyntaxNodeType::Identifier] = [&code](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto identifier = std::dynamic_pointer_cast<Identifier>(tree);
        auto idx_maybe = ctx.get(identifier->name());
        assert(idx_maybe.has_value());
        auto idx = idx_maybe.value();
        code->add(std::make_shared<Instruction>(
            Mnemonic::MOV,
            Argument { .addressing_mode = AMRegister, .reg = Register::si },
            Argument {
                .addressing_mode = Assembler::AMIndexed,
                .constant = (uint16_t)idx->to_long().value(),
                .reg = Register::bp,
            }));
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .addressing_mode = AMRegister, .reg = Register::si }));
        return tree;
    };

    output_jv80_map[SyntaxNodeType::VariableDeclaration] = [&code](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto offset = (signed char)ctx.get("#offset").value()->to_long().value();
        ctx.set("#offset", make_obj<Integer>(offset + 2));
        ctx.declare(var_decl->variable().identifier(), make_obj<Integer>(offset));
        if (var_decl->expression() == nullptr) {
            code->add(std::make_shared<Instruction>(Mnemonic::PUSHW, Argument { .addressing_mode = AMImmediate, .constant = 0 }));
        }
        return tree;
    };

    output_jv80_map[SyntaxNodeType::Return] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        return tree;
    };

    OutputJV80Context root(output_jv80_map);

    auto ret = process_tree(tree, root);

    code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .addressing_mode = AMRegister, .reg = Register::di }));
    code->add(std::make_shared<Instruction>(Mnemonic::HLT));

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
