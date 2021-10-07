//
// Created by Jan de Visser on 2021-10-04.
//

#pragma once

#include <map>
#include <string>

namespace Obelix {

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

class Logger;

class LoggingCategory {
public:
    explicit LoggingCategory(std::string name) noexcept;
    LoggingCategory(LoggingCategory const&) = default;

    [[nodiscard]] bool enabled() const;
    [[nodiscard]] std::string name() const { return m_name; }

    void vlogmsg_no_nl(LogLevel level, std::string_view const& file, int line, std::string_view const& function,
        std::string_view const& message, va_list args);
    void vlogmsg(LogLevel level, std::string_view const& file, int line, std::string_view const& function,
        std::string_view const& message, va_list args);
    void logmsg(LogLevel level, std::string_view const& file, int line, std::string_view const& function,
        char const* message, ...);
    static std::clock_t start();
    void log_duration(std::clock_t, std::string_view const&, int, std::string_view const&, const char*, ...);

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

    void vlogmsg_no_nl(LogLevel level, std::string_view const& file, int line, std::string_view const& function,
        std::string_view const& message, va_list args);
    void vlogmsg(LogLevel level, std::string_view const& file, int line, std::string_view const& function,
        std::string_view const& message, va_list args);
    void logmsg(LogLevel level, std::string_view const& file, int line, std::string_view const& function,
        char const* message, ...);

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

#ifndef NDEBUG
#    define logging_category(module) LoggingCategory module##_logger(#    module)
#    define log_timestamp_start(module)  ((module ## _logger).enabled()) ? (module ## _logger).start() : NULL)
#    define _debug(fmt, args...) Logger::get_logger().logmsg(LogLevel::Debug, __FILE__, __LINE__, __func__, fmt, ##args)
#    define _vdebug(fmt, args) Logger::get_logger().vlogmsg(LogLevel::Debug, __FILE__, __LINE__, __func__, fmt, args)
#    define debug(module, fmt, args...)                                                         \
        if (module##_logger.enabled()) {                                                        \
            module##_logger.logmsg(LogLevel::Debug, __FILE__, __LINE__, __func__, fmt, ##args); \
        }                                                                                       \
        (void)0
#    define vdebug(module, fmt, args...)                                                       \
        if ((module##_logger).enabled()) {                                                     \
            module##_logger.vlogmsg(LogLevel::Debug, __FILE__, __LINE__, __func__, fmt, args); \
        }                                                                                      \
        (void)0
#    define log_timestamp_end(module, ts, fmt, args...)                                    \
        if ((module##_logger).enabled()) {                                                 \
            (module##_logger).log_duration(ts, __FILE__, __LINE__, __func__, fmt, ##args); \
        }
#else /* NDEBUG */
#    define log_timestamp_start(module) (NULL)
#    define log_timestamp_end(module, ts, fmt, ...)
#    define _debug(fmt, args...)
#    define _vdebug(fmt, args)
#    define debug(module, fmt, args...)
#    define mdebug(module, fmt, args...)
#    define vdebug(module, fmt, args...)
#endif /* NDEBUG */

#define logmessage(fmt, args...) Logger::get_logger().logmsg(LogLevel::None, __FILE__, __LINE__, __func__, fmt, ##args)
#define info(fmt, args...) Logger::get_logger().logmsg(LogLevel::Info, __FILE__, __LINE__, __func__, fmt, ##args)
#define warn(fmt, args...) Logger::get_logger().logmsg(LogLevel::Warning, __FILE__, __LINE__, __func__, fmt, ##args)
#define error(fmt, args...) Logger::get_logger().logmsg(LogLevel::Error, __FILE__, __LINE__, __func__, fmt, ##args)
#define fatal(fmt, args...)                                                                      \
    {                                                                                            \
        Logger::get_logger().logmsg(LogLevel::Fatal, __FILE__, __LINE__, __func__, fmt, ##args); \
        abort();                                                                                 \
    }                                                                                            \
    (void)0
#define oassert(value, fmt, args...)                                                                 \
    {                                                                                                \
        if (!(value)) {                                                                              \
            Logger::get_logger().logmsg(LogLevel::Fatal, __FILE__, __LINE__, __func__, fmt, ##args); \
            abort();                                                                                 \
        }                                                                                            \
    }                                                                                                \
    (void)0
#define vlogmessage(fmt, args) Logger::get_logger().vlogmsg(LogLevel::None, __FILE__, __LINE__, __func__, fmt, args)
#define vinfo(fmt, args) Logger::get_logger().vlogmsg(LogLevel::Info, __FILE__, __LINE__, __func__, fmt, args)
#define vwarn(fmt, args) Logger::get_logger().vlogmsg(LogLevel::Warning, __FILE__, __LINE__, __func__, fmt, args)
#define verror(fmt, args) Logger::get_logger().vlogmsg(LogLevel::Error, __FILE__, __LINE__, __func__, fmt, args)
#define vfatal(fmt, args)                                                                       \
    {                                                                                           \
        Logger::get_logger().vlogmsg(LogLevel::Fatal, __FILE__, __LINE__, __func__, fmt, args); \
        abort();                                                                                \
    }                                                                                           \
    (void)0
#define voassert(value, fmt, args)                                                                  \
    {                                                                                               \
        if (!(value)) {                                                                             \
            Logger::get_logger().vlogmsg(LogLevel::Fatal, __FILE__, __LINE__, __func__, fmt, args); \
            abort();                                                                                \
        }                                                                                           \
    }                                                                                               \
    (void)0
}
