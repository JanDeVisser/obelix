/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdio>
#include <optional>

#include <obelix/parser/Parser.h>

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

    auto result = Obelix::compile_project(config);
    for (auto const& e : result.errors()) {
        std::cerr << "ERROR: " << e.location().to_string() << " " << e.message() << "\n";
    }
    for (auto const& e : result.warnings()) {
        std::cerr << "WARNING: " << e.location().to_string() << " " << e.message() << "\n";
    }
    return (result.is_error()) ? -1 : 0;
}
