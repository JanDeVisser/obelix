/*
 * /obelix/include/closure.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __CLOSURE_H__
#define __CLOSURE_H__

#include <data.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */
  
typedef struct _closure {
  data_t           data;
  struct _closure *up;
  script_t        *script;
  bytecode_t      *bytecode;
  data_t          *self;
  dict_t          *params;
  int              free_params;
  dict_t          *variables;
  data_t          *thread;
  int              line;
} closure_t;

/* -- C L O S U R E  P R O T O T Y P E S ---------------------------------- */

extern closure_t *      closure_create(script_t *, closure_t *, data_t *);
extern closure_t *      closure_copy(closure_t *);
extern int              closure_cmp(closure_t *, closure_t *);
extern unsigned int     closure_hash(closure_t *);
extern data_t *         closure_set(closure_t *, char *, data_t *);
extern data_t *         closure_get(closure_t *, char *);
extern int              closure_has(closure_t *, char *);
extern data_t *         closure_resolve(closure_t *, char *);
extern data_t *         closure_execute(closure_t *, array_t *, dict_t *);
extern data_t *         closure_import(closure_t *, name_t *);

#define data_is_closure(d)  ((d) && (data_type((d)) == Closure))
#define data_closureval(d)  (data_is_closure((d)) ? ((closure_t *) (d) -> ptrval) : NULL)
#define closure_copy(c)     ((closure_t *) data_copy((c)))
#define closure_tostring(c) (data_tostring((data_t *) (c)))
#define closure_free(c)     (data_free((data_t *) (c)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __CLOSURE_H__ */
