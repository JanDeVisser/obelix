/*
 * /obelix/include/re.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __RE_H__
#define __RE_H__

#include <data.h>
#include <regex.h>
#include <stdarg.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _re {
  data_t   _d;
  regex_t  compiled;
  char    *pattern;
  char    *flags;
} re_t;

OBLCORE_IMPEXP re_t *   regexp_create(char *, char *);
OBLCORE_IMPEXP re_t *   regexp_vcreate(va_list);
OBLCORE_IMPEXP int      regexp_cmp(re_t *, re_t *);
OBLCORE_IMPEXP data_t * regexp_match(re_t *, char *);
OBLCORE_IMPEXP data_t * regexp_replace(re_t *, char *, array_t *);

OBLCORE_IMPEXP int Regexp;

#define data_is_regexp(d)   ((d) && data_hastype((data_t *) (d), Regexp))
#define data_as_regexp(d)   ((re_t *) (data_is_regexp((d)) ? (d) : NULL))
#define regexp_free(re)     (data_free((data_t *) (re)))
#define regexp_tostring(re) (data_tostring((data_t *) (re)))
#define regexp_copy(re)     ((re_t *) data_copy((data_t *) (re)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __RE_H__ */
