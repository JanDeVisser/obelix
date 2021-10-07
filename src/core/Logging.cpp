//
// Created by Jan de Visser on 2021-10-04.
//

#include <ctime>
#include <mutex>
#include <core/Logging.h>

namespace Obelix {

std::mutex g_logging_mutex;

std::string_view LogLevel_name(LogLevel level)
{
    switch (level) {
#undef __ENUMERATE_LOG_LEVEL
#define __ENUMERATE_LOG_LEVEL(level, cardinal) \
    case LogLevel::level:                      \
        return #level;
        ENUMERATE_LOG_LEVELS(__ENUMERATE_LOG_LEVEL)
#undef __ENUMERATE_LOG_LEVEL
    default:
        assert(false);
    }
}

std::optional<LogLevel> LogLevel_by_name(std::string_view const& name)
{
#undef __ENUMERATE_LOG_LEVEL
#define __ENUMERATE_LOG_LEVEL(level, cardinal) \
    if (name == #level) {                      \
        return LogLevel::level;                \
    }
    ENUMERATE_LOG_LEVELS(__ENUMERATE_LOG_LEVEL)
#undef __ENUMERATE_LOG_LEVEL
    return {};
}

LoggingCategory::LoggingCategory(std::string name) noexcept
    : m_name(move(name))
    , m_enabled(false)
    , m_logger(&Logger::get_logger())
{
    m_level = m_logger->level();
    m_logger->add_category(this);
}

bool LoggingCategory::enabled() const {
    return m_enabled || m_logger->m_all_enabled;
}


void LoggingCategory::vlogmsg_no_nl(LogLevel level, std::string_view const& file, int line, std::string_view const& function, std::string_view const& message, va_list args)
{
    if (m_logger && m_enabled)
        m_logger->vlogmsg_no_nl(level, file, line, function, message, args);
}

void LoggingCategory::vlogmsg(LogLevel level, std::string_view const& file, int line, std::string_view const& function, std::string_view const& message, va_list args)
{
    if (m_logger && m_enabled)
        m_logger->vlogmsg(level, file, line, function, message, args);
}

void LoggingCategory::logmsg(LogLevel level, std::string_view const& file, int line, std::string_view const& function, char const* message, ...)
{
    if (m_logger && m_enabled) {
        va_list args;
        va_start(args, message);
        m_logger->vlogmsg(level, file, line, function, message, args);
        va_end(args);
    }
}

std::clock_t LoggingCategory::start() {
    return std::clock();
}

void LoggingCategory::log_duration(std::clock_t clock_start, std::string_view const& file, int line, std::string_view const& caller, const char *msg, ...) {
    auto clock_end = std::clock();
    auto duration_ms = (unsigned long) (1000.0 * ((float) clock_end - (float) clock_start) / CLOCKS_PER_SEC);

    va_list args;
    va_start(args, msg);
    vlogmsg_no_nl(LogLevel::Debug, file, line, caller, msg, args);
    // FIXME
    fprintf(m_logger -> m_destination, "%ld.%03ld sec\n", duration_ms / 1000, duration_ms % 1000);
    va_end(args);
}

void Logger::set_nolock(std::basic_string_view<char> cat, bool enabled)
{
    if (cat != "all") {
        std::string cat_str(cat);
        if (m_categories.contains(cat_str)) {
            m_categories[cat_str] -> set(enabled);
        } else {
            m_unregistered[cat_str] = enabled;
        }
    } else {
        reset(enabled);
    }
}

LoggingCategory* Logger::add_category(LoggingCategory* category)
{
    const std::lock_guard<std::mutex> lock(g_logging_mutex);
    return add_category_nolock(category);
}

LoggingCategory* Logger::add_category_nolock(LoggingCategory* category)
{
    m_categories.insert_or_assign(category->name(), category);
    if (m_unregistered.contains(category->name())) {
        category->set(m_unregistered[category->name()]);
        m_unregistered.erase(category->name());
    }
    return m_categories[category->name()];
}

void Logger::reset(bool value)
{
    std::for_each(m_categories.begin(), m_categories.end(),
        [value](auto& p) {
            p.second->set(value);
        });
    std::for_each(m_unregistered.begin(), m_unregistered.end(),
        [value](auto &p) {
            p.second = value;
        });
    m_all_enabled = value;
}

void Logger::enable(std::string const& cat)
{
    const std::lock_guard<std::mutex> lock(g_logging_mutex);
    set_nolock(cat, true);
}

void Logger::disable(std::string const& cat)
{
    const std::lock_guard<std::mutex> lock(g_logging_mutex);
    set_nolock(cat, false);
}

bool Logger::status(std::string const& cat)
{
    if (m_categories.contains(cat)) {
        return m_categories[cat] -> enabled();
    } else if (m_unregistered.contains(cat)) {
        return m_unregistered[cat];
    } else {
        return m_all_enabled;
    }
}

LogLevel Logger::level() const
{
    return m_level;
}

LogLevel Logger::set_level(std::string const& level)
{
    const std::lock_guard<std::mutex> lock(g_logging_mutex);
    auto level_maybe = LogLevel_by_name(level);
    if (level_maybe.has_value())
        set_level(level_maybe.value());
    return m_level;
}

LogLevel Logger::set_level(LogLevel level)
{
    m_level = level;
    return m_level;
}

void Logger::set_file(std::string const& filename)
{
    const std::lock_guard<std::mutex> lock(g_logging_mutex);
    if (m_destination && (m_destination != stderr)) {
        fclose(m_destination);
    }
    m_destination = nullptr;
    m_logfile = filename;
}

void Logger::vlogmsg_no_nl(LogLevel level, std::string_view const& file, int line, std::string_view const& function, std::string_view const& message, va_list args)
{
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
    if ((level == LogLevel::None) || (level >= m_level)) {
        std::string_view f(file);
        if (f.front() == '/') {
            auto ix = f.find_last_of('/');
            if (ix != std::string_view::npos) {
                f = f.substr(ix + 1);
            }
        }
        fprintf(m_destination, "%-12.12s:%4d:%-20.20s:%-5.5s:",
            f.data(), line, function.data(), LogLevel_name(level).data());
        vfprintf(m_destination, message.data(), args);
    }
}

void Logger::vlogmsg(LogLevel level, std::string_view const& file, int line, std::string_view const& function, std::string_view const& message, va_list args)
{
    if ((level == LogLevel::None) || (level >= m_level)) {
        vlogmsg_no_nl(level, file, line, function, message, args);
        fprintf(m_destination, "\n");
    }
}

void Logger::logmsg(LogLevel level, std::string_view const& file, int line, std::string_view const& function, char const* message, ...)
{
    va_list args;

    if ((level == LogLevel::None) || (level >= m_level)) {
        va_start(args, message);
        vlogmsg(level, file, line, function, message, args);
        va_end(args);
    }
}

Logger::Logger()
{
    if (auto obl_logfile = getenv("OBL_LOGFILE"); obl_logfile) {
        m_logfile = obl_logfile;
    }

    if (auto lvl = getenv("OBL_LOGLEVEL"); lvl && *lvl) {
        set_level(lvl);
    }

    auto cats = getenv("OBL_DEBUG");
    if (!cats) {
        cats = getenv("DEBUG");
    }
    if (cats && *cats) {
        std::string_view cats_sv(cats);
        size_t prev = 0;
        for (auto ix = cats_sv.find_first_of(";,:", 0); ix != std::string_view::npos; prev = ix, ix = cats_sv.find_first_of(";,:", ix)) {
            set_nolock(cats_sv.substr(prev, ix), true);
        }
        set_nolock(cats_sv.substr(prev), true);
    }
}

Logger& Logger::get_logger()
{
    static Logger *logger = nullptr;
    const std::lock_guard<std::mutex> lock(g_logging_mutex);
    if (!logger)
        logger = new Logger();
    return *logger;
}

}
