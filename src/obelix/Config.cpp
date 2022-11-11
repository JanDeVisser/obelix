/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Logging.h>
#include <obelix/Config.h>

namespace Obelix {

extern_logging_category(parser);

Config::Config(int argc, char const** argv)
{
    for (int ix = 1; ix < argc; ++ix) {
        if ((strlen(argv[ix]) > 2) && !strncmp(argv[ix], "--", 2)) {
            if (auto eq_ptr = strchr(argv[ix], '='); eq_ptr == nullptr) {
                m_cmdline_flags[std::string(argv[ix] + 2)] = true;
            } else {
                auto ptr = argv[ix];
                ptr += 2;
                std::string flag = argv[ix] + 2;
                m_cmdline_flags[flag.substr(0, eq_ptr - ptr)] = std::string(eq_ptr + 1);
            }
        }
        if (!strcmp(argv[ix], "--help")) {
            help = true;
        } else if (!strncmp(argv[ix], "--debug", 7)) {
            if (argv[ix][7] == '=') {
                Logger::get_logger().enable(argv[ix] + 8);
            } else {
                Logger::get_logger().enable("all");
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
            const char* arch = argv[ix] + strlen("--arch=");
            auto target_maybe = Architecture_by_name(arch);
            if (!target_maybe.has_value()) {
                printf("ERROR: Unknown target architecture '%s'", arch);
                exit(-1);
            }
            target = target_maybe.value();
        } else if (!strncmp(argv[ix], "--obelix-dir", strlen("--obelix-dir")) && (argv[ix][strlen("--obelix-dir")] == '=')) {
            obelix_dir = argv[ix] + strlen("--obelix-dir=");
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

ErrorOr<std::string> ObelixBufferLocator::check_in_dir(std::string const& directory, std::string const& file_path)
{
    debug(parser, "Checking existence of dir {} file {}", directory, file_path);
    auto exists_in_dir = check_existence(format("{}/{}", directory, file_path));
    if (exists_in_dir.is_error()) {
        auto err = exists_in_dir.error();
        switch (err.code()) {
        case ErrorCode::PathIsDirectory: {
            debug(parser, "Path is directory");
            return check_in_dir(directory, "__init__.obl");
        }
        case ErrorCode::NoSuchFile: {
            if (file_path.ends_with(".obl")) {
                debug(parser, "Path does not exist");
                return err;
            }
            return check_in_dir(directory, file_path + ".obl");
        }
        default:
            debug(parser, "Unexpected error: {}", err.message());
            return err;
        }
    }
    return directory + "/" + file_path;
}

ErrorOr<std::string> ObelixBufferLocator::locate(std::string const& file_path) const
{
    std::string obl_dir = m_config.obelix_directory();
    debug(parser, "Locating file {}. OBL_DIR= {}", file_path, obl_dir);

    if (auto path_or_err = check_in_dir(".", file_path); !path_or_err.is_error()) {
        return path_or_err.value();
    } else if (path_or_err.error().code() != ErrorCode::NoSuchFile) {
        return path_or_err.error();
    }

    if (auto path_or_err = check_in_dir(obl_dir + "/share", file_path); !path_or_err.is_error()) {
        return path_or_err.value();
    } else if (path_or_err.error().code() != ErrorCode::NoSuchFile) {
        return path_or_err.error();
    }

    if (auto path_or_err = check_in_dir("./share", file_path); !path_or_err.is_error()) {
        return path_or_err.value();
    } else {
        return path_or_err.error();
    }
}

}
