/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/Processor.h>
#include <obelix/Syntax.h>
#include <obelix/Type.h>

namespace Obelix {

#define ENUMERATE_VARIABLEADDRESSTYPES(S) \
    S(StackVariableAddress)               \
    S(StaticVariableAddress)              \
    S(GlobalVariableAddress)              \
    S(StructMemberAddress)

enum class VariableAddressType {
#undef __VARIABLEADDRESSTYPE
#define __VARIABLEADDRESSTYPE(type) type,
    ENUMERATE_VARIABLEADDRESSTYPES(__VARIABLEADDRESSTYPE)
#undef __VARIABLEADDRESSTYPE
};

constexpr char const* VariableAddressType_name(VariableAddressType type)
{
    switch (type) {
#undef __VARIABLEADDRESSTYPE
#define __VARIABLEADDRESSTYPE(type) \
    case VariableAddressType::type: \
        return #type;
        ENUMERATE_VARIABLEADDRESSTYPES(__VARIABLEADDRESSTYPE)
#undef __VARIABLEADDRESSTYPE
    }
}

template<>
struct Converter<VariableAddressType> {
    static std::string to_string(VariableAddressType val)
    {
        return VariableAddressType_name(val);
    }

    static double to_double(VariableAddressType val)
    {
        return static_cast<double>(val);
    }

    static long to_long(VariableAddressType val)
    {
        return static_cast<long>(val);
    }
};

class ARM64Context;

ErrorOr<void, SyntaxError> zero_initialize(ARM64Context&, std::shared_ptr<ObjectType> const&, int);
ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType> const&, ARM64Context& ctx, size_t, int);
ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType> const&, ARM64Context& ctx, size_t, int);

class VariableAddress {
public:
    virtual ~VariableAddress() = default;
    virtual std::string to_string() const = 0;
    virtual VariableAddressType address_type() const = 0;
    virtual ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const = 0;
    virtual ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const = 0;
    virtual ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const = 0;
};

class StackVariableAddress : public VariableAddress {
public:
    explicit StackVariableAddress(int offset)
        : m_offset(offset)
    {
    }
    [[nodiscard]] int offset() const { return m_offset; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("StackVariableAddress: [{}]", m_offset);
    }
    [[nodiscard]] VariableAddressType address_type() const override { return VariableAddressType::StackVariableAddress; }
    ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const override;

private:
    int m_offset;
};

struct StaticVariableAddress : public VariableAddress {
public:
    explicit StaticVariableAddress(std::string label)
        : m_label(move(label))
    {
    }
    [[nodiscard]] std::string const& label() const { return m_label; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("StaticVariableAddress: [.{}]", m_label);
    }
    [[nodiscard]] VariableAddressType address_type() const override { return VariableAddressType::StaticVariableAddress; }
    ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const override;

private:
    std::string m_label;
};

struct GlobalVariableAddress : public StaticVariableAddress {
public:
    explicit GlobalVariableAddress(std::string label)
        : StaticVariableAddress(move(label))
    {
    }
    [[nodiscard]] std::string to_string() const override
    {
        return format("GlobalVariableAddress: [.{}]", label());
    }
    [[nodiscard]] VariableAddressType address_type() const override { return VariableAddressType::GlobalVariableAddress; }
};

struct StructMemberAddress : public VariableAddress {
public:
    StructMemberAddress(std::shared_ptr<VariableAddress> strukt, int offset)
        : m_struct(move(strukt))
        , m_offset(offset)
    {
    }
    [[nodiscard]] std::shared_ptr<VariableAddress> const& structure() const { return m_struct; }
    [[nodiscard]] int offset() const { return m_offset; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("StructMemberAddress: [{}]", m_offset);
    }
    [[nodiscard]] VariableAddressType address_type() const override { return VariableAddressType::StructMemberAddress; }
    ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const override;

private:
    std::shared_ptr<VariableAddress> m_struct;
    int m_offset;
};

class ArrayElementAddress : public VariableAddress {
public:
    ArrayElementAddress(std::shared_ptr<VariableAddress> array, int element_size)
        : m_array(move(array))
        , m_element_size(element_size)
    {
    }
    [[nodiscard]] std::shared_ptr<VariableAddress> const& array() const { return m_array; }
    [[nodiscard]] int element_size() const { return m_element_size; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("ArrayElementAddress: [{}]", m_element_size);
    }
    [[nodiscard]] VariableAddressType address_type() const override { return VariableAddressType::StructMemberAddress; }
    ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType> const&, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const override;

private:
    std::shared_ptr<VariableAddress> m_array;
    int m_element_size;
};

}
