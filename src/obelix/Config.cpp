/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "obelix/Architecture.h"
#include <obelix/Parser.h>

namespace Obelix {

Config::Config(int argc, char const** argv)
{
    for (int ix = 1; ix < argc; ++ix) {
        if ((strlen(argv[ix]) > 2) && !strncmp(argv[ix], "--", 2))
            m_cmdline_flags[std::string(argv[ix]+2)] = true;
        if (!strcmp(argv[ix], "--help")) {
            help = true;
        } else if (!strncmp(argv[ix], "--debug", 7)) {
            if (argv[ix][7] == '=') {
                Obelix::Logger::get_logger().enable(argv[ix] + 8);
            } else {
                Obelix::Logger::get_logger().enable("all");
            }
        } else if (!strcmp(argv[ix], "--show-tree")) {
            show_tree = true;
        } else if (!strcmp(argv[ix], "--parse")) {
            bind = false;
        } else if (!strcmp(argv[ix], "--bind")) {
            lower = false;
        } else if (!strcmp(argv[ix], "--lower")) {
            fold_constants = false;
        } else if (!strcmp(argv[ix], "--fold")) {
            materialize = false;
        } else if (!strcmp(argv[ix], "--materialize")) {
            compile = false;
        } else if (!strcmp(argv[ix], "--run") || !strcmp(argv[ix], "-r")) {
            run = true;
        } else if (!strcmp(argv[ix], "--no-root")) {
            import_root = false;
        } else if (!strncmp(argv[ix], "--arch", strlen("--arch")) && (argv[ix][strlen("--arch")] == '=')) {
            const char *arch = argv[ix] + strlen("--arch=");
            auto target_maybe = Architecture_by_name(arch);
            if (!target_maybe.has_value()) {
                printf("ERROR: Unknown target architecture '%s'", arch);
                exit(-1);
            }
            target = target_maybe.value();
        // } else if (!strncmp(argv[ix], "--", 2) && (strlen(argv[ix]) > 2)) {
        //     printf("ERROR: Unknown command line argument '%s'", argv[ix]);
        //     exit(-1);
        } else if (strncmp(argv[ix], "--", 2) && (filename == "")) {
            filename = argv[ix];
        // } else {
        //     printf("ERROR: Unknown command line argument '%s'\n", argv[ix]);
        //     exit(-1);
        }
    }
}

bool Config::cmdline_flag(std::string const& flag) const
{
    if (m_cmdline_flags.contains(flag))
        return m_cmdline_flags.at(flag);
    return false;
}

}