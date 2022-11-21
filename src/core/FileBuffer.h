/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <filesystem>
#include <memory>
#include <sys/stat.h>

#include <core/Error.h>
#include <core/StringBuffer.h>

namespace Obelix {

class BufferLocator {
public:
    virtual ~BufferLocator() = default;
    [[nodiscard]] virtual ErrorOr<std::string> locate(std::string const&) const = 0;

protected:
    [[nodiscard]] static ErrorOr<void> check_existence(std::string const& file_name);
};

class SimpleBufferLocator : public BufferLocator {
public:
    SimpleBufferLocator() = default;
    [[nodiscard]] ErrorOr<std::string> locate(std::string const&) const override;
};

class FileBuffer {
public:
    explicit FileBuffer(std::string const& file_name, BufferLocator* locator = nullptr);
    [[nodiscard]] std::unique_ptr<StringBuffer>& buffer() { return m_buffer; }
    [[nodiscard]] bool file_is_read() const { return m_error.code() == ErrorCode::NoError; };
    [[nodiscard]] Error<int> const& error() const { return m_error; }
    [[nodiscard]] size_t size() const { return m_size; }
    [[nodiscard]] std::string file_name() const;
    [[nodiscard]] std::string dir_name() const;
    [[nodiscard]] std::string file_path() const;
    [[nodiscard]] std::filesystem::path const& path() const { return m_path; }

private:
    std::filesystem::path m_path {};
    std::string m_file_name;
    std::unique_ptr<StringBuffer> m_buffer;
    size_t m_size { 0 };
    Error<int> m_error { ErrorCode::NoError };
    std::unique_ptr<BufferLocator> m_buffer_locator { nullptr };
};

}
