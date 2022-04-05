/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#include <core/FileBuffer.h>
#include <core/Logging.h>

namespace Obelix {

FileBuffer::FileBuffer(std::string file_name)
    : m_file_name(move(file_name))
    , m_buffer(std::make_unique<StringBuffer>())
{
    auto fh = ::open(m_file_name.c_str(), O_RDONLY);
    if (fh < 0) {
        switch (errno) {
        case ENOENT:
            m_error = Error<int> { ErrorCode::NoSuchFile, ENOENT, m_file_name };
            break;
        default:
            m_error = Error<int> { ErrorCode::IOError, errno, format("Error opening file '{}': {}", m_file_name, strerror(errno)) };
            break;
        }
        return;
    }
    struct stat sb;
    if (auto rc = fstat(fh, &sb); rc < 0) {
        m_error = Error<int> { ErrorCode::IOError, errno, format("Error stat-ing file '{}': {}", m_file_name, strerror(errno)) };
        close(fh);
        return;
    }
    if (S_ISDIR(sb.st_mode)) {
        m_error = Error<int> { ErrorCode::PathIsDirectory, m_file_name };
        close(fh);
        return;
    }

    auto size = sb.st_size;
    auto buf = new char[size + 1];
    if (auto rc = ::read(fh, (void*)buf, size); rc < size) {
        m_error = Error<int> { ErrorCode::IOError, errno, format("Error reading '{}': {}", m_file_name, strerror(errno)) };
    } else {
        buf[size] = '\0';
        m_buffer->assign(buf);
    }
    close(fh);
    delete[] buf;
}

}
