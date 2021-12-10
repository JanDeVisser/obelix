/*
 * Directive.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <core/Logging.h>
#include <oblasm/AssemblyTypes.h>

namespace Obelix::Assembler {

class Image;

class Entry {
public:
    explicit Entry(Mnemonic mnemonic, std::string const& arguments)
        : m_mnemonic(mnemonic)
        , m_arguments(split(arguments, ','))
    {
    }
    virtual ~Entry() = default;

    void add_error(std::string const& error) { m_errors.push_back(error); }
    [[nodiscard]] std::vector<std::string> const& errors() const { return m_errors; }

    [[nodiscard]] virtual std::string_view prefix() const;
    [[nodiscard]] virtual std::string to_string() const;
    [[nodiscard]] virtual uint16_t size() const;
    virtual void append_to(Image&);

    void add_errors(std::vector<std::string> const& errors) { m_errors.insert(m_errors.end(), errors.cbegin(), errors.cend()); }

    [[nodiscard]] Mnemonic mnemonic() const { return m_mnemonic; }
    [[nodiscard]] std::vector<std::string> const& arguments() const { return m_arguments; }

private:
    std::vector<std::string> m_errors {};
    Mnemonic m_mnemonic { Mnemonic::NOP };
    std::vector<std::string> m_arguments;
};

class Label : public Entry {
public:
    Label(std::string const& label, uint16_t value);
    Label(std::string const& label, std::string const&);

    [[nodiscard]] std::string to_string() const override { return m_label; }
    [[nodiscard]] std::string const& label() const { return m_label; }
    [[nodiscard]] uint16_t value() const { return m_value; }

protected:
    Label(Mnemonic mnemonic, std::string const& label, uint16_t value);
    Label(Mnemonic mnemonic, std::string const& label, std::string const&);

private:
    std::string m_label;
    uint16_t m_value {0};
};

class Define : public Label {
public:
    Define(std::string const& label, uint16_t value);
    Define(std::string const& label, std::string const& value);

    [[nodiscard]] std::string_view prefix() const override { return ""; }
    [[nodiscard]] std::string to_string() const override { return format("{} = {04x}", label(), value()); }
};

class Align : public Entry {
public:
    explicit Align(uint16_t boundary);
    explicit Align(std::string const&);
    [[nodiscard]] std::string to_string() const override { return format(".align {}", boundary()); }
    [[nodiscard]] uint16_t boundary() const { return m_boundary; }

    void append_to(Image& image) override;

private:
    uint16_t m_boundary {0};
};

}
