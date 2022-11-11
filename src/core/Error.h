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

#define ENUMERATE_ERROR_CODES(S)                                                              \
    S(NoError, "There is no error")                                                           \
    S(IOError, "{}")                                                                          \
    S(ExecutionError, "Error executing '{}': {}")                                             \
    S(InternalError, "{}")                                                                    \
    S(SyntaxError, "{}")                                                                      \
    S(NotYetImplemented, "{}")                                                                \
    S(NoSuchFile, "File '{}' does not exist")                                                 \
    S(PathIsDirectory, "Path '{}' is a directory")                                            \
    S(PathIsFile, "Path '{}' is a file")                                                      \
    S(ObjectNotCallable, "Object '{}' is not callable")                                       \
    S(ObjectNotIterable, "Object '{}' is not iterable")                                       \
    S(CannotAssignToObject, "Cannot assign to object '{}'")                                   \
    S(VariableAlreadyDeclared, "Variable '{}' is already declared")                           \
    S(NameUnresolved, "Could not resolve '{}'")                                               \
    S(OperatorUnresolved, "Could not apply '{}' to '{}'")                                     \
    S(ReturnTypeUnresolved, "Return type of operator '{}' unresolved")                        \
    S(ConversionError, "Cannot convert '{}' to {}")                                           \
    S(FunctionUndefined, "Function '{}' in image '{}' is undefined")                          \
    S(UndeclaredVariable, "Undeclared variable '{}'")                                         \
    S(RegexpSyntaxError, "Regular expression syntax error")                                   \
    S(TypeMismatch, "Type mismatch in '{}'. Expected '{}', got '{}'")                         \
    S(UntypedVariable, "Variable '{}' is untyped")                                            \
    S(UntypedFunction, "Function '{}' has no return type")                                    \
    S(UntypedParameter, "Parameter '{}' has no return type")                                  \
    S(UntypedExpression, "Expression '{}' has no type")                                       \
    S(ArgumentCountMismatch, "Function {} called with {} arguments")                          \
    S(ArgumentTypeMismatch, "Function {} called with argument of type {} for parameter {}")   \
    S(CouldNotResolveNode, "Could not resolve node")                                          \
    S(CantUseAsUnaryOp, "Cannot use '{}' as a unary operation")                               \
    S(CannotAssignToConstant, "Cannot assign to constant '{}'")                               \
    S(CannotAssignToFunction, "Identifier '{}' represents a function and cannot be assigned") \
    S(CannotAssignToRValue, "Cannot assign to expression '{}'")                               \
    S(NoSuchType, "Unknown type '{}'")                                                        \
    S(DuplicateTypeName, "Duplicate type '{}'")                                               \
    S(TemplateParameterMismatch, "Template '{}' expects {} arguments. Got {}")                \
    S(TypeNotParameterized, "Type '{}' is not parameterized")                                 \
    S(CannotAccessMember, "Cannot access members of non-struct expression '{}'")              \
    S(NotMember, "Expression '{}' is not a member of '{}'")                                   \
    S(IndexOutOfBounds, "Index value {} not in [0..{}]")

enum class ErrorCode {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) code,
    ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
};

std::string ErrorCode_name(ErrorCode);
std::string ErrorCode_message(ErrorCode);

template <typename T>
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

    Error(ErrorCode code, T t)
        : m_code(code)
        , m_message(ErrorCode_name(code))
        , m_payload(std::move(t))
    {
    }

    template <typename U>
    Error(Error<U> const& other, T t)
        : m_code(other.code())
        , m_message(other.message())
        , m_payload(std::move(t))
    {
    }

    template<typename... Args>
    explicit Error(ErrorCode code, Args&&... args)
        : m_code(code)
        , m_message(ErrorCode_name(code) + ": " + format(std::string(ErrorCode_message(code)), std::forward<Args>(args)...))
    {
    }

    template<typename... Args>
    explicit Error(ErrorCode code, T t, Args&&... args)
        : m_code(code)
        , m_message(ErrorCode_name(code) + ": " + format(std::string(ErrorCode_message(code)), std::forward<Args>(args)...))
        , m_payload(std::move(t))
    {
    }

    [[nodiscard]] ErrorCode code() const { return m_code; }
    [[nodiscard]] std::string const& message() const { return m_message; }
    [[nodiscard]] std::string to_string() const { return format("{} {}", ErrorCode_name(code()), message()); }
    [[nodiscard]] T const& payload() const { return m_payload; }

private:
    ErrorCode m_code { ErrorCode::NoError };
    std::string m_message {};
    T m_payload {};
};

template<typename ReturnType, typename ErrorType = Error<int>>
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
    ErrorOr(U&& value) requires(!std::is_same_v<std::remove_cvref_t<U>, ReturnType> && !std::is_same_v<std::remove_cvref_t<U>, ErrorOr<ReturnType, ErrorType>> && !std::is_same_v<std::remove_cvref_t<U>, ErrorType>)
        : m_value(std::forward<U>(value))
    {
    }

    ErrorOr(ErrorType const& error)
        : m_error(error)
    {
    }

    ErrorOr(ErrorType&& error)
        : m_error(std::move(error))
    {
    }

    ErrorOr(ErrorOr&&) noexcept = default;
    ErrorOr(ErrorOr const&) = default;
    ~ErrorOr() = default;

    ErrorOr& operator=(ErrorOr&&) noexcept = default;
    ErrorOr& operator=(ErrorOr const&) = default;

    [[nodiscard]] bool has_value() const { return m_value.has_value(); }
    ReturnType const& value() const { return m_value.value(); }
    [[nodiscard]] bool is_error() const { return m_error.has_value(); }
    ErrorType const& error() const { return m_error.value(); }

private:
    std::optional<ReturnType> m_value {};
    std::optional<ErrorType> m_error {};
};

template<typename ErrorType>
class [[nodiscard]] ErrorOr<void, ErrorType> {
public:
    ErrorOr(ErrorType const& error)
        : m_error(error)
    {
    }

    ErrorOr() = default;
    ErrorOr(ErrorOr&& other) noexcept = default;
    ErrorOr(ErrorOr const& other) = default;
    ~ErrorOr() = default;

    ErrorOr& operator=(ErrorOr&& other) noexcept = default;
    ErrorOr& operator=(ErrorOr const& other) = default;

    ErrorType& error() { return m_error.value(); }
    [[nodiscard]] bool is_error() const { return m_error.has_value(); }
    void release_value() { }

private:
    std::optional<ErrorType> m_error;
};

#define TRY(expr)                             \
    ({                                        \
        auto _temporary_result = (expr);      \
        if (_temporary_result.is_error())     \
            return _temporary_result.error(); \
        _temporary_result.value();            \
    })

#define TRY_ADAPT(expr, payload)                                                                         \
    ({                                                                                                   \
        auto _temporary_result = (expr);                                                                 \
        if (_temporary_result.is_error())                                                                \
            return Error<std::remove_cvref_t<decltype(payload)>> { _temporary_result.error(), payload }; \
        _temporary_result.value();                                                                       \
    })

#define TRY_RETURN(expr)                   \
    if (auto err = (expr); err.is_error()) \
    return err.error()

#define TRY_OR_EXCEPTION(expr)                                     \
    ({                                                             \
        auto _temporary_result = (expr);                           \
        if (_temporary_result.is_error())                          \
            return make_obj<Exception>(_temporary_result.error()); \
        _temporary_result.value();                                 \
    })

}
