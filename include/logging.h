/*
 * /obelix/include/logging.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _log_level {
  LogLevelNone = -1,
  LogLevelDebug = 0,
  LogLevelInfo,
  LogLevelWarning,
  LogLevelError,
  LogLevelFatal
} log_level_t;

extern int core_debug;

typedef void log_timestamp_t;

extern void   logging_init(void);
extern void   logging_register_category(const char *, int *);
extern void   logging_reset(void);
extern void   logging_enable(const char *);
extern void   logging_disable(const char *);
extern void   _vlogmsg_no_nl(log_level_t, const char *, int, const char *, const char *, va_list);
extern void   _vlogmsg(log_level_t, const char *, int, const char *, const char *, va_list);
extern void   _logmsg(log_level_t, const char *, int, const char *, const char *, ...);
extern int    logging_status(const char *);
extern int    logging_level(void);
extern int    logging_set_level(const char *);
extern int    logging_set_file(const char *);

extern log_timestamp_t * _log_timestamp_start(void);
extern void              _log_timestamp_end(log_timestamp_t *, const char *, int, const char *, const char *, ...);

#ifndef NDEBUG
#define log_timestamp_start(module)  ((module ## _debug) ? _log_timestamp_start() : NULL)
#define _debug(fmt, args...)         _logmsg(LogLevelDebug, __FILE__, __LINE__, __func__, fmt, ## args)
#define _vdebug(fmt, args)           _vlogmsg(LogLevelDebug, __FILE__, __LINE__, __func__, fmt, args)
#define debug(module, fmt, args...)  if (module ## _debug) { _debug(fmt, ## args); }
#define mdebug(module, fmt, args...) debug(module, fmt, ##args)
#define vdebug(module, fmt, args...) if (module ## _debug) { _vdebug(fmt, args); }
#define log_timestamp_end(module, ts, fmt, args...)  if (module ## _debug) { _log_timestamp_end(ts, __FILE__, __LINE__, __func__, fmt, ## args); }
#else /* NDEBUG */
#define log_timestamp_start(module)              (NULL)
#define log_timestamp_end(module, ts, fmt, ...)
#ifndef _MSC_VER
#define _debug(fmt, args...)
#define _vdebug(fmt, args)
#define debug(module, fmt, args...)
#define mdebug(module, fmt, args...)
#define vdebug(module, fmt, args...)
#else /* _MSC_VER */
#define _debug(fmt, ...)
#define _vdebug(fmt, args)
#define debug(module, fmt, ...)
#define mdebug(module, fmt, ...)
#define vdebug(module, fmt, args)
#endif /* _MSC_VER */
#endif /* NDEBUG */

#define log(fmt, args...)            _logmsg(LogLevelNone, __FILE__, __LINE__, __func__, fmt, ## args)
#define info(fmt, args...)           _logmsg(LogLevelInfo, __FILE__, __LINE__, __func__, fmt, ## args)
#define warn(fmt, args...)           _logmsg(LogLevelWarning, __FILE__, __LINE__, __func__, fmt, ## args)
#define error(fmt, args...)          _logmsg(LogLevelError, __FILE__, __LINE__, __func__, fmt, ## args)
#define fatal(fmt, args...)          { _logmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, ## args); abort(); }
#define oassert(value, fmt, args...) { if (!(value)) { _logmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, ## args); abort(); } }
#define vlog(fmt, args)              _vlogmsg(LogLevelNone, __FILE__, __LINE__, __func__, fmt, args)
#define vinfo(fmt, args)             _vlogmsg(LogLevelInfo, __FILE__, __LINE__, __func__, fmt, args)
#define vwarn(fmt, args)             _vlogmsg(LogLevelWarning, __FILE__, __LINE__, __func__, fmt, args)
#define verror(fmt, args)            _vlogmsg(LogLevelError, __FILE__, __LINE__, __func__, fmt, args)
#define vfatal(fmt, args)            { _vlogmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, args); abort(); }
#define voassert(value, fmt, args)   { if (!(value)) { _vlogmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, args); abort(); } }

#define logging_register_module(module) logging_register_category(#module, &module ## _debug)

#ifdef __cplusplus
}
#endif

#endif /* __LOGGING_H__ */
