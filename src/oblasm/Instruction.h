/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <optional>

#include <core/Format.h>
#include <core/StringUtil.h>
#include <oblasm/AssemblyTypes.h>
#include <oblasm/Buffer.h>
#include <oblasm/Bytes.h>
#include <oblasm/Directive.h>
#include <oblasm/String.h>

namespace Obelix::Assembler {

class Instruction : public Entry {
public:
    explicit Instruction(Mnemonic);
    Instruction(Mnemonic, Argument);
    Instruction(Mnemonic, Argument, Argument);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] uint16_t size() const override;
    void append_to(Image& image) override;
    [[nodiscard]] bool valid() const;
    [[nodiscard]] Argument const& target() const { return m_target; }
    [[nodiscard]] Argument const& source() const { return m_source; }
    [[nodiscard]] OpcodeDefinition const& definition() const { return m_definition; }

private:
    Argument m_target;
    Argument m_source;
    OpcodeDefinition m_definition;
};

}
