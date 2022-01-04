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

    output_jv80_map[SyntaxNodeType::Module] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        return tree;
    };

    output_jv80_map[SyntaxNodeType::Block] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        return tree;
    };

    output_jv80_map[SyntaxNodeType::FunctionParameters] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto parameters = std::dynamic_pointer_cast<FunctionParameters>(tree);
        auto ix = -8;
        for (auto& parameter : parameters->parameters()) {
            ctx.declare(parameter.identifier(), make_obj<Integer>(ix));
            ix -= 2;
        }
        return tree;
    };

    output_jv80_map[SyntaxNodeType::FunctionDecl] = [&image, &code](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto func_decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
        image.add(std::make_shared<Obelix::Assembler::Label>(format("func_{}", func_decl->identifier().identifier()), image.current_address()));
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
        code->add(std::make_shared<Instruction>(Mnemonic::MOV,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp },
            Argument { .addressing_mode = AMRegister, .reg = Register::sp }));
        return tree;
    };

    output_jv80_map[SyntaxNodeType::FunctionDef] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);

        return tree;
    };

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

    output_jv80_map[SyntaxNodeType::FunctionName] = [&code](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto func_name = std::dynamic_pointer_cast<FunctionName>(tree);
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp }));
        code->add(std::make_shared<Instruction>(Mnemonic::MOV,
            Argument { .addressing_mode = AMRegister, .reg = Register::bp },
            Argument { .addressing_mode = AMRegister, .reg = Register::sp }));
        return tree;
    };

    output_jv80_map[SyntaxNodeType::FunctionCall] = [&code](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto call = std::dynamic_pointer_cast<FunctionCall>(tree);
        // The arguments will be pushed in reverse order when the argument list was processed.
        // Reserve spot for return code on stack (will be bp[-4])
        code->add(std::make_shared<Instruction>(Mnemonic::PUSHW,
            Argument { .addressing_mode = AMImmediate, .immediate_type = Argument::ImmediateType::Constant, .constant = 0 }));

        // Call function:
        auto function = call->function()->identifier().identifier();
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
        return tree;
    };

    output_jv80_map[SyntaxNodeType::BinaryExpression] = [&code](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .addressing_mode = AMRegister, .reg = Register::cd }));
        code->add(std::make_shared<Instruction>(Mnemonic::POP, Argument { .addressing_mode = AMRegister, .reg = Register::ab }));
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
        code->add(std::make_shared<Instruction>(Mnemonic::PUSH, Argument { .addressing_mode = AMRegister, .reg = Register::ab }));
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
            Mnemonic::PUSH,
            Argument {
                .addressing_mode = Assembler::AMIndexed,
                .constant = (uint16_t)idx->to_long().value(),
                .reg = Register::bp,
            }));
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

    output_jv80_map[SyntaxNodeType::Return] = [&code](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
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
        image.list(true);
        TRY(image.write(file_name));
    }
    return ret;
}

}
