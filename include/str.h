/*
 * /obelix/src/str.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __STR_H__
#define __STR_H__

#include <limits.h>
#include <core.h>
#include <data.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _str {
  data_t  _d;
  char   *buffer;
  size_t  pos;
  size_t  len;
  size_t  bufsize;
} str_t;

typedef str_t stringbuffer_t;
struct _array;
struct _data;

/*
 * Constructor functions:
 */
extern str_t *         str_wrap(const char *);
extern str_t *         str_adopt(char *);
extern str_t *         str_copy_chars(const char *);
extern str_t *         str_copy_nchars(const char *, size_t);
extern str_t *         str_from_data(data_t *);
extern str_t *         str_printf(const char *, ...);
extern str_t *         str_vprintf(const char *, va_list);
extern str_t *         str_duplicate(const str_t *);
extern str_t *         str_create(size_t);
extern str_t *         str_free(str_t *);
extern char *          str_reassign(str_t *);

static inline str_t * str_deepcopy(const str_t *str) {
  return str_duplicate(str);
}

/*
 * Functions returning new strings:
 */
extern str_t *         str_slice(const str_t *, int, int);
extern str_t *         _str_join(const char *, const void *, obj_reduce_t);

/*
 * Functions manipulating strings:
 */
extern str_t *         str_append_char(str_t *, int);
extern str_t *         str_append_chars(str_t *, const char *);
extern str_t *         str_append_nchars(str_t *, const char *, size_t);
extern str_t *         str_append_printf(str_t *, const char *, ...);
extern str_t *         str_append_vprintf(str_t *, const char *, va_list);
extern str_t *         str_append(str_t *, const str_t *);
extern str_t *         str_chop(str_t *, int);
extern str_t *         str_lchop(str_t *, int);
extern str_t *         str_erase(str_t *);
extern str_t *         str_set(str_t *, size_t, int);
extern str_t *         str_forcecase(str_t *, int);
extern int             str_replace(str_t *, const char *, const char *, int);
/*
 * Functions returning characteristics of strings:
 */
extern int             str_is_null(const str_t *);
extern int             str_is_static(const str_t *);
extern size_t          str_len(const str_t *);
extern char *          str_chars(const str_t *);
extern int             str_at(const str_t *, size_t);
extern unsigned int    str_hash(const str_t *);
extern int             str_cmp(const str_t *, const str_t *);
extern int             str_cmp_chars(const str_t *, const char *);
extern int             str_ncmp(const str_t *, const str_t *, size_t);
extern int             str_ncmp_chars(const str_t *, const char *, size_t);
extern int             str_indexof(const str_t *, const str_t *);
extern int             str_indexof_chars(const str_t *, const char *);
extern int             str_rindexof(const str_t *, const str_t *);
extern int             str_rindexof_chars(const str_t *, const char *);
extern struct _array * str_split(const str_t *, const char *);

extern int             str_rewind(str_t *);
extern int             str_read(str_t *, char *, size_t);
extern int             str_peek(const str_t *);
extern int             str_readchar(str_t *);
extern int             str_readinto(str_t *, struct _data *);
extern int             str_read_from_stream(str_t *, void *, read_t);
extern int             str_fillup(str_t *, data_t *);
extern int             str_replenish(str_t *, data_t *);
extern int             str_skip(str_t *, int);
extern int             str_pushback(str_t *, size_t);
extern int             str_write(str_t *, char *, size_t);
extern str_t *         str_reset(str_t *);

extern str_t *         str_format(const char *, const arguments_t *);
extern str_t *         str_vformatf(const char *fmt, va_list args);
extern str_t *         str_formatf(const char *fmt, ...);

#define str_toupper(s)         (str_forcecase((s), 1))
#define str_tolower(s)         (str_forcecase((s), 0))
#define str_join(g,c,r)        _str_join((g), (c), (obj_reduce_t) (r))
#define str_replace_one(s,p,r) (str_replace((str_t *) (s), (p), (r), 1))
#define str_replace_all(s,p,r) (str_replace((str_t *) (s), (p), (r), INT_MAX))

#define data_is_string(d)      ((d) && data_is_data(d) && data_hastype((d), String))
#define data_as_string(d)      ((str_t *) (data_is_string((d)) ? ((str_t *) (d)) : NULL))
#define str_tostring(s)        (data_tostring((data_t *) (s)))
#define str_copy(s)            ((str_t *) data_copy((data_t *) (s)))
#define str_to_data(s)         ((data_t *) str_copy_chars((s)))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STR_H__ */
