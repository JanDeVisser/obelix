  /*
 * /obelix/src/types/nvp.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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
#include <str.h>

/* ------------------------------------------------------------------------ */

static nvp_t *      _nvp_new(nvp_t *, va_list);
static void         _nvp_free(nvp_t *);
static char *       _nvp_allocstring(nvp_t *);
static data_t *     _nvp_resolve(nvp_t *, char *);
static void *       _nvp_reduce_children(nvp_t *, reduce_t, void *);

static data_t *     _nvp_create(char *, arguments_t *);

/* ------------------------------------------------------------------------ */

static vtable_t _vtable_NVP[] = {
  { .id = FunctionNew,         .fnc = (void_t) _nvp_new },
  { .id = FunctionCmp,         .fnc = (void_t) nvp_cmp },
  { .id = FunctionParse,       .fnc = (void_t) nvp_parse },
  { .id = FunctionHash,        .fnc = (void_t) nvp_hash },
  { .id = FunctionResolve,     .fnc = (void_t) _nvp_resolve },
  { .id = FunctionReduce,      .fnc = (void_t) _nvp_reduce_children },
  { .id = FunctionNone,        .fnc = NULL }
};

int NVP = -1;

/* ------------------------------------------------------------------------ */

void nvp_init(void) {
  if (NVP < 1) {
    typedescr_register(NVP, nvp_t);
  }
}

nvp_t * _nvp_new(nvp_t *nvp, va_list args) {
  data_t *name;
  data_t *value;

  name = va_arg(args, data_t *);
  value = va_arg(args, data_t *);
  nvp -> name = name;
  nvp -> value = value;
  asprintf(&nvp->_d.str, "%s=%s",
           data_tostring(nvp -> name),
           data_tostring(nvp -> value));
  data_set_string_semantics(nvp, StrSemanticsStatic);
  return nvp;
}

data_t * _nvp_resolve(nvp_t *nvp, char *name) {
  if (!strcmp(name, "name")) {
    return nvp -> name;
  } else if (!strcmp(name, "value")) {
    return nvp -> value;
  } else {
    return NULL;
  }
}

void * _nvp_reduce_children(nvp_t *nvp, reduce_t reducer, void *ctx) {
  return reducer(nvp->value, reducer(nvp->name, ctx));
}

/* ------------------------------------------------------------------------ */

nvp_t * nvp_create(data_t *name, data_t *value) {
  nvp_init();
  return (nvp_t *) data_create(NVP, name, value);
}

nvp_t * nvp_parse(char *str) {
  char   *cpy;
  char   *ptr;
  char   *name;
  char   *val;
  data_t *n;
  data_t *v;
  nvp_t  *ret;

  // FIXME Woefully inadequate.
  cpy = strdup(str);
  ptr = strchr(cpy, '=');
  name = cpy;
  val = NULL;
  if (ptr) {
    *ptr = 0;
    val = ptr + 1;
    val = strtrim(val);
  }
  name = strtrim(name);
  n = str_to_data(name);
  v = (val) ? data_decode(val) : data_true();
  ret = nvp_create(n, v);
  data_free(n);
  data_free(v);
  free(cpy);
  return ret;
}

int nvp_cmp(nvp_t *nvp1, nvp_t *nvp2) {
  int ret = data_cmp(nvp1 -> name, nvp2 -> name);
  if (!ret) {
    ret = data_cmp(nvp1 -> value, nvp2 -> value);
  }
  return ret;
}

unsigned int nvp_hash(nvp_t *nvp) {
  return hashblend(data_hash(nvp -> name), data_hash(nvp -> value));
}

/* ------------------------------------------------------------------------ */

data_t * _nvp_create(char *name, arguments_t *args) {
  return (data_t *) nvp_create(arguments_get_arg(args, 0), arguments_get_arg(args, 1));
}
