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

#ifdef __cplusplus
extern "C" {
#endif

extern int lexa_debug;
extern int Lexa;

typedef void (*tokenfilter_t)(token_t *);

typedef struct _lexa {
  data_t          _d;
  char           *debug;
  char           *log_level;
  dictionary_t   *scanners;
  lexer_config_t *config;
  data_t         *stream;
  int             tokens;
  dict_t         *tokens_by_type;
  tokenfilter_t   tokenfilter;
} lexa_t;

extern lexa_t *           lexa_create(void);
extern lexa_t *           lexa_build_lexer(lexa_t *);
extern lexa_t *           lexa_add_scanner(lexa_t *, const char *);
extern scanner_config_t * lexa_get_scanner(lexa_t *, const char *);
extern lexa_t *           lexa_set_config_value(lexa_t *, const char *, const char *);
extern lexa_t *           lexa_debug_settings(lexa_t *);
extern lexa_t *           lexa_tokenize(lexa_t *);
extern int                lexa_tokens_with_code(lexa_t *, unsigned int);
extern lexa_t *           lexa_set_stream(lexa_t *, data_t *);
extern lexa_t *           lexa_set_tokenfilter(lexa_t *, tokenfilter_t);

#ifdef __cplusplus
}
#endif

#endif /* __LEXA_H__ */
