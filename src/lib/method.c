/*
 * /obelix/src/types/string.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <limits.h>

#include <data.h>
#include <exception.h>
#include <method.h>

static void          _mth_init(void) __attribute__((constructor));
static void          _mth_free(mth_t *);
static char *        _mth_allocstring(mth_t *);

static vtable_t _vtable_method[] = {
  { .id = FunctionFree,        .fnc = (void_t) _mth_free },
  { .id = FunctionCmp,         .fnc = (void_t) mth_cmp },
  { .id = FunctionAllocString, .fnc = (void_t) _mth_allocstring },
  { .id = FunctionHash,        .fnc = (void_t) mth_hash },
  { .id = FunctionCall,        .fnc = (void_t) mth_call },
  { .id = FunctionNone,        .fnc = NULL }
};

int Method = -1;

/* -- M T H _ T  S T A T I C  F U N C T I O N S --------------------------- */

void _mth_init(void) {
  Method = typedescr_create_and_register(Method, "method", _vtable_method, NULL);
}

void _mth_free(mth_t *mth) {
  if (mth) {
    data_free(mth -> self);
  }
}

char * _mth_allocstring(mth_t *mth) {
  char *buf;

  asprintf(&buf, "%s.%s",
	   data_tostring(mth -> self),
	   mth -> method -> name);
  return buf;
}

/* -- M T H _ T  P U B L I C  F U N C T I O N S --------------------------- */

mth_t * mth_create(methoddescr_t *md, data_t *self) {
  mth_t *ret = data_new(Method, mth_t);

  assert(md);
  assert(self);
  ret -> method = md;
  ret -> self = data_copy(self);
  return ret;
}

data_t * mth_call(mth_t *mth, array_t *args, dict_t *kwargs) {
  typedescr_t   *type;
  methoddescr_t *md;
  int            i;
  int            j;
  int            len;
  int            t;
  int            maxargs = -1;
  data_t        *arg;

  assert(mth);
  md = mth -> method;
  type = data_typedescr(mth -> self);

  len = (args) ? array_size(args) : 0;
  maxargs = md -> maxargs;
  if (!maxargs) {
    maxargs = (md -> varargs) ? INT_MAX : md -> minargs;
  }
  assert(maxargs >= md -> minargs);
  if (len < md -> minargs) {
    if (md -> varargs) {
      return data_exception(ErrorArgCount, "%s.%s requires at least %d arguments",
                       type -> type_name, md -> name, md -> minargs);
    } else {
      return data_exception(ErrorArgCount, "%s.%s requires exactly %d arguments",
                       type -> type_name, md -> name, md -> minargs);
    }
  } else if (len > maxargs) {
    if (maxargs == 0) {
      return data_exception(ErrorArgCount, "%s.%s accepts no arguments",
                        type -> type_name, md -> name);
    } else if (maxargs == 1) {
      return data_exception(ErrorArgCount, "%s.%s accepts only one argument",
                        type -> type_name, md -> name);
    } else {
      return data_exception(ErrorArgCount, "%s.%s accepts only %d arguments",
                       type -> type_name, md -> name, maxargs);
    }
  }
  for (i = 0; i < len; i++) {
    arg = array_get(args, i);
    if (i < md -> minargs) {
      t = md -> argtypes[i];
    } else {
      for (j = (i >= MAX_METHOD_PARAMS) ? MAX_METHOD_PARAMS - 1 : i;
           (j >= 0) && (md -> argtypes[j] == NoType);
           j--);
      t = md -> argtypes[j];
    }
    if (!data_hastype(arg, t)) {
      return data_exception(ErrorType, "Type mismatch: Type of argument %d of %s.%s must be %s, not %s",
                        i+1, type -> type_name, md -> name,
                        typedescr_get(t) -> type_name,
                        data_typedescr(arg) -> type_name);
    }
  }
  return md -> method(mth -> self, md -> name, args, kwargs);
}

unsigned int mth_hash(mth_t *mth) {
  return hashblend(strhash(mth -> method -> name), data_hash(mth -> self));
}

int mth_cmp(mth_t *m1, mth_t *m2) {
  int cmp = data_cmp(m1 -> self, m2 -> self);

  return (!cmp) ? cmp : strcmp(m1 -> method -> name, m2 -> method -> name);
}
