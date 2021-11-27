/*
 * main.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <cstdio>
#include <lexer/Token.h>
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
                    printf("%s\n", eval_result.value()->to_string(0).c_str());
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
        auto tree = std::dynamic_pointer_cast<Module>(parser.parse());
        if (!tree) {
            for (auto& e : parser.errors()) {
                printf("ERROR: %s\n", e.to_string().c_str());
            }
            return Error { ErrorCode::SyntaxError, parser.errors()[0].message };
        }
        if (auto eval_result = evaluate_tree(tree); eval_result.is_error()) {
            printf("ERROR: %s\n", eval_result.error().message().c_str());
        } else {
            printf("%s\n", eval_result.value()->to_string(0).c_str());
        }
        return 0;
    }

    [[nodiscard]] Config& config() { return m_config; }

private:
    ErrorOr<std::shared_ptr<SyntaxNode>> evaluate_tree(std::shared_ptr<SyntaxNode> const& tree)
    {
        if (tree) {
            if (config().show_tree)
                printf("%s\n", tree->to_string(0).c_str());
            auto transformed = TRY(bind_types(tree));
            if (config().show_tree)
                printf("%s\n", transformed->to_string(0).c_str());
            transformed = TRY(fold_constants(transformed));
            if (config().show_tree)
                printf("%s\n", transformed->to_string(0).c_str());
            Context<Obj> root;
            return execute(transformed, root);
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
        } else if (!strcmp(argv[ix], "--debug")) {
            Obelix::Logger::get_logger().enable("all");
        } else if (!strcmp(argv[ix], "--show-tree")) {
            config.show_tree = true;
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
