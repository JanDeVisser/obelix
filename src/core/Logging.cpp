/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-10-04.
//

#include <core/Logging.h>
#include <ctime>
#include <mutex>

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
        fatal("Unreachable");
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
    : m_enabled(false)
    , m_name(move(name))
    , m_logger(&Logger::get_logger())
{
    m_level = m_logger->level();
    m_logger->add_category(this);
}

bool LoggingCategory::enabled() const {
    return m_enabled || m_logger->m_all_enabled;
}

std::clock_t LoggingCategory::start()
{
    if constexpr (DEBUG) {
        return std::clock();
    } else {
        return 0;
    }
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
