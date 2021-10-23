#include <core/NativeFunction.h>
#include <cstdio>
#include <lexer/Token.h>
#include <obelix/Parser.h>
#include <obelix/Scope.h>
#include <optional>
#include <vector>

namespace Obelix {

void seed_global_scope(Obelix::Scope& global_scope)
{
    Resolver::get_resolver().open("");
    global_scope.declare("print", Obelix::make_obj<Obelix::NativeFunction>("oblfunc_print"));
    global_scope.declare("format", Obelix::make_obj<Obelix::NativeFunction>("oblfunc_format"));
}

}

void usage()
{
    printf(
        "Obelix v2 - A programming language\n"
        "USAGE:\n"
        "    obelix [--debug] path/to/script.obl\n");
    exit(1);
}


int main(int argc, char** argv)
{
    std::string file_name;
    for (int ix = 1; ix < argc; ++ix) {
        if (!strcmp(argv[ix], "--help")) {
            usage();
        } else if (!strcmp(argv[ix], "--debug")) {
            Obelix::Logger::get_logger().enable("lexer");
            Obelix::Logger::get_logger().enable("resolve");
        } else {
            file_name = argv[ix];
        }
    }

    if (file_name.empty()) {
        usage();
    }

    Obelix::Parser parser(file_name);
    auto tree = parser.parse();
    if (!tree || parser.has_errors()) {
        for (auto& error : parser.errors()) {
            fprintf(stderr, "%s\n", error.to_string().c_str());
        }
        return 1;
    }
    Obelix::Scope global_scope;
    seed_global_scope(global_scope);
    auto result = tree->execute(global_scope);
    if (auto exit_code = result.return_value; exit_code) {
        if (auto long_maybe = exit_code.to_long(); long_maybe.has_value())
            return (int)long_maybe.value();
    }
    return 0;
}
