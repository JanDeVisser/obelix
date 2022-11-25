/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <config.h>
#include <core/Logging.h>
#include <core/Process.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/Processor.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(c_transpiler);

class COutputFile {
public:
    COutputFile(std::string name)
        : m_name(std::move(name))
        , m_path(m_name)
    {
    }

    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] std::filesystem::path const& path() const { return m_path; }

    ErrorOr<void, SyntaxError> flush()
    {
        if (m_flushed)
            return {};
        auto c_file = ".obelix/" + name();
        std::fstream s(c_file, std::fstream::out);
        if (!s.is_open())
            return SyntaxError { ErrorCode::IOError, format("Could not open transpiled file {}", c_file) };
        s << m_text;
        if (s.fail() || s.bad())
            return SyntaxError { ErrorCode::IOError, format("Could not write transpiled file {}", c_file) };
        m_flushed = true;
        return {};
    }

    void write(std::string const& text)
    {
        if (text.empty())
            return;
        auto t = text;
        int trailing_newlines { 0 };
        while (!t.empty() && t.ends_with("\n")) {
            trailing_newlines++;
            t = t.substr(0, t.length() - 1);
        }
        t = join(split(t, '\n'), "\n" + m_indent);
        if (m_text.ends_with("\n"))
            m_text.append(m_indent);
        m_text.append(t);
        for (auto ix = 0; ix < trailing_newlines; ++ix)
            m_text += "\n";
        m_flushed = false;
    }

    void writeln(std::string const& text)
    {
        write(text + "\n");
    }

    void indent()
    {
        m_indent += "  ";
    }

    void dedent()
    {
        if (m_indent.length() > 0)
            m_indent = m_indent.substr(0, m_indent.length() - 2);
    }

    [[nodiscard]] std::string to_string() const
    {
        std::string ret = name() + "\n\n";
        ret += m_text;
        return ret;
    }

private:
    std::string m_name;
    std::filesystem::path m_path;
    std::string m_text;
    std::string m_indent;
    bool m_flushed { false };
};

class CTranspilerContextPayload {
public:
    CTranspilerContextPayload() = default;

    std::string const& header_name() const
    {
        return header->name();
    }

    ErrorOr<void, SyntaxError> open_header(std::string main_module)
    {
        if (current_file != nullptr) {
            if (auto error_maybe = current_file->flush(); error_maybe.is_error())
                return error_maybe.error();
        }
        header = std::make_shared<COutputFile>(format("{}.h", main_module));
        current_file = header;
        return {};
    }

    ErrorOr<void, SyntaxError> open_output_file(std::string name)
    {
        if (current_file != nullptr) {
            if (auto error_maybe = current_file->flush(); error_maybe.is_error())
                return error_maybe.error();
        }
        modules.emplace(name, std::make_shared<COutputFile>(name));
        current_file = modules.at(name);
        return {};
    }

    std::vector<COutputFile const*> files()
    {
        std::vector<COutputFile const*> ret { header.get() };
        for (auto const& file : modules) {
            ret.push_back(file.second.get());
        }
        return ret;
    }

    ErrorOr<void, SyntaxError> flush()
    {
        if (current_file != nullptr) {
            if (auto error_maybe = current_file->flush(); error_maybe.is_error())
                return error_maybe.error();
            current_file = nullptr;
        }
        return {};
    }

    void writeln(std::string const& text)
    {
        assert(current_file);
        current_file->writeln(text);
    }

    void write(std::string const& text)
    {
        assert(current_file);
        current_file->write(text);
    }

    void indent()
    {
        assert(current_file);
        current_file->indent();
    }

    void dedent()
    {
        assert(current_file);
        current_file->dedent();
    }

    std::shared_ptr<COutputFile> header;
    std::map<std::string, std::shared_ptr<COutputFile>> modules;
    std::shared_ptr<COutputFile> current_file;
};

using CTranspilerContext = Context<std::shared_ptr<SyntaxNode>, CTranspilerContextPayload>;

ErrorOr<void, SyntaxError> open_header(CTranspilerContext&, std::string);
ErrorOr<void, SyntaxError> open_output_file(CTranspilerContext&, std::string);
std::vector<COutputFile const*> files(CTranspilerContext&);
ErrorOr<void, SyntaxError> flush(CTranspilerContext&);
void writeln(CTranspilerContext&, std::string const&);
void write(CTranspilerContext&, std::string const&);
void indent(CTranspilerContext&);
void dedent(CTranspilerContext&);
std::string const& exit_label(CTranspilerContext const&);

// FIXME Why doesn't this work????
template<>
inline CTranspilerContext& make_subcontext(CTranspilerContext& ctx)
{
    return ctx.make_subcontext();
}

}
