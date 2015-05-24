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

#include <regex.h>
#include <stdarg.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _re {
  regex_t  compiled;
  char    *pattern;
  char    *flags;
  int      refs;
  char    *str;
} re_t;

extern re_t *   regexp_create(char *, char *);
extern re_t *   regexp_vcreate(va_list);
extern void     regexp_free(re_t *);
extern re_t *   regexp_copy(re_t *);
extern int      regexp_cmp(re_t *, re_t *);
extern char *   regexp_tostring(re_t *);
extern data_t * regexp_match(re_t *, char *);
extern data_t * regexp_replace(re_t *, char *, array_t *);

extern int Regexp;

#define data_is_regexp(d)  ((d) && (data_type((d)) == Regexp))
#define data_regexpval(d)  ((re_t *) (data_is_regexp((d)) ? ((d) -> ptrval) : NULL))
  
#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __RE_H__ */
