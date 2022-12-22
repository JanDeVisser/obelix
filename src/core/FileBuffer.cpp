/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#include <core/FileBuffer.h>
#include <core/Logging.h>
#include <core/ScopeGuard.h>

namespace Obelix {

logging_category(filebuffer);

ErrorOr<void, SystemError> BufferLocator::check_existence(std::string const& file_name)
{
    auto fh = ::open(file_name.c_str(), O_RDONLY);
    auto sg = ScopeGuard([fh]() { if (fh > 0) ::close(fh); });
    if (fh < 0) {
        switch (errno) {
        case ENOENT:
            return SystemError { ErrorCode::NoSuchFile, "File '{}' does not exist", file_name };
        default:
            return SystemError { ErrorCode::IOError, "Error opening file '{}'", file_name };
        }
    }

    struct stat sb;
    if (auto rc = fstat(fh, &sb); rc < 0)
        return SystemError { ErrorCode::IOError, "Error stat-ing file '{}'", file_name };
    if (S_ISDIR(sb.st_mode))
        return SystemError { ErrorCode::PathIsDirectory, "Path '{}' is a directory, not a file", file_name };
    return {};
}

ErrorOr<std::string, SystemError> SimpleBufferLocator::locate(std::string const& file_name) const
{
    auto exists = check_existence(file_name);
    if (exists.is_error())
        return exists.error();
    return file_name;
}

FileBuffer::FileBuffer(std::string const& file_name, BufferLocator* locator)
    : m_file_name(file_name)
    , m_buffer(std::make_unique<StringBuffer>())
{
    m_buffer_locator.reset((locator != nullptr) ? locator : new SimpleBufferLocator());
}

ErrorOr<std::shared_ptr<FileBuffer>,SystemError> FileBuffer::create(std::string const& file_name, BufferLocator* locator)
{
    debug(filebuffer, "Going to read {}", file_name);
    auto ret = std::shared_ptr<FileBuffer>(new FileBuffer(file_name, locator));
    auto file_name_or_error = ret->m_buffer_locator->locate(file_name);
    if (file_name_or_error.is_error())
        return file_name_or_error.error();

    ret->m_path = file_name_or_error.value();
    auto fh = ::open(ret->file_path().c_str(), O_RDONLY);
    auto file_closer = ScopeGuard([fh]() {
        if (fh > 0)
            close(fh);
    });
    if (fh < 0) {
        switch (errno) {
        case ENOENT:
            return SystemError { ErrorCode::NoSuchFile, "File '{}' does not exist", ret->file_name() };
        default:
            return SystemError { ErrorCode::IOError, "Error opening file '{}'", ret->file_name() };
        }
    }
    struct stat sb;
    if (auto rc = fstat(fh, &sb); rc < 0)
        return SystemError { ErrorCode::IOError, "Error stat-ing file '{}'", ret->file_name() };
    if (S_ISDIR(sb.st_mode))
        return SystemError { ErrorCode::PathIsDirectory, "Path '{}' is a directory, not a file", ret->file_name() };

    auto size = sb.st_size;
    auto buf = new char[size + 1];
    auto buffer_deleter = ScopeGuard([buf]() {
        delete[] buf;
    });
    if (auto rc = ::read(fh, (void*)buf, size); rc < size) {
        return SystemError { ErrorCode::IOError, "Error reading '{}'", ret->file_name() };
    } else {
        buf[size] = '\0';
        ret->m_buffer->assign(buf);
    }
    return ret;
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
