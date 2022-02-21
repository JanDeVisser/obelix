/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <core/Type.h>

namespace Obelix {

using TypeID = size_t;

enum class ExpressionTypeKind {
    Native,
    Array,
};

class ExpressionType {
public:
    virtual ~ExpressionType() = default;
    static std::shared_ptr<ExpressionType> simple_type(std::string const& type);
    static std::shared_ptr<ExpressionType> simple_type(TypeID type);
    static std::shared_ptr<ExpressionType> get_type(TypeID);
    static std::shared_ptr<ExpressionType> get_type(std::string const&);

    [[nodiscard]] virtual ExpressionTypeKind type_kind() const = 0;
    [[nodiscard]] virtual std::string to_string() const = 0;
    [[nodiscard]] TypeID type_id() const { return m_type_id; }

protected:
    explicit ExpressionType(TypeID id)
        : m_type_id(id)
    {
    }

    static std::unordered_map<std::string, std::shared_ptr<ExpressionType>> s_types_by_name;
    static std::unordered_map<TypeID, std::shared_ptr<ExpressionType>> s_types_by_id;

private:
    TypeID m_type_id;
};

class NativeExpressionType : public ExpressionType {
public:
    explicit NativeExpressionType(ObelixType type)
        : ExpressionType(type)
        , m_type(type)
    {
    }

    [[nodiscard]] ExpressionTypeKind type_kind() const override { return ExpressionTypeKind::Native; }
    [[nodiscard]] ObelixType native_type() const { return m_type; }
    [[nodiscard]] std::string to_string() const override { return ObelixType_name(m_type); }

private:
    ObelixType m_type;
};

std::string type_name(std::shared_ptr<ExpressionType> type);

template<>
struct Converter<ExpressionType*> {
    static std::string to_string(ExpressionType const* val)
    {
        return val->to_string();
    }

    static double to_double(ExpressionType const* val)
    {
        return NAN;
    }

    static unsigned long to_long(ExpressionType const* val)
    {
        return 0;
    }
};

}

template<>
struct std::hash<Obelix::ExpressionType> {
    auto operator()(Obelix::ExpressionType const& type) const noexcept
    {
        return std::hash<Obelix::TypeID> {}(type.type_id());
    }
};
