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

#include <data.h>
#include <exception.h>

typedef struct _method {
  methoddescr_t *method;
  data_t        *self;
  char          *str;
} mth_t;

static mth_t *       mth_create(methoddescr_t *, data_t *);
static mth_t *       mth_copy(mth_t *);
static void          mth_free(mth_t *);
static data_t *      mth_call(mth_t *, array_t *, dict_t *);
static char *        mth_tostring(mth_t *);
static unsigned int  mth_hash(mth_t *);
static int           mth_cmp(mth_t *, mth_t *);

static void          _method_init(void) __attribute__((constructor));
static data_t *      _method_new(data_t *, va_list);
static data_t *      _method_copy(data_t *, data_t *);
static int           _method_cmp(data_t *, data_t *);
static char *        _method_tostring(data_t *);
static unsigned int  _method_hash(data_t *);
static data_t *      _method_call(data_t *, array_t *, dict_t *);

static vtable_t _vtable_method[] = {
  { .id = FunctionNew,      .fnc = (void_t) _method_new },
  { .id = FunctionCopy,     .fnc = (void_t) _method_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _method_cmp },
  { .id = FunctionToString, .fnc = (void_t) _method_tostring },
  { .id = FunctionHash,     .fnc = (void_t) _method_hash },
  { .id = FunctionCall,     .fnc = (void_t) _method_call },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_method = {
  .type =        Method,
  .type_name =   "method",
  .vtable =      _vtable_method
};

/*
 * --------------------------------------------------------------------------
 * mth_t functions
 * --------------------------------------------------------------------------
 */

mth_t * mth_create(methoddescr_t *md, data_t *self) {
  mth_t *ret = NEW(mth_t);

  assert(md);
  assert(self);
  ret -> method = md;
  ret -> self = data_copy(self);
  ret -> str = NULL;
  return ret;
}

mth_t * mth_copy(mth_t *src) {
  return mth_create(src -> method, src -> self);
}

void mth_free(mth_t *mth) {
  if (mth) {
    data_free(mth -> self);
    free(mth -> str);
    free(mth);
  }
}

data_t * mth_call(mth_t *mth, array_t *args, dict_t *kwargs) {
  typedescr_t   *type;
  methoddescr_t *md;
  int            i;
  int            len;
  int            t;
  int            maxargs = -1;
  data_t        *arg;

  assert(mth);
  md = mth -> method;
  type = data_typedescr(mth -> self);
  
  len = (args) ? array_size(args) : 0;
  if (!md -> varargs) {
    maxargs = md -> minargs;
  }
  if (len < mth -> method -> minargs) {
    if (md -> varargs) {
      return data_error(ErrorArgCount, "%s.%s requires at least %d arguments",
                       type -> type_name, md -> name, md -> minargs);
    } else {
      return data_error(ErrorArgCount, "%s.%s requires exactly %d arguments",
                       type -> type_name, md -> name, md -> minargs);
    }
  } else if (!md -> varargs && (len > maxargs)) {
    if (maxargs == 0) {
      return data_error(ErrorArgCount, "%s.%s accepts no arguments",
                        type -> type_name, md -> name);
    } else if (maxargs == 1) {
      return data_error(ErrorArgCount, "%s.%s accepts only one argument",
                        type -> type_name, md -> name);
    } else {
      return data_error(ErrorArgCount, "%s.%s accepts only %d arguments",
                       type -> type_name, md -> name, maxargs);
    }
  }
  for (i = 0; i < len; i++) {
    arg = array_get(args, i);
    t = (i < md -> minargs)
      ? md -> argtypes[i]
      : md -> argtypes[(md -> minargs) ? md -> minargs - 1 : 0];
    if (!data_hastype(arg, t)) {
      return data_error(ErrorType, "Type mismatch: Type of argument %d of %s.%s must be %d, not %d",
                        i+1, type -> type_name, md -> name, t, data_type(arg));
    }
  }
  return md -> method(mth -> self, md -> name, args, kwargs);  
}

char * mth_tostring(mth_t *mth) {
  char *s = data_tostring(mth -> self);
  
  free(mth -> str);
  asprintf(&mth -> str, "%s.%s", s, mth -> method -> name);
  return mth -> str;
}

unsigned int mth_hash(mth_t *mth) {
  return hashblend(strhash(mth -> method -> name), data_hash(mth -> self));
}

int mth_cmp(mth_t *m1, mth_t *m2) {
  int cmp = data_cmp(m1 -> self, m2 -> self);
  
  return (!cmp) ? cmp : strcmp(m1 -> method -> name, m2 -> method -> name);
}

/*
 * --------------------------------------------------------------------------
 * Method datatype
 * --------------------------------------------------------------------------
 */

void _method_init(void) {
  typedescr_register(&_typedescr_method);
}

data_t * _method_new(data_t *data, va_list args) {
  methoddescr_t *md;
  data_t        *self;
  
  md = va_arg(args, methoddescr_t *);
  self = va_arg(args, data_t *);
  data -> ptrval = mth_create(md, self);
  return data;
}

data_t * _method_copy(data_t *target, data_t *src) {
  target -> ptrval = mth_copy((mth_t *) src -> ptrval);
  return target;
}

int _method_cmp(data_t *d1, data_t *d2) {
  return mth_cmp((mth_t *) d1 -> ptrval, (mth_t *) d2 -> ptrval);  
}

char * _method_tostring(data_t *data) {
  return mth_tostring((mth_t *) data -> ptrval);
}

unsigned int _method_hash(data_t *data) {
  return mth_hash((mth_t *) data -> ptrval);
}

data_t * _method_call(data_t *data, array_t *args, dict_t *kwargs) {
  return mth_call((mth_t *) data -> ptrval, args, kwargs);
}
