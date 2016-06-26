/*
 * dict.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __DICT_H__
#define __DICT_H__

#include <string.h>

#include <array.h>
#include <list.h>

typedef struct _dictentry {
  struct _dict *dict;
  void         *key;
  void         *value;
  unsigned int  hash;
  int           ix;
} dictentry_t;

typedef struct _bucket {
  struct _dict *dict;
  int           ix;
  int           capacity;
  int           size;
  dictentry_t   entries[];
} bucket_t;

typedef struct _dict {
  type_t     key_type;
  type_t     data_type;
  bucket_t **buckets;
  int        num_buckets;
  int        size;
  float      loadfactor;
  char      *str;
} dict_t;

typedef struct _entry {
  void         *key;
  void         *value;
} entry_t;

OBLCORE_IMPEXP void          entry_free(entry_t *e);

OBLCORE_IMPEXP dict_t *      dict_create(cmp_t); /* No hash - Use key point mod something */
OBLCORE_IMPEXP dict_t *      dict_clone(dict_t *);
OBLCORE_IMPEXP dict_t *      dict_copy(dict_t *);
OBLCORE_IMPEXP dict_t *      dict_set_key_type(dict_t *, type_t *);
OBLCORE_IMPEXP dict_t *      dict_set_data_type(dict_t *, type_t *);
OBLCORE_IMPEXP dict_t *      dict_set_hash(dict_t *, hash_t);
OBLCORE_IMPEXP dict_t *      dict_set_free_key(dict_t *, free_t);
OBLCORE_IMPEXP dict_t *      dict_set_free_data(dict_t *, free_t);
OBLCORE_IMPEXP dict_t *      dict_set_copy_key(dict_t *, copy_t);
OBLCORE_IMPEXP dict_t *      dict_set_copy_data(dict_t *, copy_t);
OBLCORE_IMPEXP dict_t *      dict_set_tostring_key(dict_t *, tostring_t);
OBLCORE_IMPEXP dict_t *      dict_set_tostring_data(dict_t *, tostring_t);
OBLCORE_IMPEXP void          dict_free(dict_t *);
OBLCORE_IMPEXP dict_t *      dict_clear(dict_t *);
OBLCORE_IMPEXP dict_t *      dict_put(dict_t *, void *, void *);
OBLCORE_IMPEXP int           dict_has_key(dict_t *, void *);
OBLCORE_IMPEXP void *        dict_get(dict_t *, void *);
OBLCORE_IMPEXP dict_t *      dict_remove(dict_t *, void *);
OBLCORE_IMPEXP void *        dict_pop(dict_t *, void *);
OBLCORE_IMPEXP int           dict_size(dict_t *);
OBLCORE_IMPEXP list_t *      dict_keys(dict_t *);
OBLCORE_IMPEXP list_t *      dict_values(dict_t *);
OBLCORE_IMPEXP list_t *      dict_items(dict_t *);
OBLCORE_IMPEXP int           dict_has_key(dict_t *, void *);
OBLCORE_IMPEXP void *        dict_reduce(dict_t *, reduce_t, void *);
OBLCORE_IMPEXP void *        dict_reduce_keys(dict_t *, reduce_t, void *);
OBLCORE_IMPEXP void *        dict_reduce_values(dict_t *, reduce_t, void *);
OBLCORE_IMPEXP void *        dict_reduce_chars(dict_t *, reduce_t, void *);
OBLCORE_IMPEXP void *        _dict_reduce_dictentries(dict_t *, reduce_t, void *);
OBLCORE_IMPEXP dict_t *      dict_visit(dict_t *, visit_t);
OBLCORE_IMPEXP dict_t *      dict_visit_keys(dict_t *, visit_t);
OBLCORE_IMPEXP dict_t *      dict_visit_values(dict_t *, visit_t);
OBLCORE_IMPEXP dict_t *      _dict_visit_dictentries(dict_t *, visit_t);
OBLCORE_IMPEXP dict_t *      dict_put_all(dict_t *, dict_t *);
OBLCORE_IMPEXP struct _str * dict_tostr(dict_t *);
OBLCORE_IMPEXP struct _str * dict_tostr_custom(dict_t *, char *, char *, char *, char *);
OBLCORE_IMPEXP char *        dict_tostring(dict_t *);
OBLCORE_IMPEXP char *        dict_tostring_custom(dict_t *, char *, char *, char *, char *);

OBLCORE_IMPEXP struct _str * dict_dump(dict_t *, char *);

#define intstr_dict_create()    dict_set_tostring_data( \
                                  dict_set_tostring_key( \
                                    dict_set_free_data( \
                                      dict_create(NULL),\
                                      (free_t) free), \
                                    (tostring_t) oblcore_itoa), \
                                  (tostring_t) chars)

#define strstr_dict_create()    dict_set_tostring_data( \
                                  dict_set_tostring_key( \
                                    dict_set_free_data( \
                                      dict_set_free_key( \
                                        dict_set_hash( \
                                          dict_create((cmp_t) strcmp),\
                                          (hash_t) strhash), \
                                        (free_t) free), \
                                      (free_t) free), \
                                    (tostring_t) chars), \
                                  (tostring_t) chars)

#define strvoid_dict_create()   dict_set_tostring_key( \
                                  dict_set_free_key( \
                                    dict_set_hash( \
                                      dict_create((cmp_t) strcmp),\
                                      (hash_t) strhash), \
                                    (free_t) free), \
                                  (tostring_t) chars)

#define intdict_create()        dict_set_tostring_key(dict_create(NULL), (tostring_t) oblcore_itoa)

#define dict_put_int(d, i, v)   dict_put((d), (void *)((intptr_t) (i)), (v))
#define dict_get_int(d, i)      dict_get((d), (void *)((intptr_t) (i)))
#define dict_has_int(d, i)      dict_has_key((d), (void *)((intptr_t) (i)))
#define dict_remove_int(d, i)   dict_remove((d), (void *)((intptr_t) (i)))
#define dict_empty(d)           (dict_size((d)) == 0)
#define dict_notempty(d)        (dict_size((d)) > 0)

#endif /* __DICT_H__ */
