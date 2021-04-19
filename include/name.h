/*
 * /obelix/include/name.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __NAME_H__
#define	__NAME_H__

#include <stdarg.h>

#include <array.h>
#include <data.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern name_t *          name_create(int, ...);
extern name_t *          name_vcreate(int, va_list);
extern name_t *          name_split(char *, char *);
extern name_t *          name_parse(char *);
extern name_t *          name_deepcopy(name_t *);
extern int               name_size(name_t *);
extern char *            name_first(name_t *);
extern char *            name_last(name_t *);
extern char *            name_get(name_t *, int);
extern array_t *         name_as_array(name_t *);
extern name_t *          name_tail(name_t *);
extern name_t *          name_head(name_t *);
extern char *            name_tostring_sep(name_t *, char *);
extern name_t *          name_extend(name_t *, char *);
extern name_t *          name_extend_data(name_t *, struct _data *);
extern name_t *          name_append(name_t *, name_t *);
extern name_t *          name_append_array(name_t *, array_t *);
extern name_t *          name_append_data_array(name_t *, array_t *);
extern int               name_cmp(name_t *, name_t *);
extern int               name_startswith(name_t *, name_t *);
extern unsigned int      name_hash(name_t *);

extern int Name;

type_skel(name, Name, name_t);

/* ------------------------------------------------------------------------ */

typedef struct _hierarchy {
  data_t             _d;
  char              *label;
  data_t            *data;
  struct _hierarchy *up;
  list_t            *branches;
} hierarchy_t;

extern hierarchy_t *     hierarchy_create(void);
extern hierarchy_t *     hierarchy_insert(hierarchy_t *, name_t *, data_t *);
extern hierarchy_t *     hierarchy_append(hierarchy_t *, char *, data_t *);
extern hierarchy_t *     hierarchy_remove(hierarchy_t *, name_t *);
extern hierarchy_t *     hierarchy_get_bylabel(hierarchy_t *, char *);
extern hierarchy_t *     hierarchy_get(hierarchy_t *, int);
extern hierarchy_t *     hierarchy_find(hierarchy_t *, name_t *);
extern hierarchy_t *     hierarchy_match(hierarchy_t *, name_t *, int *);
extern name_t *          hierarchy_name(hierarchy_t *);
extern int               hierarchy_depth(hierarchy_t *);
extern hierarchy_t *     hierarchy_root(hierarchy_t *);

extern int Hierarchy;

type_skel(hierarchy, Hierarchy, hierarchy_t);


#ifdef	__cplusplus
}
#endif

#endif	/* __NAME_H__ */
