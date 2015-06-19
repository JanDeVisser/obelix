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
#define	__LOGGING_H__

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum _log_level {
  LogLevelDebug = 0,
  LogLevelInfo,
  LogLevelWarning,
  LogLevelError,
  LogLevelFatal
} log_level_t;
  
extern void   logging_register_category(char *, int *);
extern void   logging_reset(void);
extern void   logging_enable(char *);
extern void   logging_disable(char *);
extern void   _logmsg(log_level_t, const char *, int, const char *, const char *, ...);
extern int    logging_status(char *);

extern log_level_t log_level;

#ifndef NDEBUG
#define debug(fmt, args...)          _logmsg(LogLevelDebug, __FILE__, __LINE__, __PRETTY_FUNCTION__, fmt, ## args)
#else /* NDEBUG */
#define debug(fmt, args...)
#endif /* NDEBUG */
#define info(fmt, args...)           _logmsg(LogLevelInfo, __FILE__, __LINE__, __PRETTY_FUNCTION__, fmt, ## args)
#define warning(fmt, args...)        _logmsg(LogLevelWarning, __FILE__, __LINE__, __PRETTY_FUNCTION__, fmt, ## args)
#define error(fmt, args...)          _logmsg(LogLevelError, __FILE__, __LINE__, __PRETTY_FUNCTION__, fmt, ## args)
#define fatal(fmt, args...)          { _logmsg(LogLevelFatal, __FILE__, __LINE__, __PRETTY_FUNCTION__, fmt, ## args); *((char *) NULL) = 0; }
#define oassert(value, fmt, args...) { if (!(value)) { _logmsg(LogLevelFatal, __FILE__, __LINE__, __PRETTY_FUNCTION__, fmt, ## args); assert(0); } }

#ifdef	__cplusplus
}
#endif

#endif	/* __LOGGING_H__ */

