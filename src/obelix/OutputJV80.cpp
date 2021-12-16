/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

ErrorOrNode output_jv80(std::shared_ptr<SyntaxNode> const& tree)
{
    using OutputJV80Context = Context<std::string>;
    OutputJV80Context::ProcessorMap output_jv80_map;

    output_jv80_map[SyntaxNodeType::Module] = [](std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx) -> ErrorOrNode {
        auto code = ctx.get(".code").value();
        printf("%s\n", code.c_str());
        return tree;
    };

    output_jv80_map[SyntaxNodeType::BinaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx) -> ErrorOrNode {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        auto code = ctx.get(".code").value();
        code += "    pop d\n"
                "    pop c\n"
                "    pop b\n"
                "    pop a\n";
        switch (expr->op().code()) {
        case TokenCode::Plus:
            code += "    add a,c\n"
                    "    adc b,d\n";
            break;
        case TokenCode::Minus:
            code += "    sub a,c\n"
                    "    sbb b,d\n";
            break;
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr->op().value()));
        }
        code += "    push a\n"
                "    push b\n";
        ctx.set(".code", code);
        return tree;
    };

    output_jv80_map[SyntaxNodeType::Literal] = [](std::shared_ptr<SyntaxNode> const& tree, OutputJV80Context& ctx) -> ErrorOrNode {
        auto literal = std::dynamic_pointer_cast<Literal>(tree);
        auto val_maybe = TRY(literal->to_object());
        assert(val_maybe.has_value());
        auto val = val_maybe.value();
        auto code = ctx.get(".code").value();
        switch (val.type()) {
        case ObelixType::TypeInt: {
            uint16_t v = static_cast<uint16_t>(val->to_long().value());
            code += format("    mov si, ${04x}\n", v);
            code += format("    push si\n");
            break;
        }
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit literals of type {} yet", ObelixType_name(val.type())));
        }
        ctx.set(".code", code);
        return tree;
    };

    OutputJV80Context root(output_jv80_map);
    root.declare(".code", "");
    root.declare(".data", "");
    return process_tree(tree, root);
}

}
