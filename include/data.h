/*
 * /obelix/include/data.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __DATA_H__
#define __DATA_H__

#include <core.h>

typedef struct _typedescr {
  int           type;
  new_t         new;
  copydata_t    copy;
  cmp_t         cmp;
  free_t        free;
  tostring_t    tostring;
  parse_t       parse;
  hash_t        hash;
} typedescr_t;

typedef enum _datatype {
  Pointer, String, Int, Float, Bool, List, Function
} datatype_t;

typedef struct _data {
  datatype_t type;
  union {
    void      *ptrval;
    long       intval;
    double     dblval;
  };
} data_t;

extern int            datatype_register(typedescr_t *);
extern data_t *       data_create(int, ...);
extern data_t *       data_create_pointer(void *);
extern data_t *       data_null(void);
extern data_t *       data_create_int(long);
extern data_t *       data_create_float(double);
extern data_t *       data_create_bool(long);
extern data_t *       data_create_string(char *);
extern data_t *       data_create_pointer(void *);
extern data_t *       data_create_function(function_t *);
extern data_t *       data_parse(int, char *);
extern void           data_free(data_t *);
extern int            data_type(data_t *);
extern int            data_is_numeric(data_t *);
extern data_t *       data_copy(data_t *);
extern unsigned int   data_hash(data_t *);
extern char *         data_tostring(data_t *);
extern int            data_cmp(data_t *, data_t *);

#endif /* __DATA_H__ */
