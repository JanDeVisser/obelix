/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <unistd.h>

#include <config.h>
#include <core/Logging.h>
#include <lexer/OblBuffer.h>

namespace Obelix {

extern_logging_category(lexer);

OblBuffer::OblBuffer(std::string file_name)
    : m_file_name(move(file_name))
{
    char cwd[1024];
    getcwd(cwd, 1024);

    std::string obl_dir = (getenv("OBL_DIR")) ? getenv("OBL_DIR") : OBELIX_DIR;
    debug(lexer, "Loading file {}. OBL_DIR= {}", this->file_name(), obl_dir);

    if (auto err = try_open("."); !err.is_error() || err.error().code() != ErrorCode::NoSuchFile)
        return;
    if (auto err = try_open(obl_dir + "/share"); !err.is_error() || err.error().code() != ErrorCode::NoSuchFile)
        return;
    auto err = try_open("./share");
}

StringBuffer& OblBuffer::buffer()
{
    assert(file_is_read());
    return *m_buffer;
}

ErrorOr<void> OblBuffer::try_open(std::string const& directory, std::optional<std::string> const& alternative_file_name)
{
    assert(!directory.empty());
    auto f = file_name();
    if (alternative_file_name.has_value())
        f = alternative_file_name.value();
    auto path = directory + "/" + f;
    debug(lexer, "Attempting {}", path);

    m_error = Error<int> { ErrorCode::NoError };
    FileBuffer fb(path);
    if (fb.file_is_read()) {
        m_buffer = std::move(fb.buffer());
        m_dir_name = directory;
        m_effective_file_name = f;
        debug(lexer, "Success");
        return {};
    }
    switch (fb.error().code()) {
    case ErrorCode::PathIsDirectory: {
        debug(lexer, "Path is directory");
        return try_open(directory, "__init__.obl");
    }
    case ErrorCode::NoSuchFile: {
        if (path.ends_with(".obl")) {
            m_error = fb.error();
            debug(lexer, "Path does not exist");
            return m_error;
        }
        return try_open(directory, f + ".obl");
    }
    default: {
        m_error = fb.error();
        log_error("I/O Error opening '{}': {}", path, m_error);
        return m_error;
    }
    }
}

std::string const& OblBuffer::file_name() const
{
    return m_file_name;
}

std::string const& OblBuffer::dir_name() const
{
    return m_dir_name;
}

std::string const& OblBuffer::effective_file_name() const
{
    return m_effective_file_name;
}

}
