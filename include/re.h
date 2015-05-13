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
  char    *pattern;
  regex_t  compiled;
  char     flags[5];
  int      refs;
  char    *str;
} re_t;
  
extern re_t *   re_create(va_list);
extern void     re_free(re_t *);
extern re_t *   re_copy(re_t *);
extern int      re_cmp(re_t *, re_t *);
extern char *   re_tostring(re_t *);
extern data_t * re_match(re_t *, char *);
extern data_t * re_replace(re_t *, char *, array_t *);

extern int Regex;

#define data_is_regex(d)  ((d) && (data_type((d)) == Regex))
#define data_regexval(d)  ((re_t *) (data_is_regex((d)) ? ((d) -> ptrval) : NULL))
  
#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __RE_H__ */
