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

#include <core.h>
#include <data.h>
#include <nvp.h>

/* ------------------------------------------------------------------------ */

static void         _data_init_nvp(void) __attribute__((constructor));
static data_t *     _data_new_nvp(data_t *, va_list);
static data_t *     _data_copy_nvp(data_t *, data_t *);
static int          _data_cmp_nvp(data_t *, data_t *);
static char *       _data_tostring_nvp(data_t *);
static unsigned int _data_hash_nvp(data_t *);
static data_t *     _nvp_create(data_t *, char *, array_t *, dict_t *);

/* ------------------------------------------------------------------------ */

static vtable_t _vtable_nvp[] = {
  { .id = FunctionNew,      .fnc = (void_t) _data_new_nvp },
  { .id = FunctionCopy,     .fnc = (void_t) _data_copy_nvp },
  { .id = FunctionCmp,      .fnc = (void_t) _data_cmp_nvp },
  { .id = FunctionFree,     .fnc = (void_t) nvp_free },
  { .id = FunctionToString, .fnc = (void_t) _data_tostring_nvp },
  { .id = FunctionHash,     .fnc = (void_t) _data_hash_nvp },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methoddescr_nvp[] = {
  { .type = Any,    .name = "nvp",  .method = _nvp_create,  .argtypes = { Any, Any, NoType },       .minargs = 2, .varargs = 0 },
  { .type = NoType, .name = NULL,    .method = NULL,        .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};



static typedescr_t _typedescr_nvp = {
  .type =      NVP,
  .type_name = "nvp",
  .vtable =    _vtable_nvp
};

/* ------------------------------------------------------------------------ */

void _data_init_nvp(void) {
  typedescr_register(&_typedescr_nvp);  
  typedescr_register_methods(_methoddescr_nvp);
}

data_t * _data_new_nvp(data_t *ret, va_list arg) {
  data_t *name;
  data_t *value;

  name = va_arg(arg, data_t *);
  value = va_arg(arg, data_t *);
  ret -> ptrval = nvp_create(name, value);
  return ret;
}

data_t * _data_copy_nvp(data_t *target, data_t *src) {
  target -> ptrval = nvp_copy(data_nvpval(src));
  return target;
}

int _data_cmp_nvp(data_t *d1, data_t *d2) {
  return nvp_cmp(data_nvpval(d1), data_nvpval(d2));
}

char * _data_tostring_nvp(data_t *d) {
  return nvp_tostring(data_nvpval(d));
}

unsigned int _data_hash_nvp(data_t *d) {
  return nvp_hash(data_nvpval(d));
}

/* ------------------------------------------------------------------------ */

data_t * _nvp_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(NVP, data_array_get(args, 0), data_array_get(args, 1));
}

/* ------------------------------------------------------------------------ */

nvp_t * nvp_create(data_t *name, data_t *value) {
  nvp_t *ret;
  
  ret = NEW(nvp_t);
  ret -> name = data_copy(name);
  ret -> value = data_copy(value);
  ret -> refs = 1;
  ret -> str = NULL;
  return ret;
}

nvp_t * nvp_parse(char *str) {
  char    cpy[strlen(str) + 1];
  char   *ptr;
  char   *name;
  char   *val;
  data_t *n;
  data_t *v;
  nvp_t  *ret;
  
  // FIXME Woefully inadequate.
  strcpy(cpy, str);
  ptr = strchr(cpy, '=');
  name = cpy;
  val = NULL;
  if (ptr) {
    *ptr = 0;
    val = ptr + 1;
  }
  n = data_create(String, name);
  v = (val) ? data_decode(val) : data_true();
  ret = nvp_create(n, v);
  data_free(n);
  data_free(v);
  return ret;
}

nvp_t * nvp_copy(nvp_t *src) {
  src -> refs++;
  return src;
}

void nvp_free(nvp_t *nvp) {
  if (nvp) {
    nvp -> refs--;
    if (!nvp -> refs) {
      free(nvp -> str);
      data_free(nvp -> name);
      data_free(nvp -> value);
      free(nvp);
    }
  }
}

char * nvp_tostring(nvp_t *nvp) {
  free(nvp -> str);
  asprintf(&nvp -> str, "%s: %s", 
	   data_tostring(nvp -> name),
	   data_tostring(nvp -> value));
  return nvp -> str;
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
