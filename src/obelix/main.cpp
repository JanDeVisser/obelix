#include <core/NativeFunction.h>
#include <cstdio>
#include <lexer/Token.h>
#include <obelix/Parser.h>
#include <obelix/Runtime.h>
#include <obelix/Scope.h>
#include <optional>
#include <vector>

namespace Obelix {


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
            //            Obelix::Logger::get_logger().enable("lexer");
            Obelix::Logger::get_logger().enable("resolve");
            Obelix::Logger::get_logger().enable("filebuffer");
        } else {
            file_name = argv[ix];
        }
    }

    if (file_name.empty()) {
        usage();
    }

    auto runtime = Obelix::Runtime();
    auto result = runtime.run(file_name);

    if (auto exit_code = result.return_value; exit_code) {
        if (auto long_maybe = exit_code.to_long(); long_maybe.has_value())
            return (int)long_maybe.value();
    }
    return 0;
}
