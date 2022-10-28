/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/syntax/Typedef.h>

namespace Obelix {

// -- Typedef.h -------------------------------------------------------------

NODE_CLASS(BoundType, SyntaxNode)
public:
    BoundType(Token, std::shared_ptr<ObjectType>);
    [[nodiscard]] std::shared_ptr<ObjectType> type() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<ObjectType> m_type;
};

using BoundTypes = std::vector<std::shared_ptr<BoundType>>;

NODE_CLASS(BoundStructDefinition, Statement)
public:
    BoundStructDefinition(std::shared_ptr<StructDefinition> const&, std::shared_ptr<ObjectType>);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ObjectType> type() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_name;
    std::shared_ptr<ObjectType> m_type;
};

NODE_CLASS(BoundEnumValueDef, SyntaxNode)
public:
    BoundEnumValueDef(Token, std::string, long);
    [[nodiscard]] long const& value() const;
    [[nodiscard]] std::string const& label() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    long m_value;
    std::string m_label;
};

using BoundEnumValueDefs = std::vector<std::shared_ptr<BoundEnumValueDef>>;

NODE_CLASS(BoundEnumDef, Statement)
public:
    BoundEnumDef(std::shared_ptr<EnumDef> const& enum_def, std::shared_ptr<ObjectType> type, BoundEnumValueDefs values);
    BoundEnumDef(Token, std::string name, std::shared_ptr<ObjectType> type, BoundEnumValueDefs values, bool extend);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ObjectType> type() const;
    [[nodiscard]] BoundEnumValueDefs const& values() const;
    [[nodiscard]] bool extend() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_name;
    std::shared_ptr<ObjectType> m_type;
    BoundEnumValueDefs m_values;
    bool m_extend { false };
};

NODE_CLASS(BoundTypeDef, Statement)
public:
    BoundTypeDef(Token, std::string, std::shared_ptr<BoundType>);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<BoundType> const& type() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_name;
    std::shared_ptr<BoundType> m_type;
};

}
