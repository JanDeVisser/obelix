/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
            return SyntaxError { ErrorCode::IOError, format("Could not open C file {}", c_file) };
        s << m_text;
        if (s.fail() || s.bad())
            return SyntaxError { ErrorCode::IOError, format("Could not write C file {}", c_file) };
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
            t = t.substr(0, t.length()-1);
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

    void indent() {
        m_indent += "  ";
    }

    void dedent() {
        if (m_indent.length() > 0)
            m_indent = m_indent.substr(0, m_indent.length()-2);
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

class CTranspilerContext : public Context<std::shared_ptr<SyntaxNode>> {
public:
    CTranspilerContext(CTranspilerContext *parent = nullptr)
        : Context(parent)
    {
    }

    std::string const& header_name() const
    {
        return m_header->name();
    }

    ErrorOr<void, SyntaxError> open_header(std::string main_module)
    {
        if (parent() != nullptr) {
            return dynamic_cast<CTranspilerContext*>(parent())->open_header(main_module);
        }
        if (m_current != nullptr) {
            if (auto error_maybe = m_current->flush(); error_maybe.is_error())
                return error_maybe.error();
        }
        m_header = std::make_shared<COutputFile>(format("{}.h", main_module));
        m_current = m_header;
        return {};
    }

    ErrorOr<void, SyntaxError> open_output_file(std::string name)
    {
        if (parent() != nullptr) {
            return dynamic_cast<CTranspilerContext*>(parent())->open_output_file(name);
        }
        if (m_current != nullptr) {
            if (auto error_maybe = m_current->flush(); error_maybe.is_error())
                return error_maybe.error();
        }
        m_modules.emplace(name, std::make_shared<COutputFile>(name));
        m_current = m_modules.at(name);
        return {};
    }

    std::vector<COutputFile const*> files()
    {
        std::vector<COutputFile const*> ret { m_header.get() };
        for (auto const& file : m_modules) {
            ret.push_back(file.second.get());
        }
        return ret;
    }

    [[nodiscard]] std::shared_ptr<COutputFile> const& current() const
    {
        if (parent() != nullptr) {
            return dynamic_cast<CTranspilerContext const*>(parent())->current();
        }
        return m_current;
    }

    ErrorOr<void, SyntaxError> flush()
    {
        if (parent() != nullptr) {
            return dynamic_cast<CTranspilerContext*>(parent())->flush();
        }
        if (m_current != nullptr) {
            if (auto error_maybe = m_current->flush(); error_maybe.is_error())
                return error_maybe.error();
            m_current = nullptr;
        }
        return {};
    }

    void writeln(std::string const& text)
    {
        if (parent() != nullptr) {
            dynamic_cast<CTranspilerContext*>(parent())->writeln(text);
            return;
        }
        assert(m_current);
        m_current->writeln(text);
    }

    void write(std::string const& text)
    {
        if (parent() != nullptr) {
            dynamic_cast<CTranspilerContext*>(parent())->write(text);
            return;
        }
        assert(m_current);
        m_current->write(text);
    }

    void indent()
    {
        if (parent() != nullptr) {
            dynamic_cast<CTranspilerContext*>(parent())->indent();
            return;
        }
        assert(m_current);
        m_current->indent();
    }

    void dedent()
    {
        if (parent() != nullptr) {
            dynamic_cast<CTranspilerContext*>(parent())->dedent();
            return;
        }
        assert(m_current);
        m_current->dedent();
    }

private:
    std::shared_ptr<COutputFile> m_header;
    std::unordered_map<std::string, std::shared_ptr<COutputFile>> m_modules;
    std::shared_ptr<COutputFile> m_current;
};

}
