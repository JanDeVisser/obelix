/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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


    auto ret_or_error = Obelix::compile_project(config, config.filename);
    if (ret_or_error.is_error()) {
        auto err = ret_or_error.error();
        std::cerr << "ERROR: " << err.payload().location.to_string() << " " << ret_or_error.error().message() << "\n";
        return -1;
    }
    return 0;
}
