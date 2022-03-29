/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>

#include <cpu/emulator.h>
#include <lexer/Token.h>
#ifdef JV80
#include <obelix/OutputJV80.h>
#endif
#include <obelix/ARM64.h>
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

    ErrorOr<int> run(std::string const& file_name)
    {
        std::string fname = file_name;
        for (auto ix = 0u; ix < fname.length(); ++ix) {
            if ((fname[ix] == '.') && (fname.substr(ix) != ".obl"))
                fname[ix] = '/';
        }
        Parser parser(config(), fname);
        if (!parser.buffer_read()) {
            assert(!parser.errors().empty());
            return Error { ErrorCode::NoSuchFile, fname };
        }
        auto tree = std::dynamic_pointer_cast<Compilation>(parser.parse());
        if (!tree) {
            for (auto& e : parser.errors()) {
                printf("ERROR: %s\n", e.to_string().c_str());
            }
            return Error { ErrorCode::SyntaxError, parser.errors()[0].message };
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
    ErrorOr<std::shared_ptr<SyntaxNode>> evaluate_tree(std::shared_ptr<SyntaxNode> const& tree, std::string file_name = "")
    {
        if (tree) {
            if (config().show_tree)
                std::cout << "\n\nOriginal:\n" << tree->to_xml() << "\n";
            auto transformed = TRY(bind_types(tree));
            if (config().show_tree)
                std::cout << "\n\nTypes bound:\n" << transformed->to_xml() << "\n";
            transformed = TRY(lower(transformed));
            if (config().show_tree)
                std::cout << "\n\nFlattened:\n" << transformed->to_xml() << "\n";
            transformed = TRY(fold_constants(transformed));
            if (config().show_tree)
                std::cout << "\n\nConstants folded:\n" << transformed->to_xml() << "\n";
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
#define MACOSX
#ifdef MACOSX
                auto err = output_arm64(transformed, file_name, config().run);
                if (err.is_error())
                    return err;
                auto ret_val = err.value();
                if (ret_val->node_type() != SyntaxNodeType::BoundLiteral)
                    ret_val = std::make_shared<BoundLiteral>(Token {}, make_obj<Integer>(0));
                return ret_val;
#endif
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

int main(int argc, char** argv)
{
    std::string file_name;
    Obelix::Config config { false };
    for (int ix = 1; ix < argc; ++ix) {
        if (!strcmp(argv[ix], "--help")) {
            usage();
        } else if (!strncmp(argv[ix], "--debug", 7)) {
            if (argv[ix][7] == '=') {
                Obelix::Logger::get_logger().enable(argv[ix] + 8);
            } else {
                Obelix::Logger::get_logger().enable("all");
            }
        } else if (!strcmp(argv[ix], "--show-tree")) {
            config.show_tree = true;
        } else if (!strcmp(argv[ix], "--run") || !strcmp(argv[ix], "-r")) {
            config.run = true;
        } else {
            file_name = argv[ix];
        }
    }

    Obelix::Repl repl(config);
    auto ret_or_error = (file_name.empty()) ? repl.repl() : repl.run(file_name);
    if (ret_or_error.is_error()) {
        printf("ERROR: %s\n", ret_or_error.error().message().c_str());
        return -1;
    }
    return ret_or_error.value();
}
