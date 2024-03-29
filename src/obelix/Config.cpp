/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <filesystem>
#include <iostream>

#include <core/Logging.h>
#include <obelix/Config.h>

namespace Obelix {

logging_category(config);

namespace fs = std::filesystem;

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
                std::cerr << "ERROR: Unknown target architecture '" << arch << "'\n";
                exit(-1);
            }
            target = target_maybe.value();
        } else if (!strncmp(argv[ix], "--obelix-dir", strlen("--obelix-dir")) && (argv[ix][strlen("--obelix-dir")] == '=')) {
            obelix_dir = argv[ix] + strlen("--obelix-dir=");
        } else if (strncmp(argv[ix], "--", 2) && filename.empty()) {
            filename = argv[ix];
        }
    }
    if (filename.empty())
        help = true;
}

std::string Config::obelix_directory() const
{
    std::string obl_dir = (getenv("OBL_DIR")) ? getenv("OBL_DIR") : OBELIX_DIR;
    if (!obelix_dir.empty())
        obl_dir = obelix_dir;
    return obl_dir;
}

std::string Config::base_directory() const
{
    assert(!filename.empty());
    fs::path path = filename;
    fs::path parent_path = path.parent_path();
    if (parent_path.empty()) {
        parent_path = fs::current_path();
    } else {
        parent_path = fs::weakly_canonical(parent_path);
    }
    return parent_path.string();
}

std::string Config::main() const
{
    assert(!filename.empty());
    fs::path path = filename;
    return path.filename().stem().string();
}

ErrorOr<std::string,SystemError> ObelixBufferLocator::check_in_dir(std::string const& directory, std::string const& file_path)
{
    debug(config, "Checking existence of dir {} file {}", directory, file_path);
    auto exists_in_dir = check_existence(format("{}/{}", directory, file_path));
    if (exists_in_dir.is_error()) {
        auto err = exists_in_dir.error();
        switch (err.code()) {
        case ErrorCode::PathIsDirectory: {
            debug(config, "Path is directory");
            return check_in_dir(directory, "__init__.obl");
        }
        case ErrorCode::NoSuchFile: {
            if (file_path.ends_with(".obl")) {
                debug(config, "Path does not exist");
                return err;
            }
            return check_in_dir(directory, file_path + ".obl");
        }
        default:
            debug(config, "Unexpected error: {}", err);
            return err;
        }
    }

    // File exists. If path doesn't end with ".obl" it's probably the executable
    // generated by a previous run:
    if (!file_path.ends_with(".obl")) {
        debug(config, "Path exists but is not an '.obl' file");
        return check_in_dir(directory, file_path + ".obl");
    }
    debug(config, "File located: {}/{}", directory, file_path);
    return directory + "/" + file_path;
}

ErrorOr<std::string,SystemError> ObelixBufferLocator::locate(std::string const& file) const
{
    std::string obl_dir = m_config.obelix_directory();
    debug(config, "Locating file '{}' with OBL_DIR={}", file, obl_dir);

    std::string path;
    if (file == "/") {
        path = "__init__.obl";
    } else {
        path = file;
    }

    if (auto path_or_err = check_in_dir(m_config.base_directory(), path); !path_or_err.is_error()) {
        return path_or_err.value();
    } else if (path_or_err.error().code() != ErrorCode::NoSuchFile) {
        return path_or_err.error();
    }

    fs::path dir = fs::canonical(".");
    do {
        auto share_path = std::filesystem::weakly_canonical("./share");
        if (auto share_exists = check_existence(share_path.string()); share_exists.is_error()) {
            if (share_exists.error().code() == ErrorCode::NoSuchFile) {
                if (dir != dir.parent_path()) {
                    dir = dir.parent_path();
                    continue;
                }
                break;
            }
            return share_exists.error();
        }
        if (auto path_or_err = check_in_dir(share_path.string(), path); !path_or_err.is_error()) {
            return path_or_err.value();
        } else {
            return path_or_err.error();
        }
    } while (true);

    if (obl_dir.empty()) {
        fatal("No obelix directory specified!");
    }
    if (auto share_exists = check_existence(format("{}/share", obl_dir)); share_exists.is_error() && share_exists.error().code() != ErrorCode::PathIsDirectory) {
        fatal("Obelix directory '{}' has no 'share' subdirectory", obl_dir);
    }
    if (auto path_or_err = check_in_dir(obl_dir + "/share", path); !path_or_err.is_error()) {
        return path_or_err.value();
    } else {
        return path_or_err.error();
    }

}

}
