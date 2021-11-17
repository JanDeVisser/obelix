#include <core/NativeFunction.h>
#include <cstdio>
#include <lexer/Token.h>
#include <obelix/Parser.h>
#include <obelix/Runtime.h>
#include <obelix/Scope.h>
#include <optional>

#include <readline/readline.h>
#include <readline/history.h>


namespace Obelix {

class Repl {
public:
    explicit Repl(Runtime &runtime)
        : m_runtime(runtime)
    {
    }

    enum class ParseResult {
        Execute,
        WantMore,
        Errors
    };

    int run()
    {
        Parser parser(m_runtime.config());
        printf("Obelix 1.99 - A programming language\n");
        bool quit = false;
        bool show_tree = false;
        auto scope = m_runtime.new_scope();
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
                auto tree = parser.parse(m_runtime, buffer);
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
                    if (auto result = tree->execute_in(scope); result.code == ExecutionResultCode::Error) {
                        printf("ERROR: %s\n", result.return_value->to_string().c_str());
                    } else if (result.return_value){
                        printf("%s\n", result.return_value->to_string().c_str());
                    }
                    scope = tree->scope();
                }
            }
        } while (!quit);
	return 0;
    }

private:
    Runtime& m_runtime;
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
    Obelix::Runtime::Config config { false };
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

    auto runtime = Obelix::Runtime(config);

    if (file_name.empty()) {
        return Obelix::Repl(runtime).run();
    }

    auto result = runtime.run(file_name);

    if (auto exit_code = result.return_value; exit_code) {
        if (auto long_maybe = exit_code->to_long(); long_maybe.has_value())
            return (int)long_maybe.value();
    }
    return 0;
}
