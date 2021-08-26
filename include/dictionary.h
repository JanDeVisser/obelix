/*
 * /obelix/include/dictionary.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include <data.h>
#include <dict.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void            dictionary_init(void);
extern dictionary_t *  dictionary_create(void *);
extern dictionary_t *  dictionary_create_from_dict(dict_t *);

extern data_t *        dictionary_get(const dictionary_t *, const char *);
extern data_t *        dictionary_pop(dictionary_t *, const char *);
extern data_t *        _dictionary_set(dictionary_t *, const char *, data_t *);
extern int             dictionary_has(const dictionary_t *, const char *);
extern int             dictionary_size(const dictionary_t *);
extern data_t *        _dictionary_reduce(dictionary_t *, reduce_t, void *);
extern data_t *        _dictionary_reduce_keys(dictionary_t *, reduce_t, void *);
extern data_t *        _dictionary_reduce_values(dictionary_t *, reduce_t, void *);
extern void            _dictionary_visit(dictionary_t *, visit_t);
extern dictionary_t *  dictionary_update(dictionary_t *, dictionary_t *);

extern int Dictionary;

type_skel(dictionary, Dictionary, dictionary_t);

static inline void * dictionary_set(dictionary_t *dict, const char *key, void *value) {
  return _dictionary_set(dict, key, data_as_data(value));
}

static inline void dictionary_remove(dictionary_t *dict, const char *key) {
  (void) dictionary_pop(dict, key);
}

static inline dictionary_t * dictionary_clear(dictionary_t *dict) {
  dict_clear(dict -> attributes);
  return dict;
}

static inline char * dictionary_value_tostring(const dictionary_t *dict, const char *key) {
  return data_tostring(dictionary_get(dict, key));
}

#define dictionary_reduce(d, r, i)         (_dictionary_reduce((d), (reduce_t) (r), (i)))
#define dictionary_reduce_keys(d, r, i)    (_dictionary_reduce_keys((d), (reduce_t) (r), (i)))
#define dictionary_reduce_values(d, r, i)  (_dictionary_reduce_values((d), (reduce_t) (r), (i)))
#define dictionary_visit(d, v)             (_dictionary_visit((d), (visit_t) (v)))

#ifdef __cplusplus
}
#endif

#endif /* __DICTIONARY_H__ */
