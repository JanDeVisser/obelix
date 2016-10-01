/*
 * obelix/include/lexa.h - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LEXA_H__
#define __LEXA_H__

#include <data.h>
#include <lexer.h>

extern int lexa_debug;
extern int Lexa;

typedef struct _lexa {
  data_t           _d;
  char            *debug;
  log_level_t     log_level;
  dict_t          *scanners;
  lexer_config_t  *config;
  data_t          *stream;
  int              tokens;
  dict_t          *tokens_by_type;
  void           (*tokenfilter)(token_t *);
} lexa_t;

extern lexa_t * lexa_create(void);
extern lexa_t * lexa_build_lexer(lexa_t *);
extern lexa_t * lexa_add_scanner(lexa_t *, char *);
extern lexa_t * lexa_append_config_value(lexa_t *, char *, char *);
extern lexa_t * lexa_debug_settings(lexa_t *);
extern lexa_t * lexa_tokenize(lexa_t *);
extern int      lexa_tokens_with_code(lexa_t *, token_code_t);
extern lexa_t * lexa_set_stream(lexa_t *, data_t *);
extern lexa_t * lexa_set_tokenfilter(lexa_t *, void (*)(token_t *));

#define data_is_lexa(d)           ((d) && data_hastype((d), Lexa))
#define data_as_lexa(d)           (data_is_lexa((d)) ? ((lexa_t *) (d)) : NULL)
#define lexa_copy(l)              ((lexa_t *) data_copy((data_t *) (l)))
#define lexa_free(l)              (data_free((data_t *) (l)))
#define lexa_tostring(l)          (data_tostring((data_t *) (l)))

#endif /* __LEXA_H__ */
