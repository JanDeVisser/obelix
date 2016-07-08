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

#include <core-setup.h>

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
  
OBLCORE_IMPEXP void   logging_register_category(char *, int *);
OBLCORE_IMPEXP void   logging_reset(void);
OBLCORE_IMPEXP void   logging_enable(char *);
OBLCORE_IMPEXP void   logging_disable(char *);
OBLCORE_IMPEXP void   _logmsg(log_level_t, const char *, int, const char *, const char *, ...);
OBLCORE_IMPEXP int    logging_status(char *);
OBLCORE_IMPEXP int    logging_level(void);
OBLCORE_IMPEXP int    logging_set_level(log_level_t);

#ifndef NDEBUG
#ifndef _MSC_VER
#define debug(fmt, args...)            _logmsg(LogLevelDebug, __FILE__, __LINE__, __func__, fmt, ## args)
#define mdebug(module, fmt, args...)   \
  if (module ## _debug) { \
    _logmsg(LogLevelDebug, __FILE__, __LINE__, __func__, fmt, ## args); \
  }
#else /* _MSC_VER */
#define debug(fmt, ...)              _logmsg(LogLevelDebug, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define mdebug(module, fmt, args...)   \
  if (module ## _debug) { \
    _logmsg(LogLevelDebug, __FILE__, __LINE__, __func__, fmt, __VA_ARGS__); \
  }
#endif /* _MSC_VER */
#else /* NDEBUG */
#define debug(fmt, ...)
#endif /* NDEBUG */

#ifndef _MSC_VER
#define info(fmt, args...)           _logmsg(LogLevelInfo, __FILE__, __LINE__, __func__, fmt, ## args)
#define warn(fmt, args...)           _logmsg(LogLevelWarning, __FILE__, __LINE__, __func__, fmt, ## args)
#define error(fmt, args...)          _logmsg(LogLevelError, __FILE__, __LINE__, __func__, fmt, ## args)
#define fatal(fmt, args...)          { _logmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, ## args); exit(-10); }
#define oassert(value, fmt, args...) { if (!(value)) { _logmsg(LogLevelFatal, __FILE__, __LINE__, __func__, fmt, ## args); assert(0); } }
#else /* _MSC_VER */
#define info(fmt, ...)               _logmsg(LogLevelInfo, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define warn(fmt, ...)               _logmsg(LogLevelWarning, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define error(fmt, ...)              _logmsg(LogLevelError, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define fatal(fmt, ...)              { _logmsg(LogLevelFatal, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__); exit(-10); }
#define oassert(value, fmt, ...)     { if (!(value)) { _logmsg(LogLevelFatal, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__); assert(0); } }
#endif /* _MSC_VER */

#ifdef __cplusplus
}
#endif

#endif /* __LOGGING_H__ */

