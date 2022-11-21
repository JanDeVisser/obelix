/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstring>
#include <map>
#include <mutex>
#include <string>

#include <core/Format.h>

namespace Obelix {

#ifndef NDEBUG
constexpr static bool DEBUG = true;
#else
constexpr static bool DEBUG = false;
#endif

#define ENUMERATE_LOG_LEVELS(S) \
    S(None, -1)                 \
    S(Debug, 0)                 \
    S(Info, 1)                  \
    S(Warning, 2)               \
    S(Error, 3)                 \
    S(Fatal, 4)

enum class LogLevel {
#undef __ENUMERATE_LOG_LEVEL
#define __ENUMERATE_LOG_LEVEL(level, cardinal) level = cardinal,
    ENUMERATE_LOG_LEVELS(__ENUMERATE_LOG_LEVEL)
#undef __ENUMERATE_LOG_LEVEL
};

std::string_view LogLevel_name(LogLevel);
std::optional<LogLevel> LogLevel_by_name(std::string_view const&);

struct LogMessage {
    std::string_view file;
    size_t line;
    std::string_view function;
    LogLevel level;
    std::string message;
};

class Logger;
class LoggingCategory;

extern std::mutex g_logging_mutex;

class Logger {
public:
    LoggingCategory* add_category(LoggingCategory*);
    void reset(bool);
    void enable(std::string const&);
    void disable(std::string const&);
    [[nodiscard]] bool status(std::string const&);
    [[nodiscard]] LogLevel level() const;
    LogLevel set_level(std::string const&);
    LogLevel set_level(LogLevel);
    void set_file(std::string const&);

    template<typename... Args>
    void logmsg(LogMessage const& msg, Args&&... args)
    {
        if (msg.level == LogLevel::Debug && !DEBUG)
            return;
        const std::lock_guard<std::mutex> lock(g_logging_mutex);
        if (!m_destination) {
            if (!m_logfile.empty()) {
                m_destination = fopen(m_logfile.c_str(), "w");
                if (!m_destination) {
                    fprintf(stderr, "Could not open logfile '%s': %s\n", m_logfile.c_str(), strerror(errno));
                    fprintf(stderr, "Falling back to stderr\n");
                }
            }
            if (!m_destination) {
                m_destination = stderr;
            }
        }
        if ((msg.level <= LogLevel::Debug) || (msg.level >= m_level)) {
            std::string_view f(msg.file);
            if (f.front() == '/') {
                auto ix = f.find_last_of('/');
                if (ix != std::string_view::npos) {
                    f = f.substr(ix + 1);
                }
            }
            auto file_line = format("{s}:{d}", f, msg.line);
            auto prefix = format("{<24s}:{<20s}:{<5s}:", file_line, msg.function, LogLevel_name(msg.level));
            fprintf(m_destination, "%s", prefix.c_str());
            auto message = format(msg.message, std::forward<Args>(args)...);
            fprintf(m_destination, "%s\n", message.c_str());
        }
    }

    template<typename... Args>
    void error_msg(std::string_view const& file, size_t line, std::string_view const& function, char const* message, Args&&... args)
    {
        logmsg({ file, line, function, LogLevel::Error, message }, std::forward<Args>(args)...);
        exit(1);
    }

    template<typename... Args>
    [[noreturn]] void fatal_msg(std::string_view const& file, size_t line, std::string_view const& function, char const* message, Args&&... args)
    {
        logmsg({ file, line, function, LogLevel::Fatal, message }, std::forward<Args>(args)...);
        abort();
    }

    template<typename... Args>
    void assert_msg(std::string_view const& file, size_t line, std::string_view const& function, bool condition, char const* message, Args&&... args)
    {
        if (condition)
            return;
        logmsg({ file, line, function, LogLevel::Fatal, message }, std::forward<Args>(args)...);
        abort();
    }
    static Logger& get_logger();

private:
    friend LoggingCategory;

    Logger();
    void set_nolock(std::basic_string_view<char>, bool);
    LoggingCategory* add_category_nolock(LoggingCategory*);

    std::map<std::string, LoggingCategory*> m_categories {};
    std::map<std::string, bool> m_unregistered;
    LogLevel m_level { LogLevel::Debug };
    FILE* m_destination { stderr };
    std::string m_logfile {};
    bool m_all_enabled { false };
};

class LoggingCategory {
public:
    explicit LoggingCategory(std::string name) noexcept;
    LoggingCategory(LoggingCategory const&) = default;

    [[nodiscard]] bool enabled() const;
    [[nodiscard]] std::string name() const { return m_name; }

    template<typename... Args>
    void logmsg(LogMessage const& msg, Args&&... args)
    {
        if (m_logger && m_enabled)
            m_logger->logmsg(msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug_msg(std::string_view const& file, size_t line, std::string_view const& function, char const* message, Args&&... args)
    {
        if constexpr (!DEBUG)
            return;
        if (m_logger != nullptr && m_enabled) {
            m_logger->logmsg({ file, line, function, LogLevel::Debug, message }, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void info_msg(std::string_view const& file, size_t line, std::string_view const& function, char const* message, Args&&... args)
    {
        if (m_logger != nullptr && m_enabled) {
            m_logger->logmsg({ file, line, function, LogLevel::Info, message }, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void warning_msg(std::string_view const& file, size_t line, std::string_view const& function, char const* message, Args&&... args)
    {
        if (m_logger != nullptr && m_enabled) {
            m_logger->logmsg({ file, line, function, LogLevel::Warning, message }, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void error_msg(std::string_view const& file, size_t line, std::string_view const& function, char const* message, Args&&... args)
    {
        if (m_logger != nullptr && m_enabled) {
            m_logger->logmsg({ file, line, function, LogLevel::Error, message }, std::forward<Args>(args)...);
        }
    }

    static std::clock_t start();

    template<typename... Args>
    void log_duration(std::clock_t clock_start, std::string_view const& file, size_t line, std::string_view const& caller, const char* msg, Args&&... args)
    {
        if constexpr (!DEBUG)
            return;
        auto clock_end = std::clock();
        auto duration_ms = (unsigned long)(1000.0 * ((float)clock_end - (float)clock_start) / CLOCKS_PER_SEC);

        auto msg_with_timing = std::string(msg).append(" {d}.{03d} sec");
        LogMessage log_message {
            file, line, caller, LogLevel::Debug, msg_with_timing
        };
        logmsg(log_message, std::forward<Args>(args)..., duration_ms / 1000, duration_ms % 1000);
    }

private:
    friend Logger;
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    void set(bool value) { m_enabled = value; }

    bool m_enabled { false };
    std::string m_name {};
    LogLevel m_level;
    Logger* m_logger { nullptr };
};

#define logging_category(module) Obelix::LoggingCategory module##_logger(#module)
#define extern_logging_category(module) extern Obelix::LoggingCategory module##_logger
#define debug(module, fmt, args...) (module##_logger).debug_msg(__FILE__, __LINE__, __func__, fmt, ##args)
#define info(module, fmt, args...) (module##_logger).info_msg(__FILE__, __LINE__, __func__, fmt, ##args)
#define warning(module, fmt, args...) (module##_logger).warning_msg(__FILE__, __LINE__, __func__, fmt, ##args)
#define log_timestamp_start(module) ((module##_logger).start())
#define log_timestamp_end(module, ts, fmt, ...) (module##_logger).log_duration(ts, __FILE__, __LINE__, __func__, fmt, ##args)

#define log_error(fmt, args...) Logger::get_logger().error_msg(__FILE__, __LINE__, __func__, fmt, ##args)
#define fatal(fmt, args...) Logger::get_logger().fatal_msg(__FILE__, __LINE__, __func__, fmt, ##args)
#ifdef assert
#    undef assert
#endif
#define assert(condition) Obelix::Logger::get_logger().assert_msg(__FILE__, __LINE__, __func__, condition, "Assertion error: " #condition)
#define oassert(condition, fmt, args...) Obelix::Logger::get_logger().assert_msg(__FILE__, __LINE__, __func__, condition, fmt, ##args)

}
