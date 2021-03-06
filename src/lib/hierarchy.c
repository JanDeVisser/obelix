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
#include <nvp.h>

/* ------------------------------------------------------------------------ */

static hierarchy_t * _hierarchy_new(hierarchy_t *, va_list);
static unsigned long _hierarchy_hash(hierarchy_t *);
static int           _hierarchy_cmp(hierarchy_t *, hierarchy_t *);
static int           _hierarchy_size(hierarchy_t *);
static data_t *      _hierarchy_iter(hierarchy_t *);
static void          _hierarchy_free(hierarchy_t *);
static char *        _hierarchy_tostring(hierarchy_t *);
static data_t *      _hierarchy_resolve(hierarchy_t *, char *);
static void *        _hierarchy_reduce_children(hierarchy_t *, reduce_t, void *);


  static void          _hierarchy_get_nodes(hierarchy_t *, array_t *);

static data_t *      _hierarchy_append(hierarchy_t *, char *, arguments_t *);
static data_t *      _hierarchy_insert(hierarchy_t *, char *, arguments_t *);
static data_t *      _hierarchy_find(hierarchy_t *, char *, arguments_t *);

static vtable_t _vtable_Hierarchy[] = {
  { .id = FunctionNew,         .fnc = (void_t) _hierarchy_new},
  { .id = FunctionCmp,         .fnc = (void_t) _hierarchy_cmp},
  { .id = FunctionHash,        .fnc = (void_t) _hierarchy_hash },
  { .id = FunctionResolve,     .fnc = (void_t) _hierarchy_resolve },
  { .id = FunctionLen,         .fnc = (void_t) _hierarchy_size },
  { .id = FunctionIter,        .fnc = (void_t) _hierarchy_iter },
  { .id = FunctionReduce,      .fnc = (void_t) _hierarchy_reduce_children },
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
  char        *label = va_arg(args, char *);
  data_t      *data = NULL;
  hierarchy_t *up = NULL;

  data = va_arg(args, data_t *);
  if (label) {
    up = va_arg(args, hierarchy_t *);
  } else {
    label = strdup("/");
  }
  data_set_static_string(hierarchy, strdup(label));
  hierarchy -> data = data;
  hierarchy -> up = up;
  hierarchy -> branches = datalist_create(NULL);
  return hierarchy;
};

unsigned long _hierarchy_hash(hierarchy_t *hierarchy) {
  return strhash(hierarchy -> _d.str);
}

int _hierarchy_size(hierarchy_t *hierarchy) {
  return datalist_size(hierarchy -> branches);
}

int _hierarchy_cmp(hierarchy_t *l1, hierarchy_t *l2) {
  return strcmp(l1 -> _d.str, l2 -> _d.str);
}

void * _hierarchy_reduce_children(hierarchy_t *hierarchy, reduce_t reducer, void *ctx) {
  ctx = reducer(hierarchy->data, ctx);
  ctx = reducer(hierarchy->up, ctx);
  return reducer(hierarchy->branches, ctx);
}

data_t * _hierarchy_iter(hierarchy_t *hierarchy) {
  array_t *nodes;
  data_t  *iter;
  data_t  *list;

  nodes = array_create(4);
  _hierarchy_get_nodes(hierarchy, nodes);
  list = (data_t *) datalist_create(nodes);
  iter = data_iter(list);
  data_free(list);
  return iter;
}

data_t * _hierarchy_resolve(hierarchy_t *hierarchy, char *name) {
  hierarchy_t *branch;
  long         l;

  if ((branch = hierarchy_get_bylabel(hierarchy, name))) {
    return (data_t *) branch;
  } else if (!strtoint(name, &l) && (l >= 0) && (l < list_size(hierarchy -> branches))) {
    return (data_t *) hierarchy_get(hierarchy, l);
  } else if (!strcmp(name, "up")) {
    return (data_t *) hierarchy -> up;
  } else if (!strcmp(name, "depth")) {
    return int_to_data(hierarchy_depth(hierarchy));
  } else if (!strcmp(name, "root")) {
    return (data_t *) hierarchy_root(hierarchy);
  }
  return NULL;
}

/* -- H I E R A R C H Y  A P I ---------------------------------------------*/

void _hierarchy_get_nodes(hierarchy_t *hierarchy, array_t *nodes) {
  int          ix;
  hierarchy_t *branch;

  for (ix = 0; ix < datalist_size(hierarchy->branches); ix++) {
    branch = (hierarchy_t *) datalist_get(hierarchy->branches, ix);
    if (branch -> data) {
      array_push(nodes,
          nvp_create((data_t *) hierarchy_name(branch), branch -> data));
    }
    _hierarchy_get_nodes(branch, nodes);
  }
}

hierarchy_t * hierarchy_create(char *label, data_t *data, hierarchy_t *up) {
  hierarchy_init();
  return (hierarchy_t *) data_create(Hierarchy, label, data, up);
}

hierarchy_t * hierarchy_append(hierarchy_t *hierarchy, char *label, data_t *data) {
  hierarchy_t *ret;

  ret = hierarchy_create(label, data, hierarchy);
  datalist_push(hierarchy -> branches, ret);
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
    hierarchy = hierarchy_get_bylabel(up, label);
    if (!hierarchy) {
      hierarchy = hierarchy_append(up, label, NULL);
    }
  }
  data_free(hierarchy -> data);
  hierarchy -> data = data;
  return hierarchy;
}

hierarchy_t * hierarchy_remove(hierarchy_t *hierarchy, name_t *name) {
  hierarchy_t *leaf;
  hierarchy_t *up;
  hierarchy_t *ret = NULL;
  int          ix;

  if (!name_size(name)) {
    return NULL;
  }
  leaf = hierarchy_find(hierarchy, name);
  if (leaf) {
    up = leaf -> up;
    for (ix = 0; ix < datalist_size(hierarchy->branches); ix++) {
      if (data_cmp((data_t *) leaf, (data_t *) datalist_get(hierarchy->branches, ix))) {
        datalist_remove(hierarchy->branches, ix);
        ret = hierarchy;
      }
    }
  } else {
    return NULL;
  }
  return ret;
}

hierarchy_t * hierarchy_get_bylabel(hierarchy_t *hierarchy, char *label) {
  hierarchy_t *branch;
  hierarchy_t *ret = NULL;
  int          ix;


  for (ix = 0; ix < datalist_size(hierarchy->branches); ix++) {
    branch = (hierarchy_t *) datalist_get(hierarchy->branches, ix);
    if (!strcmp(label, branch -> _d.str)) {
      ret = branch;
    }
  }
  return ret;
}

hierarchy_t * hierarchy_get(hierarchy_t *hierarchy, int ix) {
  return ((ix >= 0) && (ix < datalist_size(hierarchy -> branches)))
    ? (hierarchy_t *) datalist_get(hierarchy -> branches, ix)
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
    array_set(labels, ix, hierarchy -> _d.str);
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

data_t * _hierarchy_append(hierarchy_t *hierarchy, _unused_ char *name, arguments_t *args) {
  return (data_t *) hierarchy_append(hierarchy,
    arguments_arg_tostring(args, 0),
    arguments_get_arg(args, 0));
}

data_t * _hierarchy_insert(hierarchy_t *hierarchy, _unused_ char *name, arguments_t *args) {
  return (data_t *) hierarchy_insert(hierarchy,
    (name_t *) arguments_get_arg(args, 0), arguments_get_arg(args, 0));
}

data_t * _hierarchy_find(hierarchy_t *hierarchy, _unused_ char *name, arguments_t *args) {
  return (data_t *) hierarchy_find(hierarchy,
      data_as_name(arguments_get_arg(args, 0)));
}
