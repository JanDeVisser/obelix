/*
 * /obelix/src/lib/data.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcore.h"
#include <data.h>
#include <dictionary.h>
#include <exception.h>
#include <list.h>
#include <logging.h>
#include <method.h>
#include <name.h>
#include <str.h>
#include <typedescr.h>

static data_t *    _data_call_constructor(data_t *, new_t, va_list);
static data_t *    _data_call_constructors(typedescr_t *, va_list);
static void        _data_call_free(typedescr_t *, data_t *);
static data_t *    _data_call_resolve(typedescr_t *, data_t *, char *);
static data_t *    _data_call_setter(typedescr_t *, data_t *, char *, data_t *);


static type_t _type_data = {
  .hash     = (hash_t) data_hash,
  .tostring = (tostring_t) _data_tostring,
  .copy     = (copy_t)  data_copy,
  .free     = (free_t) data_free,
  .cmp      = (cmp_t) data_cmp
};

int     data_debug = 0;
int     _data_count = 0;
type_t *type_data;

/* -- D A T A  S T A T I C  F U N C T I O N S ----------------------------- */

void data_init(void) {
  if (!type_data) {
    logging_register_category("data", &data_debug);
    type_data = &_type_data;
  }
}

data_t * _data_call_constructor(data_t *data, new_t n, va_list args) {
  data_t  *ret;

  debug(data, "Calling constructor %p", n);
  ret = n(data, args);
  if (data != ret) {
    data_free(data);
  }
  return ret;
}

data_t * _data_call_constructors(typedescr_t *type, va_list args) {
  data_t     *ret = NULL;
  data_t     *data;
  void_t     *constructors;
  va_list     copy;

  constructors = typedescr_constructors(type);
  if (constructors && *constructors) {
    data = data_create_noinit(typetype(type));
    for (; *constructors; constructors++) {
      va_copy(copy, args);
      ret = _data_call_constructor(data, (new_t) *constructors, copy);
      va_end(copy);
      if (ret != data) {
        break;
      }
    }
  } else {
    ret = data_copy(va_arg(args, data_t *));
  }
  return ret;
}

data_t * _data_call_resolve(typedescr_t *type, data_t *data, char *name) {
  resolve_name_t  resolve;
  data_t         *ret = NULL;
  int             ix;

  resolve = (resolve_name_t) typedescr_get_local_function(type, FunctionResolve);
  if (resolve) {
    ret = resolve(data, name);
  }
  for (ix = 0; !ret && (ix < MAX_INHERITS) && type -> inherits[ix]; ix++) {
    ret = _data_call_resolve(typedescr_get(type -> inherits[ix]), data, name);
  }
  return ret;
}

void _data_call_free(typedescr_t *type, data_t *data) {
  free_t f;
  int    ix;

  f = (free_t) typedescr_get_local_function(type, FunctionFree);
  if (f) {
    f(data);
  }
  for (ix = 0; (ix < MAX_INHERITS) && type -> inherits[ix]; ix++) {
    _data_call_free(typedescr_get(type -> inherits[ix]), data);
  }
}

data_t * _data_call_setter(typedescr_t *type, data_t *data, char *name, data_t *value) {
  setvalue_t  setter;
  data_t     *ret = NULL;
  int         ix;

  setter = (setvalue_t) typedescr_get_local_function(type, FunctionSet);
  if (setter) {
    ret = setter(data, name, value);
  }
  for (ix = 0; !ret && (ix < MAX_INHERITS) && type -> inherits[ix]; ix++) {
    ret = _data_call_setter(typedescr_get(type -> inherits[ix]), data, name, value);
  }
  return ret;
}

/* -- D A T A  P U B L I C  F U N C T I O N S ----------------------------- */

data_t * data_create_noinit(int type) {
  typedescr_t *descr;
  data_t      *ret;

  descr = typedescr_get(type);
  debug(data, "Allocating %d bytes for new '%s'", descr -> size, typename(descr));
  ret = (data_t *) _new((descr -> size) ? descr -> size : sizeof(data_t));
  ret = data_settype(ret, type);
  ret -> free_me = Normal;
  return ret;
}

data_t * data_settype(data_t *data, int type) {
  typedescr_t *descr;

  data_init();
  assert(data);
  assert(type > 0);
  descr = typedescr_get(type);
  assert(descr);
  assert(typetype(descr) == type);
  assert(typename(descr));
  if (!descr || !typename(descr)) {
    error("Attempt to initialize atom with non-existant or broken type %d", type);
    return NULL;
  }
  if (!data -> type) {
#ifndef NDEBUG
    data -> cookie = MAGIC_COOKIE;
#endif /* NDEBUG */
    data -> type = type;
    data -> refs = 1;
    data -> str = NULL;
    data -> free_me = Normal;
    data -> free_str = Normal;
    descr -> count++;
    _data_count++;
  } else {
    descr = typedescr_get(data -> type);
    if (!descr || !typename(descr)) {
      error("Not re-initializing atom with broken type %d", data -> type);
    }
    return NULL;
  }
  return data;
}

data_t * data_create(int type, ...) {
  va_list      args;
  data_t      *ret;
  typedescr_t *descr;
  factory_t    f;

  data_init();
  descr = typedescr_get(type);
  f = (factory_t) typedescr_get_function(descr, FunctionFactory);
  if (f) {
    va_start(args, type);
    ret = f(type, args);
    va_end(args);
    if (ret) {
      data_settype(ret, type);
    }
  } else {
    va_start(args, type);
    ret = _data_call_constructors(descr, args);
    va_end(args);
  }
  return ret;
}

data_t * data_parse(int type, char *str) {
  typedescr_t *descr;
  void_t       parse;

  data_init();
  descr = typedescr_get(type);
  debug(data, "Parsing %s : '%s'", typename(descr), str);
  parse = typedescr_get_function(descr, FunctionParse);
  return (parse)
    ? ((data_t * (*)(char *)) parse)(str)
    : NULL;
}

data_t * data_decode(char *encoded) {
  char         copy[strlen(encoded)];
  char        *ptr;
  char        *t;
  typedescr_t *type;
  data_t      *ret = NULL;

  data_init();
  if (encoded && encoded[0]) {
    strcpy(copy, encoded);
    debug(data, "Decoding '%s'", copy);
    ptr = strchr(copy, ':');
    if (ptr) {
      *ptr = 0;
      t = strtrim(copy);
      debug(data, "type: '%s'", t);
      type = typedescr_get_byname(t);
      if (!type) {
        ret = data_exception(ErrorType, "Cannot decode '%s': Unknown type '%s'",
          encoded, t);
      }
      ptr++;
    } else {
      ptr = encoded;
      type = typedescr_get(String);
      assert(type);
    }
    if (!ret) {
      ret = data_parse(typetype(type), ptr);
    }
  }
  return ret;
}

char * data_encode(data_t *data) {
  typedescr_t *type;
  void_t       encode;
  char        *encoded = NULL;

  type = data_typedescr(data);
  if (type) {
    encode = typedescr_get_function(type, FunctionEncode);
    if (encode) {
      encoded = ((char * (*)(data_t *)) encode)(data);
    } else {
      encoded = strdup(data_tostring(data));
    }
  }
  return encoded;
}

data_t * data_deserialize(data_t *serialized) {
  typedescr_t *descr;
  void_t       deserialize;

  descr = data_typedescr(serialized);
  debug(data, "Deserializing %s : '%s'", typename(descr), data_tostring(serialized));
  deserialize = typedescr_get_function(descr, FunctionDeserialize);
  return (deserialize)
    ? ((data_t * (*)(data_t *)) deserialize)(serialized)
    : data_copy(serialized);
}

data_t * data_serialize(data_t *data) {
  typedescr_t  *type;
  void_t        serialize;
  data_t       *serialized;
  dictionary_t *dict;

  if (!data) {
    return data_null();
  }
  type = data_typedescr(data);
  if (type) {
    serialize = typedescr_get_function(type, FunctionSerialize);
    if (serialize) {
      serialized = ((data_t * (*)(data_t *)) serialize)(data);
      if (data_is_dictionary(serialized)) {
        dict = data_as_dictionary(serialized);
        dictionary_set(dict, "__obl_type__", (data_t *) str_wrap(typename(type)));
      }
    } else {
      serialized = data_copy(data);
    }
  }
  return serialized;
}

data_t * data_cast(data_t *data, int totype) {
  typedescr_t *descr;
  typedescr_t *totype_descr;
  cast_t       cast;
  tostring_t   tostring;

  assert(data);
  if (data_type(data) == totype) {
    return data_copy(data);
  } else {
    descr = typedescr_get(data_type(data));
    assert(descr);
    totype_descr = typedescr_get(totype);
    assert(totype_descr);
    if (totype == String) {
      tostring = (tostring_t) typedescr_get_function(descr, FunctionToString);
      if (tostring) {
        return (data_t *) str_copy_chars(tostring(data));
      }
    }
    cast = (cast_t) typedescr_get_function(descr, FunctionCast);
    if (cast) {
      return cast(data, totype);
    } else {
      return NULL;
    }
  }
}

data_t * data_promote(data_t *data) {
  typedescr_t *type = data_typedescr(data);

  return ((type -> promote_to != NoType) && (type -> promote_to != Exception))
    ? data_cast(data, type -> promote_to)
    : NULL;
}

void data_free(data_t *data) {
  typedescr_t      *type;
  free_semantics_t  free_me;
  free_semantics_t  free_str;

  if (data && (data -> free_me != Constant) && (--data -> refs <= 0)) {
    free_me = data -> free_me;
    free_str = data -> free_str;
    type = data_typedescr(data);
    _data_call_free(type, data);
    if (free_str != DontFreeData) {
      free(data -> str);
    }
    type -> count--;
    _data_count--;
    if (free_me != DontFreeData) {
      free(data);
    }
  }
}

unsigned int data_hash(data_t *data) {
  typedescr_t *type;
  hash_t       hash;

  if (data) {
    type = data_typedescr(data);
    hash = (hash_t) typedescr_get_function(type, FunctionHash);
    return (hash) ? hash(data) : hashptr(data);
  } else {
    return 0;
  }
}

data_t * data_copy(data_t *src) {
  if (src) {
    src -> refs++;
  }
  return src;
}

data_t * data_uncopy(data_t *src) {
  if (src) {
    src -> refs--;
  }
  return src;
}

data_t * data_call(data_t *self, array_t *args, dict_t *kwargs) {
  call_t call = (call_t) data_get_function(self, FunctionCall);

  assert(data_is_callable(self));
  return call(self, args, kwargs);
}

int data_hasmethod(data_t *data, char *name) {
  typedescr_t   *type = data_typedescr(data);
  methoddescr_t *md;

  md = typedescr_get_method(type, name);
  mdebug(data, "'%s'.hasmethod(%s): %s", data_tostring(data), name, (md) ? "true" : "false");
  return (md) ? TRUE : FALSE;
}

data_t * data_method(data_t *data, char *name) {
  typedescr_t   *type = NULL;
  methoddescr_t *md;
  data_t        *ret = NULL;

  mdebug(data, "%s[%s].data_method(%s)", data_tostring(data), data_typename(data), name);
  type = data_typedescr(data);
  md = typedescr_get_method(type, name);
  if (md) {
    ret = (data_t *) mth_create(md, data);
    debug(data, "%s[%s].data_method(%s) = %s", data_tostring(data),
          data_typename(data), name, runtimemethod_tostring(ret));
  } else {
    debug(data, "%s[%s].data_method(%s) = NULL", data_tostring(data),
          data_typename(data), name);
  }
  return ret;
}

data_t * data_resolve(data_t *data, name_t *name) {
  typedescr_t    *type = data_typedescr(data);
  data_t         *ret = NULL;
  data_t         *tail_resolve;
  name_t         *tail;
  char           *n;

  assert(type);
  assert(name);
  debug(data, "%s[%s].resolve(%s:%d)",
        data_tostring(data), data_typename(data),
        name_tostring(name), name_size(name));
  n = (name_size(name)) ? name_first(name) : NULL;
  if (name_size(name) == 0) {
    ret = data_copy(data);
  }
  if (!ret) {
    ret = _data_call_resolve(type, data, n);
  }
  if (!ret) {
    if (!strcmp(n, "type")) {
      ret = (data_t *) type;
    } else if (!strcmp(n, "typename")) {
      ret = (data_t *) str_copy_chars(typename(type));
    } else if (!strcmp(n, "typeid")) {
      ret = int_to_data(typetype(type));
    }
  }
  if (!ret && (name_size(name) == 1)) {
    ret = data_method(data, n);
  }
  if (!ret) {
    ret = data_exception(ErrorType,
            "Cannot resolve name '%s' in %s '%s'",
            name_tostring(name), typename(type),
            data_tostring(data));
  }
  if (ret && (!data_is_exception(ret) || data_as_exception(ret) -> handled)
          && (name_size(name) > 1)) {
    tail = name_tail(name);
    tail_resolve = data_resolve(ret, tail);
    data_free(ret);
    ret = tail_resolve;
    name_free(tail);
  }
  debug(data, "%s.resolve(%s) = %s", data_tostring(data), name_tostring(name), data_tostring(ret));
  return ret;
}

data_t * data_invoke(data_t *self, name_t *name, array_t *args, dict_t *kwargs) {
  data_t  *callable;
  data_t  *ret = NULL;
  array_t *args_shifted = NULL;

  mdebug(data, "data_invoke(%s, %s, %s)",
         data_tostring(self), name_tostring(name), (args) ? array_tostring(args) : "''");
  if (!self && array_size(args)) {
    self = data_array_get(args, 0);
    args_shifted = array_slice(args, 1, -1);
    if (!args_shifted) {
      args_shifted = data_array_create(1);
    }
    args = args_shifted;
  }
  if (!self) {
    ret = data_exception(ErrorArgCount, "No 'self' object specified for method '%s'", name);
  }

  if (!ret) {
    callable = data_resolve(self, name);
    if (data_is_exception(callable) && !data_as_exception(callable) -> handled) {
      ret = callable;
    } else if (callable) {
      if (data_is_callable(callable)) {
        ret = data_call(callable, args, kwargs);
      } else {
        ret = data_exception(ErrorNotCallable,
                         "Atom '%s' is not callable",
                         data_tostring(callable));
      }
      data_free(callable);
    } else {
      ret = data_exception(ErrorName, "Could not resolve '%s' in '%s'",
                       name_tostring(name), data_tostring(self));
    }
  }
  if (args_shifted) {
    array_free(args_shifted);
  }
  mdebug(data, "data_invoke(%s, %s, %s) = %s", data_tostring(self), name_tostring(name), array_tostring(args), data_tostring(ret));
  return ret;
}

data_t * data_execute(data_t *data, char *name, array_t *args, dict_t *kwargs) {
  name_t *n = name_create(1, name);
  data_t *ret;

  ret = data_invoke(data, n, args, kwargs);
  name_free(n);
  return ret;
}

data_t * data_get(data_t *data, name_t *name) {
  data_t *ret;

  ret = data_resolve(data, name);
  if (!ret) {
    ret = data_exception(ErrorName, "Could not resolve '%s' in '%s'",
                     name_tostring(name), data_tostring(data));
  }
  return ret;
}

data_t * data_get_attribute(data_t *data, char *name) {
  name_t *n;
  data_t *ret;

  n = name_create(1, name);
  ret = data_get(data, n);
  name_free(n);
  return ret;
}

data_t * data_set(data_t *data, name_t *name, data_t *value) {
  data_t      *container;
  name_t      *head;
  data_t      *ret;

  mdebug(data, "%s.set(%s:%d, %s)",
        data_tostring(data), name_tostring(name),
        name_size(name), data_tostring(value));
  if (name_size(name) == 1) {
    container = data;
  } else {
    assert(name_size(name) > 1);
    head = name_head(name);
    container = data_resolve(data, head);
    name_free(head);
  }
  if (data_is_exception(container)) {
    ret = container;
  } else if (container) {
    ret = _data_call_setter(data_typedescr(container), container, name_last(name), value);
  } else {
    ret = data_exception(ErrorName, "Could not resolve '%s' in '%s'",
                         name_tostring(name), data_tostring(data));
  }
  return ret;
}

data_t * data_set_attribute(data_t *data, char *name, data_t *value) {
  name_t *n;
  data_t *ret;

  n = name_create(1, name);
  ret = data_set(data, n, value);
  name_free(n);
  return ret;
}

int data_has(data_t *self, name_t *name) {
  data_t *attr = NULL;
  int     ret = 0;

  attr = data_resolve(self, name);
  if (attr && !data_is_exception(attr)) {
    ret = 1;
  }
  data_free(attr);
  return ret;
}

int data_has_callable(data_t *self, name_t *name) {
  data_t *callable = NULL;
  int     ret = 0;

  callable = data_resolve(self, name);
  if (callable && !data_is_unhandled_exception(callable) && data_is_callable(callable)) {
    ret = 1;
  }
  data_free(callable);
  return ret;
}

char * _data_tostring(data_t *data) {
  char        *ret;
  typedescr_t *type;
  tostring_t   tostring;

  if (!data) {
    return "null";
  } else if (data -> str && (data -> free_str == DontFreeData)) {
    return data -> str;
  } else {
    free(data -> str);
    data -> str = NULL;
    type = data_typedescr(data);
    tostring = (tostring_t) typedescr_get_function(type, FunctionAllocString);
    if (tostring) {
      data -> str = tostring(data);
    }
    if (!data -> str) {
      tostring = (tostring_t) typedescr_get_function(type, FunctionToString);
      if (tostring) {
        ret = tostring(data);
        if (ret && !data -> str) {
          data -> str = strdup(ret);
        }
      }
    }
    if (!data -> str) {
      tostring = (tostring_t) typedescr_get_function(type, FunctionStaticString);
      if (tostring) {
        data -> str = tostring(data);
        data -> free_str = DontFreeData;
      }
    }
    if (!data -> str) {
      asprintf(&(data -> str), "%s:%p", data_typename(data), data);
    }
    return data -> str;
  }
}

double _data_floatval(data_t *data) {
  double (*fltvalue)(data_t *);
  int    (*intvalue)(data_t *);
  data_t  *flt;
  double   ret;

  fltvalue = (double (*)(data_t *)) data_get_function(data, FunctionFltValue);
  if (fltvalue) {
    return fltvalue(data);
  } else {
    intvalue = (int (*)(data_t *)) data_get_function(data, FunctionIntValue);
    if (intvalue) {
      return (double) intvalue(data);
    } else {
      flt = data_cast(data, Float);
      if (flt && !data_is_exception(flt)) {
        ret = ((flt_t *) flt) -> dbl;
        data_free(flt);
        return ret;
      } else {
        return nan("Can't convert atom to float");
      }
    }
  }
}

int _data_intval(data_t *data) {
  int    (*intvalue)(data_t *);
  double (*fltvalue)(data_t *);
  data_t  *i = NULL;
  int      ret = 0;

  if (data) {
    intvalue = (int (*)(data_t *)) data_get_function(data, FunctionIntValue);
    if (intvalue) {
      ret = intvalue(data);
    } else {
      fltvalue = (double (*)(data_t *)) data_get_function(data, FunctionFltValue);
      if (fltvalue) {
        ret = (int) fltvalue(data);
      } else {
        if ((i = data_cast(data, Int)) && !data_is_exception(i)) {
          ret = ((int_t *) i) -> i;
        }
        data_free(i);
      }
    }
  }
  return ret;
}

int data_cmp(data_t *d1, data_t *d2) {
  typedescr_t *type;
  data_t      *p1 = NULL;
  data_t      *p2 = NULL;
  int          ret;
  cmp_t        cmp;

  mdebug(data, "Comparing '%s' [%s] and '%s' [%s]",
        data_tostring(d1), data_typename(d1),
        data_tostring(d2), data_typename(d2));
  if (d1 == d2) {
    return 0;
  } else if (!d1 || !d2) {
    return (!d1) ? -1 : 1;
  } else if (d1 -> type != d2 -> type) {
    p1 = data_promote(d1);
    if (p1 && (p1 -> type == d2 -> type)) {
      ret = data_cmp(p1, d2);
    } else {
      p2 = data_promote(d2);
      if (p2 && (d1 -> type == p2 -> type)) {
	      ret = data_cmp(d1, p2);
      } else if (p1 && !p2) {
	      ret = data_cmp(p1, d2);
      } else if (!p1 && p2) {
	      ret = data_cmp(d1, p2);
      } else if (p1 && p2) {
	      ret = data_cmp(p1, p2);
      } else {
	      ret = d1 -> type - d2 -> type;
      }
      data_free(p2);
    }
    data_free(p1);
    return ret;
  } else {
    type = data_typedescr(d1);
    cmp = (cmp_t) typedescr_get_function(type, FunctionCmp);
    return (cmp) ? cmp(d1, d2) : (d1 == d2);
  }
}

/* - G E N E R A L  M E T H O D S ------------------------------------------*/

data_t * data_len(data_t *data) {
  typedescr_t  *type;
  int         (*len)(data_t *);
  data_t       *ret = NULL;

  if (data) {
    type = data_typedescr(data);
    len = (int (*)(data_t *)) typedescr_get_function(type, FunctionLen);
    if (len && (len >= 0)) {
      ret = (data_t *) int_create(len(data));
    }
  }
  if (!ret) {
    ret = data_exception(ErrorFunctionUndefined,
                         "%s '%s' is has no 'len' function",
                         data_typename(data), data_tostring(data));
  }
  return ret;
}

/* - R E A D E R S / W R I T E R S ---------------------------------------- */

/**
 * @brief Read <code>num</code> bytes from reader <code>reader</code> into
 * buffer <code>buf</code>.
 */
data_t * data_read(data_t *reader, char *buf, int num) {
  typedescr_t  *type = data_typedescr(reader);
  read_t        fnc;
  int           retval;

  fnc = (read_t) typedescr_get_function(type, FunctionRead);
  if (fnc) {
    retval = fnc(reader, buf, num);
    if (retval >= 0) {
      return (data_t *) int_create(retval);
    } else {
      return data_exception_from_errno();
    }
  } else {
    return data_exception(ErrorFunctionUndefined,
                          "%s '%s' is has no 'read' function",
                          typedescr_tostring(data_typedescr(reader)),
                          data_tostring(reader));
  }
}

/**
 * @brief Write <code>num</code> bytes from <code>buf</code> to writer
 * <code>writer</code>.
 */
data_t * data_write(data_t *writer, char *buf, int num) {
  typedescr_t  *type = data_typedescr(writer);
  write_t       fnc;
  int           ret;

  fnc = (write_t) typedescr_get_function(type, FunctionWrite);
  if (fnc) {
    ret = fnc(writer, buf, num);
    if (ret >= 0) {
      return (data_t *) int_create(ret);
    } else {
      return data_exception_from_errno();
    }
  } else {
    return data_exception(ErrorFunctionUndefined,
                          "%s '%s' is has no 'write' function",
                          typedescr_tostring(type),
                          data_tostring(writer));
  }
}

/* - I T E R A T O R S -----------------------------------------------------*/

data_t * data_iter(data_t *data) {
  typedescr_t *type;
  data_fnc_t   iter;
  data_t      *ret = NULL;

  if (data_is_iterable(data)) {
    type = data_typedescr(data);
    iter = (data_fnc_t) typedescr_get_function(type, FunctionIter);
    if (iter) {
      ret = iter(data);
    }
    if (!ret) {
      ret = data_exception(ErrorExhausted,
                           "Iterator '%s' exhausted",
                           data_tostring(data));
    }
  } else if (data_is_iterator(data)) {
    ret = data_copy(data);
  } else {
    ret = data_exception(ErrorNotIterable,
                     "Atom '%s' is not iterable",
                     data_tostring(data));
  }
  return ret;
}

data_t * data_has_next(data_t *data) {
  typedescr_t *type;
  data_fnc_t   hasnext;
  data_t      *ret = NULL;

  if (data_is_iterator(data)) {
    type = data_typedescr(data);
    hasnext = (data_fnc_t) typedescr_get_function(type, FunctionHasNext);
    if (hasnext) {
      ret = hasnext(data);
    }
  }
  if (!ret) {
    ret = data_exception(ErrorNotIterator,
                         "Atom '%s' is not an iterator",
                         data_tostring(data));
  }
  return ret;
}

data_t * data_next(data_t *data) {
  typedescr_t *type;
  data_fnc_t   next;
  data_fnc_t   hasnext;
  data_t      *ret = NULL;
  data_t      *hn;

  if (data_is_iterator(data)) {
    type = data_typedescr(data);
    hasnext = (data_fnc_t) typedescr_get_function(type, FunctionHasNext);
    next = (data_fnc_t) typedescr_get_function(type, FunctionNext);
    if (next && hasnext) {
      hn = hasnext(data);
      ret = data_intval(hn)
        ? next(data)
        : data_exception(ErrorExhausted, "Iterator '%s' exhausted", data_tostring(data));
      data_free(hn);
    }
  }
  if (!ret) {
    ret = data_exception(ErrorNotIterator,
                         "Atom '%s' is not an iterator",
                         data_tostring(data));
  }
  return ret;
}

data_t * data_visit(data_t *iterable, data_t *visitor) {
  data_t *ret = data_reduce(iterable, visitor, data_null());

  return (data_is_unhandled_exception(ret)) ? ret : iterable;
}

data_t * data_reduce(data_t *iterable, data_t *reducer, data_t *initial) {
  data_t  *iterator = data_iter(iterable);
  data_t  *has_next = NULL;
  data_t  *current = NULL;
  data_t  *accum = NULL;
  data_t  *ret = iterable;
  array_t *args = NULL;

  /*
   * TODO: Allow types to provide their own FunctionReduce. Could be have
   * performance benefits.
   */

  if (data_is_unhandled_exception(iterator)) {
    return iterator;
  }
  has_next = data_has_next(iterator);
  if (data_is_unhandled_exception(has_next)) {
    ret = data_copy(has_next);
  } else {
    args = data_array_create(1);
    array_set(args, 0, data_copy(initial));
    while (data_intval(has_next)) {
      current = data_next(iterator);
      if (data_is_unhandled_exception(current)) {
        ret = data_copy(current);
        break;
      }
      array_set(args, 1, data_copy(current));
      accum = data_call(reducer, args, NULL);
      data_uncopy(data_array_get(args, 0));
      data_uncopy(data_array_get(args, 1));
      if (data_is_unhandled_exception(accum)) {
        ret = data_copy(accum);
        break;
      }
      data_free(has_next);
      has_next = data_has_next(iterator);
      if (data_is_unhandled_exception(has_next)) {
        ret = data_copy(has_next);
        break;
      }
      array_set(args, 0, data_copy(accum));
      data_free(ret);
      ret = data_copy(accum);
    }
  }
  array_free(args);
  data_free(accum);
  data_free(current);
  data_free(has_next);
  data_free(iterator);
  return ret;
}

data_t * data_reduce_with_fnc(data_t *iterable, reduce_t reduce, data_t *initial) {
  data_t  *iterator = data_iter(iterable);
  data_t  *has_next = NULL;
  data_t  *current = NULL;
  data_t  *accum = NULL;
  data_t  *current_accum = NULL;
  data_t  *ret = iterable;

  if (data_is_unhandled_exception(iterator)) {
    return iterator;
  }
  has_next = data_has_next(iterator);
  if (data_is_unhandled_exception(has_next)) {
    ret = data_copy(has_next);
  } else {
    current_accum = data_copy(initial);
    while (data_intval(has_next)) {
      current = data_next(iterator);
      if (data_is_unhandled_exception(current)) {
        ret = data_copy(current);
        break;
      }
      accum = reduce(current, current_accum);
      data_free(current);
      data_free(current_accum);
      if (data_is_unhandled_exception(accum)) {
        ret = data_copy(accum);
        break;
      }
      current_accum = accum;
      data_free(has_next);
      has_next = data_has_next(iterator);
      if (data_is_unhandled_exception(has_next)) {
        ret = data_copy(has_next);
        break;
      }
      data_free(ret);
      ret = data_copy(accum);
    }
  }
  data_free(accum);
  data_free(has_next);
  data_free(iterator);
  return ret;
}

/* - S C O P E -------------------------------------------------------------*/

data_t * data_push(data_t *scope, data_t *value) {
  typedescr_t *type;
  data2_fnc_t  pushfnc;
  data_t      *ret = NULL;

  if (scope) {
    type = data_typedescr(scope);
    pushfnc = (data2_fnc_t) typedescr_get_function(type, FunctionPush);
    if (pushfnc) {
      (void) pushfnc(scope, value);
      ret = scope;
    }
  }
  if (!ret) {
    ret = data_exception(ErrorType,
                         "Atom '%s' is not a Scope",
                         data_tostring(scope));
  }
  return ret;
}

data_t * data_pop(data_t *scope) {
  typedescr_t *type;
  data_fnc_t   popfnc;
  data_t      *ret = NULL;

  if (scope) {
    type = data_typedescr(scope);
    popfnc = (data_fnc_t) typedescr_get_function(type, FunctionPush);
    if (popfnc) {
      ret = popfnc(scope);
    }
  }
  if (!ret) {
    ret = data_exception(ErrorType,
                         "Atom '%s' is not a Scope",
                         data_tostring(scope));
  }
  return ret;
}

/* -------------------------------------------------------------------------*/

data_t * data_interpolate(data_t *self, array_t *args, dict_t *kwargs) {
  typedescr_t  *type;
  data_t *    (*interpolate)(data_t *, array_t *, dict_t *);
  data_t       *ret = self;

  assert(self);
  type = data_typedescr(self);
  interpolate = (data_t *(*)(data_t *, array_t *, dict_t *))
    typedescr_get_function(type, FunctionInterpolate);
  if ((args && array_size(args)) || (kwargs && dict_size(kwargs))) {
    ret = (interpolate)
      ? interpolate(self, args, kwargs)
      : (data_t *) str_format(data_tostring(self), args, kwargs);
  }
  return ret;
}

data_t * data_query(data_t *self, data_t *querytext) {
  typedescr_t  *type;
  data_t *    (*queryfnc)(data_t *, data_t *);
  data_t       *ret = NULL;

  assert(self);
  type = data_typedescr(self);
  queryfnc = (data_t * (*)(data_t *, data_t *))
    typedescr_get_function(type, FunctionQuery);
  if (queryfnc) {
    ret = queryfnc(self, querytext);
  }
  return ret;
}

int data_count(void) {
  return _data_count;
}

/* -- Standard vtable functions ----------------------------------------- */

/* -- Standard reducer functions ----------------------------------------- */

array_t * data_add_strings_reducer(data_t *data, array_t *target) {
  array_push(target, strdup(data_tostring(data)));
  return target;
}

array_t * data_add_all_reducer(data_t *data, array_t *target) {
  array_push(target, data_copy(data));
  return target;
}

array_t * data_add_all_as_data_reducer(char *str, array_t *target) {
  array_push(target, (data_t *) str_copy_chars(str));
  return target;
}

dict_t * data_put_all_reducer(entry_t *e, dict_t *target) {
  dict_put(target, strdup(e -> key), data_copy(e -> value));
  return target;
}
