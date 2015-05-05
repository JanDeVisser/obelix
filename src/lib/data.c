/*
 * /obelix/src/types/data.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <core.h>
#include <data.h>
#include <exception.h>
#include <list.h>
#include <logging.h>
#include <object.h>


static void     _data_init(void) __attribute__((constructor));
static void     _data_call_free(typedescr_t *, data_t *);

int             debug_data = 0;
int             _data_count = 0;

/* -- D A T A  S T A T I C  F U N C T I O N S ----------------------------- */

void _data_init(void) {
  logging_register_category("data", &debug_data);
}

/* -- D A T A  P U B L I C  F U N C T I O N S ----------------------------- */

data_t * data_create_noinit(int type) {
  return data_settype(NEW(data_t), type);
}

data_t * data_settype(data_t *data, int type) {
  typedescr_t *descr = typedescr_get(type);

  data -> type = type;
  data -> refs++;
  data -> str = NULL;
  data -> free_me = TRUE;
  descr -> count++;
  _data_count++;
  return data;
}

data_t * data_create(int type, ...) {
  va_list      arg;
  data_t      *ret;
  data_t      *initialized;
  typedescr_t *descr = typedescr_get(type);
  new_t        n;
  factory_t    f;
  
  f = (factory_t) typedescr_get_function(descr, FunctionFactory);
  if (f) {
    va_start(arg, type);
    initialized = f(type, arg);
    data_settype(initialized, type);
    initialized -> free_me = FALSE;
    va_end(arg);
  } else {
    ret = data_create_noinit(type);
    n = (new_t) typedescr_get_function(descr, FunctionNew);
    if (n) {
      va_start(arg, type);
      initialized = n(ret, arg);
      va_end(arg);
      if (initialized != ret) {
        data_free(ret);
      }
    } else {
      initialized = ret;
    }
    initialized -> free_me = TRUE;
  }
  return initialized;
}

data_t * data_parse(int type, char *str) {
  typedescr_t *descr;
  void_t       parse;

  descr = typedescr_get(type);
  parse = typedescr_get_function(descr, FunctionParse);
  return (parse) ? ((data_t * (*)(typedescr_t *, char *)) parse)(descr, str) : NULL;
}

data_t * data_decode(char *encoded) {
  char        *ptr;
  typedescr_t *type;
  data_t      *ret = NULL;
  
  if (!encoded || !encoded[0]) return NULL;
  ptr = strchr(encoded, ':');
  if (!ptr) {
    return data_create(String, encoded);
  } else {
    *ptr = 0;
    type = typedescr_get_byname(encoded);
    if (type) {
      ret = data_parse(type -> type, ptr + 1);
    }
    *ptr = ':';
    if (!ret) {
      ret = data_create(String, encoded);
    }
    return ret;
  }
}

data_t * data_cast(data_t *data, int totype) {
  typedescr_t *descr;
  typedescr_t *totype_descr;
  cast_t       cast;
  tostring_t   tostring;
  void_t       parse;

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
        return data_create(String, tostring(data));
      }
    }
    if (data_type(data) == String) {
      parse = typedescr_get_function(totype_descr, FunctionParse);
      if (parse) {
        return ((data_t * (*)(typedescr_t *, char *)) parse)(totype_descr, data -> ptrval);
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

void _data_call_free(typedescr_t *type, data_t *data) {
  free_t f;
  int    ix;
  
  f = (free_t) typedescr_get_function(type, FunctionFreeData);
  if (f) {
    f(data);
  }
  f = (free_t) typedescr_get_function(type, FunctionFree);
  if (f) {
    f(data -> ptrval);
  }
  for (ix = 0; ix < type -> inherits_size; ix++) {
    _data_call_free(typedescr_get(type -> inherits[ix]), data);
  }
}

void data_free(data_t *data) {
  typedescr_t *type;
  
  if (data && (--data -> refs <= 0)) {
    type = data_typedescr(data);
    _data_call_free(type, data);
    free(data -> str);
    type -> count--;
    _data_count--;
    if (data -> free_me) {
      free(data);
    }
  }
}

typedescr_t * data_typedescr(data_t *data) {
  return (data) ? typedescr_get(data_type(data)) : NULL;
}

void_t data_get_function(data_t *data, int fnc_id) {
  return typedescr_get_function(data_typedescr(data), fnc_id);
}

int data_is_numeric(data_t *data) {
  return data && typedescr_is(data_typedescr(data), Number);
}

int data_is_callable(data_t *data) {
  typedescr_t *td;
  
  if (data) {
    td = data_typedescr(data);
    assert(td);
    return (typedescr_get_function(td, FunctionCall) != NULL);
  } else {
    return 0;
  }
}

int data_hastype(data_t *data, int type) {
  if (type == Any) {
    return TRUE;
  } else if (type == Callable) {
    return data_is_callable(data);
  } else {
    return data && typedescr_is(data_typedescr(data), type);
  }
}

data_t * data_copy(data_t *src) {
  if (src) {
    src -> refs++;
  }
  return src;
}

data_t * data_call(data_t *self, array_t *args, dict_t *kwargs) {
  call_t call = (call_t) data_get_function(self, FunctionCall);
  
  assert(data_is_callable(self));
  return call(self, args, kwargs);
}

data_t * data_method(data_t *data, char *name) {
  typedescr_t   *type = data_typedescr(data);
  methoddescr_t *md;
  data_t        *ret = NULL;
  
  md = typedescr_get_method(type, name);
  if (md) {
    ret = data_create(Method, md, data);
  }
  return ret;
}

data_t * data_resolve(data_t *data, name_t *name) {
  typedescr_t    *type = data_typedescr(data);
  data_t         *ret = NULL;
  data_t         *tail_resolve;
  resolve_name_t  resolve;
  name_t         *tail;
  
  assert(type);
  assert(name);
  if (debug_data) {
    debug("%s.resolve(%s:%d)", data_tostring(data), name_tostring(name), name_size(name));
  }
  if (name_size(name) == 0) {
    ret = data_copy(data);
  } else if (name_size(name) == 1) {
    ret = data_method(data, name_first(name));
  }
  if (!ret) {
    resolve = (resolve_name_t) typedescr_get_function(type, FunctionResolve);
    if (!resolve) {
      ret = data_exception(ErrorType,
                           "Cannot resolve name '%s' in %s '%s'", 
                           name_tostring(name),
                           type -> type_name, 
                           data_tostring(data));
    } else {
      ret = resolve(data, name_first(name));
    }
  }
  if (ret && (!data_is_exception(ret) || data_exceptionval(ret) -> handled) 
          && (name_size(name) > 1)) {
    tail = name_tail(name);
    tail_resolve = data_resolve(ret, tail);
    data_free(ret);
    ret = tail_resolve;
    name_free(tail);
  }
  if (debug_data) {
    debug("%s.resolve(%s) = %s", data_tostring(data), name_tostring(name), data_tostring(ret));
  }
  return ret;
}

data_t * data_invoke(data_t *self, name_t *name, array_t *args, dict_t *kwargs) {
  data_t  *callable;
  data_t  *ret = NULL;
  array_t *args_shifted = NULL;
  
  if (debug_data) {
    debug("data_invoke(%s, %s, %s)", 
          data_tostring(self), 
          name_tostring(name), 
          (args) ? array_tostring(args) : "''");
  }
  if (!self && array_size(args)) {
    self = (data_t *) array_get(args, 0);
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
    if (data_is_exception(callable) && !data_exceptionval(callable) -> handled) {
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
  if (debug_data) {
    debug("data_invoke(%s, %s, %s) = %s", data_tostring(self), name_tostring(name), array_tostring(args), data_tostring(ret));
  }
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
  if (callable && !data_is_exception(callable) && data_is_callable(callable)) {
    ret = 1;
  }
  data_free(callable);
  return ret;
}

data_t * data_set(data_t *data, name_t *name, data_t *value) {
  typedescr_t *type;
  data_t      *container;
  name_t      *head;
  data_t      *ret;
  setvalue_t   setter;

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
    type = data_typedescr(container);
    setter = (setvalue_t) typedescr_get_function(type, FunctionSet);
    if (!setter) {
      ret = data_exception(ErrorType,
                       "Cannot set values on objects of type '%s'", 
                       typedescr_tostring(type));
    } else {
      ret = setter(container, name_last(name), value);
    }
  } else {
    ret = data_exception(ErrorName, "Could not resolve '%s' in '%s'", 
                     name_tostring(name), data_tostring(data));
  }
  return ret;
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

char * data_tostring(data_t *data) {
  int          len;
  char        *ret;
  typedescr_t *type;
  tostring_t   tostring;

  if (!data) {
    return "data:NULL";
  } else {
    free(data -> str);
    data -> str = NULL;
    type = data_typedescr(data);
    tostring = (tostring_t) typedescr_get_function(type, FunctionToString);
    if (tostring) {
      ret = tostring(data);
      if (ret && !data -> str) {
	data -> str = strdup(ret);
      }
    } else {
      len = asprintf(&(data -> str), "data:%s:%p", data_typedescr(data) -> type_name, data);
    }
    return data -> str;
  }
}

int data_cmp(data_t *d1, data_t *d2) {
  typedescr_t *type;
  data_t      *p1 = NULL;
  data_t      *p2 = NULL;
  int          ret;
  cmp_t        cmp;

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
    return (cmp) ? cmp(d1, d2) : ((d1 -> ptrval == d2 -> ptrval) ? 0 : 1);
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
    if (len) {
      ret = data_create(Int, len(data));
    }
  }
  if (!ret) {
    ret = data_exception(ErrorFunctionUndefined,
                     "Atom '%s' is has no size function",
                     data_tostring(data));
  }
  return ret;
}

/* - I T E R A T O R S -----------------------------------------------------*/

int data_is_iterable(data_t *data) {
  typedescr_t *td;
  
  if (data) {
    td = data_typedescr(data);
    assert(td);
    return (typedescr_get_function(td, FunctionIter) != NULL);
  } else {
    return 0;
  }  
}

int data_is_iterator(data_t *data) {
  typedescr_t *td;
  
  if (data) {
    td = data_typedescr(data);
    assert(td);
    return (typedescr_get_function(td, FunctionNext) != NULL) &&
           (typedescr_get_function(td, FunctionHasNext) != NULL);
  } else {
    return 0;
  }  
}

data_t * data_iter(data_t *data) {
  typedescr_t *type;
  data_fnc_t   iter;
  data_t      *ret = NULL;

  if (data) {
    type = data_typedescr(data);
    iter = (data_fnc_t) typedescr_get_function(type, FunctionIter);
    if (iter) {
      ret = iter(data);
    }
  }
  if (!ret) {
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

  if (data) {
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

  if (data) {
    type = data_typedescr(data);
    hasnext = (data_fnc_t) typedescr_get_function(type, FunctionHasNext);
    next = (data_fnc_t) typedescr_get_function(type, FunctionNext);
    if (next && hasnext) {
      hn = hasnext(data);
      ret = (hn -> intval)
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

double data_floatval(data_t *data) {
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
        ret = flt -> dblval;
        data_free(flt);
        return ret;
      } else {
        return nan("Can't convert atom to float");
      }
    }
  }
}

int data_intval(data_t *data) {
  int    (*intvalue)(data_t *);
  double (*fltvalue)(data_t *);
  data_t  *i;
  int      ret;
  
  intvalue = (int (*)(data_t *)) data_get_function(data, FunctionIntValue);
  if (intvalue) {
    return intvalue(data);
  } else {
    fltvalue = (double (*)(data_t *)) data_get_function(data, FunctionFltValue);
    if (fltvalue) {
      return (int) fltvalue(data);
    } else {
      i = data_cast(data, Int);
      if (i && !data_is_exception(i)) {
        ret = i -> intval;
        data_free(i);
        return ret;
      } else {
        return 0;
      }
    }
  }
}

/* -------------------------------------------------------------------------*/

int data_count(void) {
  return _data_count;
}

/*
 * Standard reducer functions
 */

array_t * data_add_strings_reducer(data_t *data, array_t *target) {
  array_push(target, strdup(data_tostring(data)));
  return target;
}

array_t * data_add_all_reducer(data_t *data, array_t *target) {
  array_push(target, data_copy(data));
  return target;
}

array_t * data_add_all_as_data_reducer(char *str, array_t *target) {
  array_push(target, data_create(String, str));
  return target;
}

dict_t * data_put_all_reducer(entry_t *e, dict_t *target) {
  dict_put(target, strdup(e -> key), data_copy(e -> value));
  return target;
}

