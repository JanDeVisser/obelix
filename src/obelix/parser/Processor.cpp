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
    auto parser_or_error = Parser::create(ctx, module_name);
    if (parser_or_error.is_error()) {
        return SyntaxError { parser_or_error.error().message() };
    }
    auto parser = parser_or_error.value();
    ProcessResult ret;
    ret = parser->parse();
    for (auto const& e : parser->errors())
        ret.error(e);
    return ret;
}

#define PROCESS_RESULT(res)        \
    ({                             \
        auto __result = res;       \
        if (result.is_error()) { \
            return __result;       \
        }                          \
    })

#define TRY_PROCESS_RESULT(tree, ctx)               \
    ({                                              \
        process(tree, ctx, result); \
        if (__result.is_error()) {                  \
            return __result;                        \
        }                                           \
    })

ProcessResult compile_project(Config const& config)
{
    ParserContext ctx { config };

    ProcessResult result;
    result = std::make_shared<Compilation>(config.main());
    process(result.value(), ctx, result);
    if (result.is_error())
        return result;
    if (config.cmdline_flag<bool>("show-tree"))
        std::cout << "\n\nOriginal:\n"
                  << result.value()->to_xml() << "\n";
    if (!config.bind)
        return result;

    bind_types(config, result);
    if (result.is_error() || !config.lower)
        return result;

    lower(config, result);
    if (result.is_error())
        return result;
    if (config.cmdline_flag<bool>("show-tree"))
        std::cout << "\n\nFlattened:\n"
                  << result.value()->to_xml() << "\n";
    if (!config.fold_constants)
        return result;

    fold_constants(result);
    if (result.is_error())
        return result;
    if (config.cmdline_flag<bool>("show-tree"))
        std::cout << "\n\nConstants folded:\n"
                  << result.value()->to_xml() << "\n";
    if (!config.compile)
        return result;

    switch (config.target) {
    case Architecture::MACOS_ARM64: {
        output_arm64(result, config);
        if (result.is_error())
            return result;
        if (result.value() == nullptr || result.value()->node_type() != SyntaxNodeType::BoundIntLiteral)
            result = std::make_shared<BoundIntLiteral>(Span {}, 0);
        return result;
    }
    case Architecture::C_TRANSPILER: {
        transpile_to_c(result, config);
        if (result.value() == nullptr || result.value()->node_type() != SyntaxNodeType::BoundIntLiteral)
            result = std::make_shared<BoundIntLiteral>(Span {}, 0);
        return result;
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
