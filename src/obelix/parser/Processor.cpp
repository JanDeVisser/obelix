/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>

#include <obelix/Processor.h>
#include <obelix/Syntax.h>
#include <obelix/arm64/ARM64.h>
#include <obelix/parser/Parser.h>
#include <obelix/transpile/c/CTranspiler.h>

#ifdef JV80
#    include <cpu/emulator.h>
#    include <obelix/OutputJV80.h>
#endif

namespace Obelix {

logging_category(processor);

std::string sanitize_module_name(std::string const& unsanitized)
{
    auto ret = to_lower(unsanitized);
    if (ret.ends_with(".obl"))
        ret = ret.substr(0, ret.length() - 4);
    if (ret.starts_with("./"))
        ret = ret.substr(2);
    return ret;
}

ProcessResult parse(ParserContext& ctx, std::string const& module_name)
{
    Parser parser(ctx, module_name);
    ProcessResult ret;
    ret = parser.parse();
    for (auto const& e : parser.errors())
        ret = SyntaxError { ErrorCode::SyntaxError, e.token, e.message };
    return ret;
}

#define PROCESS_RESULT(res)        \
    ({                             \
        auto __result = res;       \
        if (__result.is_error()) { \
            return __result;       \
        }                          \
        __result.value();          \
    })

#define TRY_PROCESS_RESULT(tree, ctx)       \
    ({                                      \
        auto __result = process(tree, ctx); \
        if (__result.is_error()) {          \
            return __result;                \
        }                                   \
        __result.value();                   \
    })

ProcessResult compile_project(Config const& config)
{
    auto compilation = std::make_shared<Compilation>(config.main());
    ParserContext ctx { config };

    auto tree = TRY_PROCESS_RESULT(compilation, ctx);
    if (config.cmdline_flag<bool>("show-tree"))
        std::cout << "\n\nOriginal:\n"
                  << tree->to_xml() << "\n";
    if (!config.bind)
        return tree;

    auto transformed = PROCESS_RESULT(bind_types(tree, config));
    if (!config.lower)
        return transformed;
    transformed = PROCESS_RESULT(lower(transformed, config));
    if (config.cmdline_flag<bool>("show-tree"))
        std::cout << "\n\nFlattened:\n"
                  << transformed->to_xml() << "\n";
    if (!config.fold_constants)
        return transformed;
    transformed = PROCESS_RESULT(fold_constants(transformed));
    if (config.cmdline_flag<bool>("show-tree"))
        std::cout << "\n\nConstants folded:\n"
                  << transformed->to_xml() << "\n";

    if (!config.compile)
        return transformed;
    switch (config.target) {
    case Architecture::MACOS_ARM64: {
        auto ret = PROCESS_RESULT(output_arm64(transformed, config));
        if (ret->node_type() != SyntaxNodeType::BoundIntLiteral)
            ret = std::make_shared<BoundIntLiteral>(Token {}, 0);
        return ret;
    }
    case Architecture::C_TRANSPILER: {
        auto ret = PROCESS_RESULT(transpile_to_c(transformed, config));
        if (ret->node_type() != SyntaxNodeType::BoundIntLiteral)
            ret = std::make_shared<BoundIntLiteral>(Token {}, 0);
        return ret;
    }
#ifdef JV80
    case Architecture::JV_80:
        auto image = file_name + ".bin";
        if (auto err = output_jv80(transformed, image); err.is_error())
            return err;
        Obelix::CPU::CPU cpu(image);
        auto ret = cpu.run(config().show_tree);
        if (ret.is_error())
            return Error { ErrorCode::SyntaxError, format("Runtime error: {}", SystemErrorCode_name(ret.error())) };
        return std::make_shared<Literal>(make_obj<Integer>(ret.value()));
#endif
    default:
        fatal("Unsupported target architecture {}", config.target);
    }
}

INIT_NODE_PROCESSOR(ParserContext)

NODE_PROCESSOR(Compilation)
{
    auto compilation = std::dynamic_pointer_cast<Compilation>(tree);
    Modules modules = compilation->modules();
    auto main_done = std::any_of(modules.begin(), modules.end(), [&compilation](auto const& m) {
        return m->name() == compilation->main_module();
    });
    if (!main_done) {
        auto res = parse(ctx, compilation->main_module());
        result += res;
        if (result.is_error())
            return result.error();
        modules.push_back(std::dynamic_pointer_cast<Module>(res.value()));
        while (!ctx.modules.empty()) {
            Strings module_names;
            for (auto const& m : ctx.modules) {
                module_names.push_back(m);
            }
            ctx.modules.clear();
            for (auto const& m : module_names) {
                res = parse(ctx, m);
                if (res.has_value())
                    modules.push_back(std::dynamic_pointer_cast<Module>(res.value()));
                result += res;
            }
        }
    }
    if (ctx.config.import_root) {
        auto root_done = std::any_of(modules.begin(), modules.end(), [](auto const& m) {
            return m->name() == "/";
        });
        if (!root_done) {
            auto res = parse(ctx, "/");
            if (res.has_value())
                modules.push_back(std::dynamic_pointer_cast<Module>(res.value()));
            result += res;
        }
    }
    return std::make_shared<Compilation>(modules, compilation->main_module());
}

}
