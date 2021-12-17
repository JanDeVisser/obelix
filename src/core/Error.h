/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <concepts>
#include <string>

#include <core/Format.h>

namespace Obelix {

#define ENUMERATE_ERROR_CODES(S)                                                \
    S(NoError, "There is no error")                                             \
    S(IOError, "{}")                                                            \
    S(SyntaxError, "{}")                                                        \
    S(NotYetImplemented, "{}")                                                  \
    S(NoSuchFile, "File '{}' does not exist")                                   \
    S(PathIsDirectory, "Path '{}' is a directory")                              \
    S(PathIsFile, "Path '{}' is a file")                                        \
    S(ObjectNotCallable, "Object '{}' is not callable")                         \
    S(ObjectNotIterable, "Object '{}' is not iterable")                         \
    S(CannotAssignToObject, "Cannot assign to object '{}'")                     \
    S(VariableAlreadyDeclared, "Variable '{}' is already declared")             \
    S(NameUnresolved, "Could not resolve '{}'")                                 \
    S(OperatorUnresolved, "Could not apply '{}' to '{}'")                       \
    S(ConversionError, "Cannot convert '{}' to {}")                             \
    S(FunctionUndefined, "Function '{}' in image '{}' is undefined")            \
    S(UndeclaredVariable, "Undeclared variable '{}'")                           \
    S(RegexpSyntaxError, "Regular expression syntax error")                     \
    S(TypeMismatch, "Type mismatch in operation '{}'. Expected '{}', got '{}'") \
    S(CouldNotResolveNode, "Could not resolve node")                            \
    S(CantUseAsUnaryOp, "Cannot use '{}' as a unary operation")

enum class ErrorCode {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) code,
    ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
};

std::string ErrorCode_name(ErrorCode);
std::string ErrorCode_message(ErrorCode);

class Error {
public:
    Error()
        : Error(ErrorCode::NoError)
    {
    }

    explicit Error(ErrorCode code)
        : m_code(code)
        , m_message(ErrorCode_name(code))
    {
    }

    template<typename... Args>
    explicit Error(ErrorCode code, Args&&... args)
        : m_code(code)
        , m_message(ErrorCode_name(code) + ": " + format(std::string(ErrorCode_message(code)), std::forward<Args>(args)...))
    {
    }

    [[nodiscard]] ErrorCode code() const { return m_code; }
    [[nodiscard]] std::string const& message() const { return m_message; }

private:
    ErrorCode m_code { ErrorCode::NoError };
    std::string m_message {};
};

template<typename ReturnType>
class [[nodiscard]] ErrorOr {
public:
    ErrorOr(ReturnType const& return_value)
        : m_value(return_value)
    {
    }

    ErrorOr(ReturnType&& return_value)
        : m_value(std::move(return_value))
    {
    }

    template<typename U>
    ErrorOr(U&& value) requires(!std::is_same_v<std::remove_cvref_t<U>, ReturnType> && !std::is_same_v<std::remove_cvref_t<U>, ErrorOr<ReturnType>> && !std::is_same_v<std::remove_cvref_t<U>, Error>)
        : m_value(std::forward<U>(value))
    {
    }

    ErrorOr(Error const& error)
        : m_error(error)
    {
    }

    ErrorOr(Error&& error)
        : m_error(std::move(error))
    {
    }

    ErrorOr(ErrorOr&&) noexcept = default;
    ErrorOr(ErrorOr const&) = default;
    ~ErrorOr() = default;

    ErrorOr& operator=(ErrorOr&&) noexcept = default;
    ErrorOr& operator=(ErrorOr const&) = default;

    [[nodiscard]] bool has_value() const { return m_value.has_value(); }
    ReturnType& value() { return m_value.value(); }
    [[nodiscard]] bool is_error() const { return m_error.has_value(); }
    Error& error() { return m_error.value(); }

private:
    std::optional<ReturnType> m_value {};
    std::optional<Error> m_error {};
};

#define TRY(expr)                             \
    ({                                        \
        auto _temporary_result = (expr);      \
        if (_temporary_result.is_error())     \
            return _temporary_result.error(); \
        _temporary_result.value();            \
    })

#define TRY_OR_EXCEPTION(expr)                                     \
    ({                                                             \
        auto _temporary_result = (expr);                           \
        if (_temporary_result.is_error())                          \
            return make_obj<Exception>(_temporary_result.error()); \
        _temporary_result.value();                                 \
    })

}
