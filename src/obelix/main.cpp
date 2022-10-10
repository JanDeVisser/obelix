/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "obelix/Architecture.h"
#include "obelix/Context.h"
#include <cstdio>

#include <lexer/Token.h>
#ifdef JV80
#    include <cpu/emulator.h>
#    include <obelix/OutputJV80.h>
#endif
#include <obelix/arm64/ARM64.h>
#include <obelix/transpile/c/CTranspiler.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>
#include <optional>

#include <readline/readline.h>
#include <readline/history.h>

namespace Obelix {

class Repl {
public:
    explicit Repl(Config const& config)
        : m_config(config)
    {
    }

    ErrorOr<int> repl()
    {
        Parser parser(config());
        printf("Obelix 1.99 - A programming language\n");
        bool quit = false;
        std::string buffer;
        do {
            auto line = readline(m_prompt.c_str());
            if (!line || !strcmp(line, "#exit")) {
                quit = true;
            } else if (!strcmp(line, "#show")) {
                config().show_tree = !config().show_tree;
                printf("%sshowing parse results\n", (config().show_tree) ? "" : "Not ");
            } else {
                buffer += line;
                auto tree = parser.parse(buffer);
                if (!tree && (parser.current_code() == TokenCode::EndOfFile)) {
                    m_prompt = ". ";
                    continue;
                }

                buffer = "";
                m_prompt = "> ";
                if (!tree) {
                    for (auto& e : parser.errors()) {
                        printf("ERROR: %s\n", e.to_string().c_str());
                    }
                    continue;
                }
                if (auto eval_result = evaluate_tree(tree); eval_result.is_error()) {
                    printf("ERROR: %s\n", eval_result.error().message().c_str());
                } else {
                    printf("%s\n", eval_result.value()->to_string().c_str());
                }
            }
        } while (!quit);
        return 0;
    }

    ErrorOr<int, SyntaxError> run(std::string const& file_name)
    {
        std::string fname = file_name;
        for (auto ix = 0u; ix < fname.length(); ++ix) {
            if ((fname[ix] == '.') && (fname.substr(ix) != ".obl"))
                fname[ix] = '/';
        }
        Parser parser(config(), fname);
        if (!parser.buffer_read()) {
            assert(!parser.errors().empty());
            return SyntaxError { ErrorCode::NoSuchFile, fname };
        }
        auto tree = std::dynamic_pointer_cast<Compilation>(parser.parse());
        if (!tree) {
            for (auto& e : parser.errors()) {
                printf("ERROR: %s\n", e.to_string().c_str());
            }
            return SyntaxError { ErrorCode::SyntaxError, parser.errors()[0].message };
        }
        if (auto eval_result = evaluate_tree(tree, file_name); eval_result.is_error()) {
            printf("ERROR: %s\n", eval_result.error().message().c_str());
        } else {
            printf("%s\n", eval_result.value()->to_string().c_str());
        }
        return 0;
    }

    [[nodiscard]] Config& config() { return m_config; }

private:
    ErrorOr<std::shared_ptr<SyntaxNode>, SyntaxError> evaluate_tree(std::shared_ptr<SyntaxNode> const& tree, std::string file_name = "")
    {
        if (tree) {
            if (config().cmdline_flag("show-tree"))
                std::cout << "\n\nOriginal:\n" << tree->to_xml() << "\n";
            if (!m_config.bind)
                return tree;
            auto transformed = TRY(bind_types(tree, config()));
            if (!m_config.lower)
                return transformed;
            if (m_config.target == Architecture::C_TRANSPILER) {
                transformed = TRY(lower(transformed));
                if (config().cmdline_flag("show-tree"))
                    std::cout << "\n\nFlattened:\n"
                              << transformed->to_xml() << "\n";
            }
            if (!m_config.fold_constants)
                return transformed;
            transformed = TRY(fold_constants(transformed));
            if (config().cmdline_flag("show-tree"))
                std::cout << "\n\nConstants folded:\n" << transformed->to_xml() << "\n";
            if (!m_config.compile)
                return transformed;
            if (!file_name.empty()) {
#ifdef JV80
                auto image = file_name + ".bin";
                if (auto err = output_jv80(transformed, image); err.is_error())
                    return err;
                Obelix::CPU::CPU cpu(image);
                auto ret = cpu.run(config().show_tree);
                if (ret.is_error())
                    return Error { ErrorCode::SyntaxError, format("Runtime error: {}", SystemErrorCode_name(ret.error())) };
                return std::make_shared<Literal>(make_obj<Integer>(ret.value()));
#endif
                if (m_config.target == Architecture::MACOS_ARM64) {
                    auto err = output_arm64(transformed, config(), file_name);
                    if (err.is_error())
                        return err;
                    auto ret_val = err.value();
                    if (ret_val->node_type() != SyntaxNodeType::BoundIntLiteral)
                        ret_val = std::make_shared<BoundIntLiteral>(Token {}, 0);
                    return ret_val;
                }
                if (m_config.target == Architecture::C_TRANSPILER) {
                    auto err = transpile_to_c(transformed, config(), file_name);
                    if (err.is_error())
                        return err;
                    auto ret_val = err.value();
                    if (ret_val->node_type() != SyntaxNodeType::BoundIntLiteral)
                        ret_val = std::make_shared<BoundIntLiteral>(Token {}, 0);
                    return ret_val;
                }
            } else {
                fprintf(stderr, "Script execution temporarily disabled\n");
                //                Context<Obj> root;
                //                return execute(transformed, root);
            }
            return transformed;
        }
        return tree;
    }

    Config m_config;
    std::string m_prompt { "> " };
};

}

void usage()
{
    printf(
        "Obelix v2 - A programming language\n"
        "USAGE:\n"
        "    obelix [--debug] [--show-tree] path/to/script.obl\n");
    exit(1);
}

int main(int argc, char const** argv)
{
    std::string file_name;
    Obelix::Config config(argc, argv);
    if (config.help) {
        usage();
        exit(-1);
    }

    Obelix::Repl repl(config);

    if (config.filename.empty()) {
        auto ret_or_error = repl.repl();
        if (ret_or_error.is_error()) {
            printf("ERROR: %s\n", ret_or_error.error().message().c_str());
            return -1;
        }
        return ret_or_error.value();
    }

    auto ret_or_error = repl.run(config.filename);
    if (ret_or_error.is_error()) {
        printf("ERROR: %s\n", ret_or_error.error().message().c_str());
        return -1;
    }
    return ret_or_error.value();
}
