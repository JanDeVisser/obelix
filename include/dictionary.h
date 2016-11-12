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

typedef struct _dictionary {
  data_t   _d;
  dict_t  *attributes;
} dictionary_t;

OBLCORE_IMPEXP dictionary_t *  dictionary_create(data_t *);
OBLCORE_IMPEXP data_t *        dictionary_get(dictionary_t *, char *);
OBLCORE_IMPEXP data_t *        dictionary_set(dictionary_t *, char *, data_t *);
OBLCORE_IMPEXP int             dictionary_has(dictionary_t *, char *);
OBLCORE_IMPEXP data_t *        dictionary_resolve(dictionary_t *, char *);

#define data_is_dictionary(d)  ((d) && (data_hastype((data_t *) (d), Dictionary)))
#define data_as_dictionary(d)  ((dictionary_t *) (data_is_dictionary((data_t *) (d)) ? (d) : NULL))
#define dictionary_free(s)     (data_free((data_t *) (s)))
#define dictionary_tostring(s) (data_tostring((data_t *) (s)))
#define dictionary_debugstr(s) (data_tostring((data_t *) (s)))
#define dictionary_copy(s)     ((dictionary_t *) data_copy((data_t *) (s)))

OBLCORE_IMPEXP int Dictionary;

#ifdef __cplusplus
}
#endif

#endif /* __DICTIONARY_H__ */
