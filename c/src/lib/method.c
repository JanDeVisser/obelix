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

#include "libcore.h"
#include <data.h>
#include <exception.h>
#include <method.h>

static void          _mth_init(void);
static mth_t *       _mth_new(mth_t *, va_list);
static void *        _mth_reduce_children(mth_t *, reduce_t, void *);

static vtable_t _vtable_RuntimeMethod[] = {
  { .id = FunctionNew,         .fnc = (void_t) _mth_new },
  { .id = FunctionCmp,         .fnc = (void_t) mth_cmp },
  { .id = FunctionHash,        .fnc = (void_t) mth_hash },
  { .id = FunctionCall,        .fnc = (void_t) mth_call },
  { .id = FunctionReduce,      .fnc = (void_t) _mth_reduce_children },
  { .id = FunctionNone,        .fnc = NULL }
};

int RuntimeMethod = -1;
int method_debug = 0;

/* -- M T H _ T  S T A T I C  F U N C T I O N S --------------------------- */

mth_t * _mth_new(mth_t *mth, va_list args) {
  methoddescr_t *md = va_arg(args, methoddescr_t *);
  data_t        *self = va_arg(args, data_t *);

  mth -> method = md;
  mth -> self = self;
  asprintf(&mth->_d.str, "%s.%s",
           data_tostring(mth->self),
           mth->method->name);
  data_set_string_semantics(mth, StrSemanticsStatic);
  return mth;
}

void _mth_init(void) {
  if (RuntimeMethod < 0) {
    logging_register_module(method);
    typedescr_register(RuntimeMethod, mth_t);
  }
}

void * _mth_reduce_children(mth_t *mth, reduce_t reducer, void *ctx) {
  return reducer(mth->self, ctx);
}

/* -- M T H _ T  P U B L I C  F U N C T I O N S --------------------------- */

mth_t * mth_create(methoddescr_t *md, data_t *self) {
  assert(md);
  assert(self);
  _mth_init();
  return (mth_t *) data_create(RuntimeMethod, md, self);
}

data_t * mth_call(mth_t *mth, arguments_t *args) {
  typedescr_t   *type;
  methoddescr_t *md;
  int            i;
  int            j;
  int            len;
  int            t;
  int            maxargs = -1;
  data_t        *arg;
  char           buf[4096];
  char           argstr[1024];

  assert(mth);
  md = mth -> method;
  type = data_typedescr(mth -> self);

  len = (args) ? datalist_size(args -> args) : 0;
  maxargs = md -> maxargs;
  if (!maxargs) {
    maxargs = (md -> varargs) ? INT_MAX : md -> minargs;
  }
  assert(maxargs >= md -> minargs);
  if (len < md -> minargs) {
    if (md -> varargs) {
      return data_exception(ErrorArgCount, "%s.%s requires at least %d arguments",
                            type_name(type), md -> name, md -> minargs);
    } else {
      return data_exception(ErrorArgCount, "%s.%s requires exactly %d arguments",
                            type_name(type), md -> name, md -> minargs);
    }
  } else if (len > maxargs) {
    if (maxargs == 0) {
      return data_exception(ErrorArgCount, "%s.%s accepts no arguments",
                            type_name(type), md -> name);
    } else if (maxargs == 1) {
      return data_exception(ErrorArgCount, "%s.%s accepts only one argument",
                            type_name(type), md -> name);
    } else {
      return data_exception(ErrorArgCount, "%s.%s accepts only %d arguments",
                            type_name(type), md -> name, maxargs);
    }
  }
  buf[0] = 0;
  for (i = 0; i < len; i++) {
    arg = arguments_get_arg(args, i);
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
                        i + 1, type_name(type), md -> name,
                            type_name(kind_get(t)),
                            data_typename(arg));
    }
    if (method_debug) {
      if (i > 0) {
        strcat(buf, ", ");
      }
      snprintf(argstr, 1024, "%s [%s]", arguments_arg_tostring(args, i), data_typename(arg));
      strcat(buf, argstr);
    }
  }
  debug(method, "Calling %s -> %s(%s)", data_tostring(mth -> self), md -> name, buf)
  return md -> method(mth -> self, md -> name, args);
}

unsigned int mth_hash(mth_t *mth) {
  return hashblend(strhash(mth -> method -> name), data_hash(mth -> self));
}

int mth_cmp(mth_t *m1, mth_t *m2) {
  int cmp = data_cmp(m1 -> self, m2 -> self);

  return (!cmp) ? cmp : strcmp(m1 -> method -> name, m2 -> method -> name);
}
