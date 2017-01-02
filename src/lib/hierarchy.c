/*
 * /obelix/src/lib/name.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <stdio.h>
#include <string.h>

#include "libcore.h"
#include <data.h>
#include <name.h>
#include <nvp.h>
#include <str.h>

/* ------------------------------------------------------------------------ */

static hierarchy_t * _hierarchy_new(hierarchy_t *, va_list);
static unsigned long _hierarchy_hash(hierarchy_t *);
static int           _hierarchy_cmp(hierarchy_t *, hierarchy_t *);
static int           _hierarchy_size(hierarchy_t *);
static data_t *      _hierarchy_iter(hierarchy_t *);
static void          _hierarchy_free(hierarchy_t *);
static char *        _hierarchy_tostring(hierarchy_t *);
static data_t *      _hierarchy_resolve(hierarchy_t *, char *);

static void          _hierarchy_get_nodes(hierarchy_t *, array_t *);

static data_t *      _hierarchy_append(hierarchy_t *, char *, array_t *, dict_t *);
static data_t *      _hierarchy_insert(hierarchy_t *, char *, array_t *, dict_t *);
static data_t *      _hierarchy_find(hierarchy_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_Hierarchy[] = {
  { .id = FunctionNew,         .fnc = (void_t) _hierarchy_new},
  { .id = FunctionCmp,         .fnc = (void_t) _hierarchy_cmp},
  { .id = FunctionFree,        .fnc = (void_t) _hierarchy_free },
  { .id = FunctionAllocString, .fnc = (void_t) _hierarchy_tostring },
  { .id = FunctionHash,        .fnc = (void_t) _hierarchy_hash },
  { .id = FunctionResolve,     .fnc = (void_t) _hierarchy_resolve },
  { .id = FunctionLen,         .fnc = (void_t) _hierarchy_size },
  { .id = FunctionIter,        .fnc = (void_t) _hierarchy_iter },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_Hierarchy[] = {
  { .type = -1,     .name = "append", .method = (method_t) _hierarchy_append, .argtypes = { String, Any, Any },       .minargs = 2, .varargs = 0 },
  { .type = -1,     .name = "insert", .method = (method_t) _hierarchy_insert, .argtypes = { Any, Any, Any },          .minargs = 2, .varargs = 0 },
  { .type = -1,     .name = "find",   .method = (method_t) _hierarchy_find,   .argtypes = { Any, Any, Any },          .minargs = 2, .varargs = 0 },
  { .type = NoType, .name = NULL,     .method = NULL,                         .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

/* ------------------------------------------------------------------------ */

int Hierarchy = -1;
int hierarchy_debug = 0;

/* ----------------------------------------------------------------------- */

void hierarchy_init(void) {
  if (Hierarchy < 1) {
    name_init();
    logging_register_module(hierarchy);
    _methods_Hierarchy[1].argtypes[0] = Name;
    _methods_Hierarchy[2].argtypes[0] = Name;
    typedescr_register_with_methods(Hierarchy, hierarchy_t);
  }
}

/* -- H I E R A R C H Y --------------------------------------------------- */

hierarchy_t * _hierarchy_new(hierarchy_t *hierarchy, va_list args) {
  char   *label = va_arg(args, char *);
  data_t *data = NULL;
  hierarchy_t *up = NULL;

  if (label) {
    data = va_arg(args, data_t *);
    up = va_arg(args, hierarchy_t *);
    hierarchy -> label = strdup(label);
  } else {
    hierarchy -> label = strdup("/");
  }
  hierarchy -> data = data_copy(data);
  hierarchy -> up = hierarchy_copy(up);
  hierarchy -> branches = data_list_create();
  return hierarchy;
};

unsigned long _hierarchy_hash(hierarchy_t *hierarchy) {
  return strhash(hierarchy -> label);
}

int _hierarchy_size(hierarchy_t *hierarchy) {
  return list_size(hierarchy -> branches);
}

int _hierarchy_cmp(hierarchy_t *l1, hierarchy_t *l2) {
  return strcmp(l1 -> label, l2 -> label);
}

void _hierarchy_free(hierarchy_t *hierarchy) {
  if (hierarchy) {
    free(hierarchy -> label);
    data_free(hierarchy -> data);
    hierarchy_free(hierarchy -> up);
    list_free(hierarchy -> branches);
  }
}

data_t * _hierarchy_iter(hierarchy_t *hierarchy) {
  array_t *nodes;
  data_t  *iter;
  data_t  *list;

  nodes = array_create(4);
  _hierarchy_get_nodes(hierarchy, nodes);
  list = data_create_list(nodes);
  iter = data_iter(list);
  data_free(list);
  return iter;
}

char * _hierarchy_tostring(hierarchy_t *hierarchy) {
  char *buf;

  asprintf(&buf, "%s/%s", hierarchy -> label, list_tostring(hierarchy -> branches));
  return buf;
}

data_t * _hierarchy_resolve(hierarchy_t *hierarchy, char *name) {
  hierarchy_t *branch;
  long         l;

  if ((branch = hierarchy_get_bylabel(hierarchy, name))) {
    return data_copy((data_t *) branch);
  } else if (!strtoint(name, &l) && (l >= 0) && (l < list_size(hierarchy -> branches))) {
    return data_copy((data_t *) hierarchy_get(hierarchy, l));
  } else if (!strcmp(name, "up")) {
    return data_copy((data_t *) hierarchy -> up);
  } else if (!strcmp(name, "depth")) {
    return int_to_data(hierarchy_depth(hierarchy));
  } else if (!strcmp(name, "root")) {
    return data_copy((data_t *) hierarchy_root(hierarchy));
  }
  return NULL;
}

/* -- H I E R A R C H Y  A P I ---------------------------------------------*/

void _hierarchy_get_nodes(hierarchy_t *hierarchy, array_t *nodes) {
  listiterator_t *iter;
  hierarchy_t    *branch;

  for (iter = li_create(hierarchy -> branches); li_has_next(iter); ) {
    branch = (hierarchy_t *) li_next(iter);
    if (branch -> data) {
      array_push(nodes,
          nvp_create((data_t *) hierarchy_name(branch), data_copy(branch -> data)));
    }
    _hierarchy_get_nodes(branch, nodes);
  }
}

hierarchy_t * hierarchy_create(void) {
  hierarchy_init();
  return (hierarchy_t *) data_create(Hierarchy, NULL);
}

hierarchy_t * hierarchy_append(hierarchy_t *hierarchy, char *label, data_t *data) {
  hierarchy_t *ret;

  ret = (hierarchy_t *) data_create(Hierarchy, label, data, hierarchy);
  list_append(hierarchy -> branches, ret);
  return ret;
}

hierarchy_t * hierarchy_insert(hierarchy_t *hierarchy, name_t *name, data_t *data) {
  hierarchy_t *up = NULL;
  char        *label;
  int          ix;
  int          sz = name_size(name);

  for (ix = 0; ix < sz; ix++) {
    label = name_get(name, ix);
    up = hierarchy;
    hierarchy = (hierarchy_t *) hierarchy_get_bylabel(up, label);
    if (!hierarchy) {
      hierarchy = hierarchy_append(up, label, NULL);
    }
  }
  data_free(hierarchy -> data);
  hierarchy -> data = data_copy(data);
  return hierarchy;
}

hierarchy_t * hierarchy_remove(hierarchy_t *hierarchy, name_t *name) {
  hierarchy_t    *leaf;
  hierarchy_t    *up;
  hierarchy_t    *ret = NULL;
  listiterator_t *li;

  if (!name_size(name)) {
    return NULL;
  }
  leaf = hierarchy_find(hierarchy, name);
  if (leaf) {
    up = leaf -> up;
    for (li = li_create(up -> branches); !ret && li_has_next(li); ) {
      if (leaf == li_next(li)) {
        li_remove(li);
        ret = hierarchy;
      }
    }
    li_free(li);
  } else {
    return NULL;
  }
  return ret;
}

hierarchy_t * hierarchy_get_bylabel(hierarchy_t *hierarchy, char *label) {
  hierarchy_t    *branch;
  hierarchy_t    *ret = NULL;
  listiterator_t *li;


  for (li = li_create(hierarchy -> branches); !ret && li_has_next(li); ) {
    branch = (hierarchy_t *) li_next(li);
    if (!strcmp(label, branch -> label)) {
      ret = branch;
    }
  }
  li_free(li);
  return ret;
}

hierarchy_t * hierarchy_get(hierarchy_t *hierarchy, int ix) {
  return ((ix >= 0) && (ix < list_size(hierarchy -> branches)))
    ? (hierarchy_t *) list_get(hierarchy -> branches, ix)
    : NULL;
}

hierarchy_t * hierarchy_root(hierarchy_t *hierarchy) {
  return (!hierarchy -> up) ? hierarchy : hierarchy_root(hierarchy -> up);
}

int hierarchy_depth(hierarchy_t *hierarchy) {
  int ret;

  for (ret = 0; hierarchy; ret++) {
    hierarchy = hierarchy -> up;
  }
  return ret;
}

name_t * hierarchy_name(hierarchy_t *hierarchy) {
  name_t  *ret = name_create(0);
  int      ix = hierarchy_depth(hierarchy);
  array_t *labels = array_create(ix + 1);

  for (; hierarchy; ix--, hierarchy = hierarchy -> up) {
    array_set(labels, ix, hierarchy -> label);
  }
  name_append_array(ret, labels);
  array_free(labels);
  return ret;
}

hierarchy_t * hierarchy_find(hierarchy_t *hierarchy, name_t *name) {
  hierarchy_t *up = NULL;
  char        *label;
  int          ix;
  int          sz = name_size(name);

  for (ix = 0; ix < sz; ix++) {
    label = name_get(name, ix);
    up = hierarchy;
    hierarchy = (hierarchy_t *) hierarchy_get_bylabel(up, label);
    if (!hierarchy) {
      return NULL;
    }
  }
  return hierarchy;
}

hierarchy_t * hierarchy_match(hierarchy_t *hierarchy, name_t *name, int *match_lost) {
  hierarchy_t *up = NULL;
  char        *label;
  int          ix;
  int          sz = name_size(name);

  debug(hierarchy, "hierarchy: '%s' name: '%s'", hierarchy_tostring(hierarchy), name_tostring(name));
  up = hierarchy;
  for (ix = 0; ix < sz; ix++) {
    label = name_get(name, ix);
    hierarchy = (hierarchy_t *) hierarchy_get_bylabel(up, label);
    if (!hierarchy) {
      debug(hierarchy, "No match for '%s' found on level %d", label, ix);
      break;
    }
    up = hierarchy;
  }
  if (match_lost) {
    *match_lost = ix;
  }
  debug(hierarchy, "Returning '%s'", hierarchy_tostring(up));
  return up;
}

/* -- H I E R A R C H Y  M E T H O D S -------------------------------------*/

data_t * _hierarchy_append(hierarchy_t *hierarchy, char *n, array_t *args, dict_t *kwargs) {
  (void) n;
  (void) kwargs;

  return (data_t *) hierarchy_append(hierarchy,
    data_tostring(data_array_get(args, 0)),
    data_array_get(args, 0));
}

data_t * _hierarchy_insert(hierarchy_t *hierarchy, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) kwargs;

  return (data_t *) hierarchy_insert(hierarchy,
    (name_t *) array_get(args, 0),
    data_array_get(args, 0));
}

data_t * _hierarchy_find(hierarchy_t *hierarchy, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) kwargs;

  return (data_t *) hierarchy_find(hierarchy, (name_t *) array_get(args, 0));
}
