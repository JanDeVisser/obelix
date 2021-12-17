/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include <config.h>
#include <core/Logging.h>
#include <lexer/OblBuffer.h>

namespace Obelix {

OblBuffer::OblBuffer(std::string file_name)
    : m_file_name(move(file_name))
{
    char cwd[1024];
    getcwd(cwd, 1024);

    std::string obl_dir = (getenv("OBL_DIR")) ? getenv("OBL_DIR") : OBELIX_DIR;

    auto opened = try_open(m_file_name.empty() ? "." : m_file_name);
    if (!opened && (m_error.code() == ErrorCode::NoSuchFile))
        opened = try_open(obl_dir + "/share/" + m_file_name);
    if (!opened && (m_error.code() == ErrorCode::NoSuchFile))
        opened = try_open("./share/" + m_file_name);
    if (!opened && (m_error.code() == ErrorCode::NoSuchFile))
        try_open("./" + m_file_name);
}

StringBuffer& OblBuffer::buffer()
{
    assert(file_is_read());
    return *m_buffer;
}

bool OblBuffer::try_open(std::string const& path)
{
    assert(!path.empty());

    FileBuffer fb(path);
    if (fb.file_is_read()) {
        m_buffer = std::move(fb.buffer());
        return true;
    }
    switch (fb.error().code()) {
    case ErrorCode::PathIsFile:
        return try_open(path + "/__init__.obl");
    case ErrorCode::NoSuchFile: {
        if (path.ends_with(".obl")) {
            m_error = fb.error();
            return false;
        }
        return try_open(path + ".obl");
    }
    default:
        m_error = fb.error();
        return false;
    }
}

}
