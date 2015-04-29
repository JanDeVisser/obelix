/*
 * /obelix/src/types/any.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <data.h>
#include <exception.h>

/* ------------------------------------------------------------------------ */

static data_t * _any_cmp(data_t *, char *, array_t *, dict_t *);
static data_t * _any_not(data_t *, char *, array_t *, dict_t *);
static data_t * _any_and(data_t *, char *, array_t *, dict_t *);
static data_t * _any_or(data_t *, char *, array_t *, dict_t *);
static data_t * _any_hash(data_t *, char *, array_t *, dict_t *);
static data_t * _any_hasattr(data_t *, char *, array_t *, dict_t *);
static data_t * _any_getattr(data_t *, char *, array_t *, dict_t *);
static data_t * _any_setattr(data_t *, char *, array_t *, dict_t *);
static data_t * _any_callable(data_t *, char *, array_t *, dict_t *);
static data_t * _any_iterable(data_t *, char *, array_t *, dict_t *);
static data_t * _any_iter(data_t *, char *, array_t *, dict_t *);
static data_t * _any_next(data_t *, char *, array_t *, dict_t *);
static data_t * _any_has_next(data_t *, char *, array_t *, dict_t *);

/* ------------------------------------------------------------------------ */

static typedescr_t _typedescr_any = {
  .type =      Any,
  .type_name = "any"
};

static methoddescr_t _methoddescr_any[] = {
  { .type = Any,    .name = ">" ,       .method = _any_cmp,      .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "<" ,       .method = _any_cmp,      .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = ">=" ,      .method = _any_cmp,      .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "<=" ,      .method = _any_cmp,      .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "==" ,      .method = _any_cmp,      .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "!=" ,      .method = _any_cmp,      .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "not",      .method = _any_not,      .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "and",      .method = _any_and,      .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = Any,    .name = "&&",       .method = _any_and,      .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = Any,    .name = "or",       .method = _any_or,       .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = Any,    .name = "||",       .method = _any_or,       .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = Any,    .name = "hash",     .method = _any_hash,     .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "hasattr",  .method = _any_hasattr,  .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "getattr",  .method = _any_getattr,  .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "setattr",  .method = _any_setattr,  .argtypes = { String, Any, NoType },    .minargs = 2, .varargs = 0 },
  { .type = Any,    .name = "callable", .method = _any_callable, .argtypes = { Any, NoType, NoType },    .minargs = 0, .varargs = 1 },
  { .type = Any,    .name = "iterable", .method = _any_iterable, .argtypes = { Any, NoType, NoType },    .minargs = 0, .varargs = 1 },
  { .type = Any,    .name = "iter",     .method = _any_iter,     .argtypes = { Any, NoType, NoType },    .minargs = 0, .varargs = 1 },
  { .type = Any,    .name = "next",     .method = _any_next,     .argtypes = { Any, NoType, NoType },    .minargs = 0, .varargs = 1 },
  { .type = Any,    .name = "hasnext",  .method = _any_has_next, .argtypes = { Any, NoType, NoType },    .minargs = 0, .varargs = 1 },
  { .type = NoType, .name = NULL,       .method = NULL,          .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

/* ------------------------------------------------------------------------ */

/* -- 'any' G L O B A L  M E T H O D S ------------------------------------ */

void any_init(void) {
  typedescr_register(&_typedescr_any);
  typedescr_register_methods(_methoddescr_any);
}

data_t * _any_cmp(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *other = (data_t *) array_get(args, 0);
  data_t *ret;
  int     cmp = data_cmp(self, other);
  
  if (!strcmp(name, "==")) {
    ret = data_create(Bool, !cmp);
  } else if (!strcmp(name, "!=")) {
    ret = data_create(Bool, cmp);
  } else if (!strcmp(name, ">")) {
    ret = data_create(Bool, cmp > 0);
  } else if (!strcmp(name, ">=")) {
    ret = data_create(Bool, cmp >= 0);
  } else if (!strcmp(name, "<")) {
    ret = data_create(Bool, cmp < 0);
  } else if (!strcmp(name, "<=")) {
    ret = data_create(Bool, cmp <= 0);
  } else {
    assert(0);
    ret = data_create(Bool, 0);
  }
  return ret;
}

data_t * _any_not(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *asbool = data_cast(self, Bool);
  data_t *ret;
  
  (void) name;
  (void) args;
  (void) kwargs;
  if (!asbool) {
    return data_exception(ErrorSyntax, 
                      "not(): Cannot convert value '%s' of type '%s' to boolean",
                      data_tostring(self),
                      data_typedescr(self) -> type_name);
  } else {
    ret = data_create(Int, data_intval(asbool) == 0);
    data_free(asbool);
  }
}

data_t * _any_and(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *asbool;
  int     boolval;
  int     ix;
  
  (void) name;
  (void) kwargs;
  for (ix = -1; ix < array_size(args); ix++) {
    asbool = data_cast((ix < 0) ? self : data_array_get(args, ix), Bool);
    if (!asbool) {
      return data_exception(ErrorSyntax, 
                        "and(): Cannot convert value '%s' of type '%s' to boolean",
                        data_tostring(data_array_get(args, ix)),
                        data_typedescr(data_array_get(args, ix)) -> type_name);
    }
    boolval = data_intval(asbool);
    data_free(asbool);
    if (!boolval) {
      return data_create(Bool, FALSE);
    }
  }
  return data_create(Bool, TRUE);
}

data_t * _any_or(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *asbool;
  int     boolval;
  int     ix;
  
  (void) name;
  (void) kwargs;
  for (ix = -1; ix < array_size(args); ix++) {
    asbool = data_cast((ix < 0) ? self : data_array_get(args, ix), Bool);
    if (!asbool) {
      return data_exception(ErrorSyntax, 
                        "or(): Cannot convert value '%s' of type '%s' to boolean",
                        data_tostring(data_array_get(args, ix)),
                        data_typedescr(data_array_get(args, ix)) -> type_name);
    }
    boolval = data_intval(asbool);
    data_free(asbool);
    if (boolval) {
      return data_create(Bool, TRUE);
    }
  }
  return data_create(Bool, FALSE);
}

data_t * _any_hash(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, data_hash(self));
}

data_t * _any_hasattr(data_t *self, char *func_name, array_t *args, dict_t *kwargs) {
  data_t  *attrname = data_array_get(args, 0);
  name_t  *name = name_create(1, data_tostring(attrname));
  data_t  *ret;
  
  ret = data_create(Bool, data_resolve(self, name) != NULL);
  name_free(name);
  return ret;
}

data_t * _any_getattr(data_t *self, char *func_name, array_t *args, dict_t *kwargs) {
  data_t  *attrname = data_array_get(args, 0);
  name_t  *name = name_create(1, data_tostring(attrname));
  data_t  *ret;
  
  ret = data_get(self, name);
  name_free(name);
  return ret;
}

data_t * _any_setattr(data_t *self, char *func_name, array_t *args, dict_t *kwargs) {
  data_t  *attrname = data_array_get(args, 0);
  data_t  *value = data_array_get(args, 1);
  name_t  *name = name_create(1, data_tostring(attrname));
  data_t  *ret;

  ret = data_set(self, name, value);
  name_free(name);
  return ret;
}

data_t * _any_callable(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *obj;
  
  obj = (array_size(args)) ? data_array_get(args, 0) : self;
  return data_create(Bool, data_is_callable(obj));
}

data_t * _any_iterable(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *obj;
  
  obj = (array_size(args)) ? data_array_get(args, 0) : self;
  return data_create(Bool, data_is_iterable(obj));
}

data_t * _any_iter(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_iter(self);
}

data_t * _any_next(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_next(self);
}

data_t * _any_has_next(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_has_next(self);
}

