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

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include <config.h>
#include <core/Logging.h>
#include <lexer/FileBuffer.h>

namespace Obelix {

logging_category(filebuffer);

FileBuffer::FileBuffer(std::string file_name)
    : m_file_name(move(file_name))
    , m_buffer(std::make_unique<StringBuffer>())
{
    auto fh_stat = try_open(m_file_name.empty() ? "." : m_file_name);
    std::string obl_dir = (getenv("OBL_DIR")) ? getenv("OBL_DIR") : OBELIX_DIR;
    if (fh_stat.first < 0)
        fh_stat = try_open(obl_dir + "/share/" + m_file_name);
    if (fh_stat.first < 0)
        fh_stat = try_open("./share/" + m_file_name);
    if (fh_stat.first < 0)
        fh_stat = try_open("./" + m_file_name);
    if (fh_stat.first < 0)
        fatal("Could not find file '{}'", m_file_name);

    auto size = fh_stat.second.st_size;
    auto buf = new char[size + 1];
    if (auto rc = ::read(fh_stat.first, (void*)buf, size); rc < size) {
        perror("read");
    } else {
        buf[size] = '\0';
        m_buffer->assign(buf);
    }
    close(fh_stat.first);
    delete[] buf;
}

StringBuffer& FileBuffer::buffer()
{
    return *m_buffer;
}

std::pair<int, struct stat> FileBuffer::try_open(std::string const& path)
{
    assert(!path.empty());
    debug(filebuffer, "Trying to open file {}", path);
    std::pair<int, struct stat> ret;
    auto fh = ::open(path.c_str(), O_RDONLY);
    if ((fh < 0) && (errno != ENOENT)) {
        fatal("Open '{}': {} ({})", path, std::string(strerror(errno)), errno);
    }
    ret.first = fh;
    if (fh < 0) {
        if (path.ends_with(".obl")) {
            return ret;
        } else {
            return try_open(path + ".obl");
        }
    }
    if (auto rc = fstat(fh, &ret.second); rc < 0) {
        perror(format("Stat '{}'", path).c_str());
        exit(1);
    }
    if (S_ISDIR(ret.second.st_mode)) {
        return try_open(path + "/__init__.obl");
    } else if ((fh < 0) && (path.find('.') == std::string::npos)) {
        return try_open(path + ".obl");
    }
    return ret;
};

}
