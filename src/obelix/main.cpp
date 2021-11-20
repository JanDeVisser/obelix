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
#include <obelix/Runtime.h>
#include <obelix/Scope.h>
#include <optional>

#include <readline/readline.h>
#include <readline/history.h>


namespace Obelix {

class Repl {
public:
    explicit Repl(Ptr<Runtime> runtime)
        : m_runtime(std::move(runtime))
    {
    }

    enum class ParseResult {
        Execute,
        WantMore,
        Errors
    };

    int run()
    {
        Parser parser(m_runtime->config());
        printf("Obelix 1.99 - A programming language\n");
        bool quit = false;
        bool show_tree = false;
        std::string buffer;
        do {
            auto line = readline(m_prompt.c_str());
            if (!line || !strcmp(line, "#exit")) {
                quit = true;
            } else if (!strcmp(line, "#show")) {
                show_tree = !show_tree;
                printf("%sshowing parse results\n", (show_tree) ? "" : "Not ");
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

                if (tree) {
                    if (show_tree)
                        printf("%s\n", tree->to_string(0).c_str());
                    auto transformed = fold_constants(tree);
                    if (show_tree)
                        printf("%s\n", transformed->to_string(0).c_str());
                    if (auto result = std::dynamic_pointer_cast<Module>(transformed)->execute_in(m_runtime->as_scope()); result.code == ExecutionResultCode::Error) {
                        printf("ERROR: %s\n", result.return_value->to_string().c_str());
                    } else if (result.return_value){
                        printf("%s\n", result.return_value->to_string().c_str());
                    }
                }
            }
        } while (!quit);
	return 0;
    }

private:
    Ptr<Runtime> m_runtime;
    std::string m_prompt { "> "};
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

    auto runtime = Obelix::make_typed<Obelix::Runtime>(config);

    if (file_name.empty()) {
        return Obelix::Repl(runtime).run();
    }

    auto result = runtime->run(file_name);

    if (auto exit_code = result.return_value; exit_code) {
        if (auto long_maybe = exit_code->to_long(); long_maybe.has_value())
            return (int)long_maybe.value();
    }
    return 0;
}
