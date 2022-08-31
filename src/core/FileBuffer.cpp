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
#include <core/ScopeGuard.h>

namespace Obelix {

logging_category(filebuffer);

ErrorOr<void> BufferLocator::check_existence(std::string const& file_name)
{
    auto fh = ::open(file_name.c_str(), O_RDONLY);
    auto sg = ScopeGuard([fh]() { if (fh > 0) ::close(fh); });
    if (fh < 0) {
        switch (errno) {
        case ENOENT:
            return Error<int> { ErrorCode::NoSuchFile, ENOENT, file_name };
        default:
            return Error<int> { ErrorCode::IOError, errno, format("Error opening file '{}': {}", file_name, strerror(errno)) };
        }
    }

    struct stat sb;
    if (auto rc = fstat(fh, &sb); rc < 0)
        return Error<int> { ErrorCode::IOError, errno, format("Error stat-ing file '{}': {}", file_name, strerror(errno)) };
    if (S_ISDIR(sb.st_mode))
        return Error<int> { ErrorCode::PathIsDirectory, file_name };
    return {};
}

ErrorOr<std::string> SimpleBufferLocator::locate(std::string const& file_name) const
{
    auto exists = check_existence(file_name);
    if (exists.is_error())
        return exists.error();
    return file_name;
}

FileBuffer::FileBuffer(std::string const& file_name, BufferLocator* locator)
    : m_buffer(std::make_unique<StringBuffer>())
{
    debug(filebuffer, "Going to read {}", file_name);
    m_buffer_locator.reset((locator != nullptr) ? locator : new SimpleBufferLocator());
    auto file_name_or_error = m_buffer_locator->locate(file_name);
    if (file_name_or_error.is_error()) {
        m_error = file_name_or_error.error();
        return;
    }

    m_path = file_name_or_error.value();
    auto fh = ::open(file_path().c_str(), O_RDONLY);
    auto file_closer = ScopeGuard([fh]() {
        if (fh > 0)
            close(fh);
    });
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
        return;
    }
    if (S_ISDIR(sb.st_mode)) {
        m_error = Error<int> { ErrorCode::PathIsDirectory, m_file_name };
        return;
    }

    auto size = sb.st_size;
    auto buf = new char[size + 1];
    auto buffer_deleter = ScopeGuard([buf]() {
        delete[] buf;
    });
    if (auto rc = ::read(fh, (void*)buf, size); rc < size) {
        m_error = Error<int> { ErrorCode::IOError, errno, format("Error reading '{}': {}", m_file_name, strerror(errno)) };
    } else {
        buf[size] = '\0';
        m_buffer->assign(buf);
    }
}

std::string FileBuffer::file_name() const
{
    return m_path.filename().string();
}

std::string FileBuffer::dir_name() const
{
    if (m_path.has_parent_path())
        return m_path.parent_path().string();
    return ".";
}

std::string FileBuffer::file_path() const
{
    return m_path.string();
}

}
