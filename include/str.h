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

#include <core.h>
#include <data.h>

typedef struct _str {
  data_t  _d;
  char   *buffer;
  int     pos;
  int     len;
  int     bufsize;
} str_t;

typedef str_t stringbuffer_t;
struct _array;
struct _data;

/*
 * Constructor functions:
 */
extern str_t *     str_wrap(char *);
extern str_t *     str_copy_chars(char *, ...);
extern str_t *     str_copy_vchars(char *, va_list);
extern str_t *     str_copy_nchars(int, char *);
extern str_t *     str_deepcopy(str_t *);
extern str_t *     str_create(int);
extern void        str_free(str_t *);
extern char *      str_reassign(str_t *);

/*
 * Functions returning new strings:
 */
extern str_t *      str_slice(str_t *, int, int);
extern str_t *      _str_join(char *, void *, obj_reduce_t);

/*
 * Functions manipulating strings:
 */
extern str_t *      str_append_char(str_t *, int);
extern str_t *      str_append_chars(str_t *, char *, ...);
extern str_t *      str_append_vchars(str_t *, char *, va_list);
extern str_t *      str_append_nchars(str_t *, char *, int);
extern str_t *      str_append(str_t *, str_t *);
extern str_t *      str_chop(str_t *, int);
extern str_t *      str_lchop(str_t *, int);
extern str_t *      str_erase(str_t *);
extern str_t *      str_set(str_t *, int, int);
extern str_t *      str_forcecase(str_t *, int);

/*
 * Functions returning characteristics of strings:
 */
extern int             str_len(str_t *);
extern char *          str_chars(str_t *);
extern int             str_at(str_t *, int i);
extern unsigned int    str_hash(str_t *);
extern int             str_cmp(str_t *, str_t *);
extern int             str_cmp_chars(str_t *, char *);
extern int             str_ncmp(str_t *, str_t *, int);
extern int             str_ncmp_chars(str_t *, char *, int);
extern int             str_indexof(str_t *, str_t *);
extern int             str_indexof_chars(str_t *, char *);
extern int             str_rindexof(str_t *, str_t *);
extern int             str_rindexof_chars(str_t *, char *);
extern struct _array * str_split(str_t *, char *);

extern int             str_read(str_t *, char *, int);
extern int             str_readchar(str_t *);
extern int             str_readinto(str_t *, struct _data *);
extern int             str_pushback(str_t *, int);
extern int             str_write(str_t *, char *, int);

extern str_t *         str_format(char *, array_t *, dict_t *);

#define str_toupper(s)    (str_forcecase((s), 1))
#define str_tolower(s)    (str_forcecase((s), 0))
#define str_join(g,c,r)   _str_join((g), (c), (obj_reduce_t) (r))

#define data_is_string(d) ((d) && (data_hastype((d), String)))
#define data_as_string(d) ((str_t *) (data_is_string((d)) ? ((str_t *) (d)) : NULL))
#define str_free(s)       (data_free((data_t *) (s)))
#define str_tostring(s)   (data_tostring((data_t *) (s)))
#define str_copy(s)       ((str_t *) data_copy((data_t *) (s)))

#endif /* __STR_H__ */
