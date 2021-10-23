/*
 * FileBuffer.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include <lexer/FileBuffer.h>
#include <sys/stat.h>

namespace Obelix {

FileBuffer::FileBuffer(char const* file_name)
    : m_file_name(file_name)
    , m_buffer(std::make_unique<StringBuffer>())
{
    int fh = open(file_name, O_RDONLY);
    if (fh < 0) {
        perror("Open");
        return;
    }
    struct stat sb {
    };
    if (auto rc = fstat(fh, &sb); rc < 0) {
        perror("Stat");
        close(fh);
        return;
    }

    auto size = sb.st_size;
    auto buf = new char[size + 1];
    if (auto rc = ::read(fh, (void*)buf, size); rc < size) {
        perror("read");
    } else {
        buf[size] = '\0';
        m_buffer->assign(buf);
    }
    close(fh);
    delete[] buf;
}

FileBuffer::FileBuffer(std::string const& file_name)
    : FileBuffer(file_name.c_str())
{
}

StringBuffer& FileBuffer::buffer()
{
    return *m_buffer;
}

}
