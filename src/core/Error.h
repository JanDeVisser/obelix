/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <concepts>
#include <cerrno>
#include <string>

#include <core/Format.h>

namespace Obelix {

#define ENUMERATE_ERROR_CODES(S)                                                              \
    S(ArgumentCountMismatch, "Function {} called with {} arguments")                          \
    S(ArgumentTypeMismatch, "Function {} called with argument of type {} for parameter {}")   \
    S(CannotAccessMember, "Cannot access members of non-struct expression '{}'")              \
    S(CannotAssignToConstant, "Cannot assign to constant '{}'")                               \
    S(CannotAssignToFunction, "Identifier '{}' represents a function and cannot be assigned") \
    S(CannotAssignToObject, "Cannot assign to object '{}'")                                   \
    S(CannotAssignToRValue, "Cannot assign to expression '{}'")                               \
    S(CantUseAsUnaryOp, "Cannot use '{}' as a unary operation")                               \
    S(ConversionError, "Cannot convert '{}' to {}")                                           \
    S(CouldNotResolveNode, "Could not resolve node")                                          \
    S(DuplicateTypeName, "Duplicate type '{}'")                                               \
    S(ExecutionError, "Error executing '{}': {}")                                             \
    S(FunctionUndefined, "Function '{}' in image '{}' is undefined")                          \
    S(IOError, "{}")                                                                          \
    S(IndexOutOfBounds, "Index value {} not in [0..{}]")                                      \
    S(InternalError, "{}")                                                                    \
    S(NameUnresolved, "Could not resolve '{}'")                                               \
    S(NoError, "There is no error")                                                           \
    S(NoSuchFile, "File '{}' does not exist")                                                 \
    S(NoSuchType, "Unknown type '{}'")                                                        \
    S(NotMember, "Expression '{}' is not a member of '{}'")                                   \
    S(NotYetImplemented, "{}")                                                                \
    S(ObjectNotCallable, "Object '{}' is not callable")                                       \
    S(ObjectNotIterable, "Object '{}' is not iterable")                                       \
    S(OperatorUnresolved, "Could not apply '{}' to '{}'")                                     \
    S(PathIsDirectory, "Path '{}' is a directory")                                            \
    S(PathIsFile, "Path '{}' is a file")                                                      \
    S(RegexpSyntaxError, "Regular expression syntax error")                                   \
    S(ReturnTypeUnresolved, "Return type of operator '{}' unresolved")                        \
    S(SyntaxError, "{}")                                                                      \
    S(TemplateParameterMismatch, "Template '{}' expects {} arguments. Got {}")                \
    S(TypeMismatch, "Type mismatch in '{}'. Expected '{}', got '{}'")                         \
    S(TypeNotParameterized, "Type '{}' is not parameterized")                                 \
    S(UndeclaredVariable, "Undeclared variable '{}'")                                         \
    S(UntypedExpression, "Expression '{}' has no type")                                       \
    S(UntypedFunction, "Function '{}' has no return type")                                    \
    S(UntypedParameter, "Parameter '{}' has no return type")                                  \
    S(UntypedVariable, "Variable '{}' is untyped")                                            \
    S(VariableAlreadyDeclared, "Variable '{}' is already declared")                           \
    S(ZZLast, "Don't use me")

enum class ErrorCode {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) code,
    ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
};

std::string ErrorCode_name(ErrorCode);
std::string ErrorCode_message(ErrorCode);

template<>
struct Converter<ErrorCode> {
    static std::string to_string(ErrorCode val)
    {
        return ErrorCode_name(val);
    }

    static double to_double(ErrorCode val)
    {
        return static_cast<double>(val);
    }

    static long to_long(ErrorCode val)
    {
        return static_cast<long>(val);
    }
};


class SystemError {
public:
    SystemError(ErrorCode code, std::string msg = {})
        : m_code(code)
        , m_errno(errno)
        , m_message(std::move(msg))
    {
        if (m_message.empty()) {
            if (m_errno != 0)
                m_message = strerror(m_errno);
            else
                m_message = "No Error";
        }
    }

    template <typename ...Args>
    SystemError(ErrorCode code, std::string message, Args&&... args)
        : m_code(code)
        , m_errno(errno)
    {
        m_message = format(message, std::forward<Args>(args)...);
    }

    [[nodiscard]] std::string const& message() const { return m_message; }
    [[nodiscard]] ErrorCode code() const { return m_code; }
    [[nodiscard]] int sys_errno() const { return m_errno; }

    [[nodiscard]] std::string to_string() const
    {
        if (m_errno != 0)
            return format("[{}] {}: {}", m_code, m_message, strerror(m_errno));
        return format("[{}] {}", m_code, m_message);
    }

private:
    ErrorCode m_code;
    int m_errno { 0 };
    std::string m_message;
};


template<typename ReturnType, typename ErrorType = ErrorCode>
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
    ErrorOr(U&& value)
        requires(!std::is_same_v<std::remove_cvref_t<U>, ReturnType> && !std::is_same_v<std::remove_cvref_t<U>, ErrorOr<ReturnType, ErrorType>> && !std::is_same_v<std::remove_cvref_t<U>, ErrorType>)
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
