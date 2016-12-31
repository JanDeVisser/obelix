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

#ifndef OBLCORE_IMPEXP
#define OBLCORE_IMPEXP
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _log_level {
  LogLevelDebug = 0,
  LogLevelInfo,
  LogLevelWarning,
  LogLevelError,
  LogLevelFatal
} log_level_t;

#ifndef __LOGGING_C__
typedef void log_timestamp_t;
#endif

extern int core_debug;

OBLCORE_IMPEXP void   logging_init(void);
OBLCORE_IMPEXP void   logging_register_category(char *, int *);
OBLCORE_IMPEXP void   logging_reset(void);
OBLCORE_IMPEXP void   logging_enable(char *);
OBLCORE_IMPEXP void   logging_disable(char *);
OBLCORE_IMPEXP void   _vlogmsg_no_nl(log_level_t, char *, int, const char *, const char *, va_list);
OBLCORE_IMPEXP void   _vlogmsg(log_level_t, char *, int, const char *, const char *, va_list);
OBLCORE_IMPEXP void   _logmsg(log_level_t, char *, int, const char *, const char *, ...);
OBLCORE_IMPEXP int    logging_status(char *);
OBLCORE_IMPEXP int    logging_level(void);
OBLCORE_IMPEXP int    logging_set_level(char *);
OBLCORE_IMPEXP int    logging_set_file(char *);

OBLCORE_IMPEXP log_timestamp_t * _log_timestamp_start(void);
OBLCORE_IMPEXP void              _log_timestamp_end(log_timestamp_t *, char *, int, const char *, const char *, ...);

#ifndef NDEBUG
#ifndef _MSC_VER
#define _debug(fmt, args...)         _logmsg(LogLevelDebug, __FILE__, __LINE__, __func__, fmt, ## args)
#define _vdebug(fmt, args)           _vlogmsg(LogLevelDebug, __FILE__, __LINE__, __func__, fmt, args)
#define debug(module, fmt, args...)  if (module ## _debug) { _debug(fmt, ## args); }
#define mdebug(module, fmt, args...) debug(module, fmt, ##args)
#define vdebug(module, fmt, args...) if (module ## _debug) { _vdebug(fmt, args); }
#define log_timestamp_start(module)                  ((module ## _debug) ? _log_timestamp_start() : NULL)
#define log_timestamp_end(module, ts, fmt, args...)  if (module ## _debug) { _log_timestamp_end(ts, __FILE__, __LINE__, __func__, fmt, ## args); }
#else /* _MSC_VER */
#define _debug(fmt, ...)             _logmsg(LogLevelDebug, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define _vdebug(fmt, args)           _logmsg(LogLevelDebug, __FILE__, __LINE__, __FUNCTION__, fmt, args)
#define debug(module, fmt, ...)      if (module ## _debug) { _debug(fmt, __VA_ARGS__); }
#define mdebug(module, fmt, ...)     debug(module, fmt, __VA_ARGS__)
#define vdebug(module, fmt, args)    if (module ## _debug) { _debug(fmt, args); }
#define log_timestamp_start(module)                  ((module ## _debug) ? _log_timestamp_start() : NULL)
#define log_timestamp_end(module, ts, fmt, args...)  if (module ## _debug) { _log_timestamp_end(ts, __FILE__, __LINE__, __func__, fmt, __VA_ARGS__); }
#endif /* _MSC_VER */
#else /* NDEBUG */
#ifndef _MSC_VER
#define _debug(fmt, args...)
#define _vdebug(fmt, args)
#define debug(module, fmt, args...)
#define mdebug(module, fmt, args...)
#define vdebug(module, fmt, args...)
#define log_timestamp_start(module)                   (NULL)
#define log_timestamp_end(module, ts, fmt, args...)
#else /* _MSC_VER */
#define _debug(fmt, ...)
#define _vdebug(fmt, args)
#define debug(module, fmt, ...)
#define mdebug(module, fmt, ...)
#define vdebug(module, fmt, args)
#define log_timestamp_start(module)                   (NULL)
#define log_timestamp_end(module, ts, fmt, ...)
#endif /* _MSC_VER */
#endif /* NDEBUG */

#ifndef _MSC_VER
#define info(fmt, args...)           _logmsg(LogLevelInfo, __FILE__, __LINE__, __func__, fmt, ## args)
#define warn(fmt, args...)           _logmsg(LogLevelWarning, __FILE__, __LINE__, __func__, fmt, ## args)
#define error(fmt, args...)          _logmsg(LogLevelError, __FILE__, __LINE__, __func__, fmt, ## args)
#define fatal(fmt, args...)          { _logmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, ## args); abort(); }
#define oassert(value, fmt, args...) { if (!(value)) { _logmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, ## args); abort(); } }
#define vinfo(fmt, args)             _vlogmsg(LogLevelInfo, __FILE__, __LINE__, __func__, fmt, args)
#define vwarn(fmt, args)             _vlogmsg(LogLevelWarning, __FILE__, __LINE__, __func__, fmt, args)
#define verror(fmt, args)            _vlogmsg(LogLevelError, __FILE__, __LINE__, __func__, fmt, args)
#define vfatal(fmt, args)            { _vlogmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, args); abort(); }
#define voassert(value, fmt, args)   { if (!(value)) { _vlogmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, args); abort(); } }
#else /* _MSC_VER */
#define info(fmt, ...)               _logmsg(LogLevelInfo, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define warn(fmt, ...)               _logmsg(LogLevelWarning, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define error(fmt, ...)              _logmsg(LogLevelError, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define fatal(fmt, ...)              { _logmsg(LogLevelFatal, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__); exit(-10); }
#define oassert(value, fmt, ...)     { if (!(value)) { _logmsg(LogLevelFatal, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__); assert(0); } }
#define vinfo(fmt, args)             _logmsg(LogLevelInfo, __FILE__, __LINE__, __FUNCTION__, fmt, args)
#define vwarn(fmt, args)             _logmsg(LogLevelWarning, __FILE__, __LINE__, __FUNCTION__, fmt, args)
#define verror(fmt, args)            _logmsg(LogLevelError, __FILE__, __LINE__, __FUNCTION__, fmt, args)
#define vfatal(fmt, args)            { _logmsg(LogLevelFatal, __FILE__, __LINE__, __FUNCTION__, fmt, args); exit(-10); }
#define voassert(value, fmt, ...)    { if (!(value)) { _logmsg(LogLevelFatal, __FILE__, __LINE__, __FUNCTION__, fmt, args); assert(0); } }
#endif /* _MSC_VER */

#define logging_register_module(module) logging_register_category(#module, &module ## _debug)

#ifdef __cplusplus
}
#endif

#endif /* __LOGGING_H__ */
