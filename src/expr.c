/*
 * expr.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
 * 
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <expr.h>

#include <string.h>
#include <stdio.h>

typedef data_t * (*copydata_t)(void *, void *);

static data_t * _data_create(datatype_t);
static data_t * _data_copy_string(data_t *, data_t *);
static int      _data_cmp_string(data_t *, data_t *);
static int      _data_cmp_int(data_t *, data_t *);
static int      _data_cmp_float(data_t *, data_t *);
static int      _data_cmp_function(data_t *, data_t *);
static char *   _data_tostring_string(data_t *);
static char *   _data_tostring_int(data_t *);
static char *   _data_tostring_float(data_t *);
static char *   _data_tostring_bool(data_t *);
static char *   _data_tostring_function(data_t *);

static expr_t * _expr_create(expr_t *, eval_t, void *, context_t *);
static list_t * _expr_eval_node(expr_t *, list_t *);
static expr_t * _expr_literal(expr_t *, datatype_t, void *);

static data_t * _evaluate_literal(context_t *ctx, void *data, list_t *params);
static data_t * _evaluate_deref(context_t *ctx, void *data, list_t *params);
static data_t * _evaluate_call(context_t *ctx, void *data, list_t *params);

// -----------------------
// Data conversion support

typedef struct _typedescr {
  datatype_t    type;
  copydata_t    copy;
  cmp_t         cmp;
  free_t        free;
  tostring_t    tostring;
} typedescr_t;

static typedescr_t descriptors[] = {
  {
    type: String,
    copy: (copydata_t) _data_copy_string,
    cmp: (cmp_t) _data_cmp_string,
    free: (free_t) free,
    tostring: (tostring_t) _data_tostring_string
  },
  {
    type: Int,
    copy: NULL,
    cmp: (cmp_t) _data_cmp_int,
    free: NULL,
    tostring: (tostring_t) _data_tostring_int
  },
  {
    type: Float,
    copy: NULL,
    cmp: (cmp_t) _data_cmp_float,
    free: NULL,
    tostring: (tostring_t) _data_tostring_float
  },
  {
    type: Bool,
    copy: NULL,
    cmp: (cmp_t) _data_cmp_int,
    free: NULL,
    tostring: (tostring_t) _data_tostring_bool
  },
  {
    type: Function,
    copy: NULL,
    cmp: (cmp_t) _data_cmp_function,
    free: NULL,
    tostring: (tostring_t) _data_tostring_function
  }
};

/*
 * data_t static functions
 */

data_t * _data_create(datatype_t type) {
  data_t *ret;

  ret = NEW(data_t);
  ret -> type = type;
  return ret;
}

data_t * _data_copy_string(data_t *target, data_t *src) {
  target -> ptrval = strdup(src -> ptrval);
  return target;
}

int _data_cmp_string(data_t *d1, data_t *d2) {
  return strcmp((char *) d1 -> ptrval, (char *) d2 -> ptrval);
}

int _data_cmp_int(data_t *d1, data_t *d2) {
  return d1 -> intval - d2 -> intval;
}

int _data_cmp_float(data_t *d1, data_t *d2) {
  return (d1 -> dblval == d2 -> dblval)
      ? 0
      : (d1 -> dblval > d2 -> dblval) ? 1 : -1;
}

int _data_cmp_function(data_t *d1, data_t *d2) {
  return (d1 -> fnc == d2 -> fnc) ? 0 : 1;
}

char * _data_tostring_string(data_t *data) {
  return (char *) data -> ptrval;
}

char * _data_tostring_int(data_t *data) {
  return itoa(data -> intval);
}

char * _data_tostring_float(data_t *data) {
  return dtoa(data -> dblval);
}

char * _data_tostring_bool(data_t *data) {
  return btoa(data -> intval);
}

char * _data_tostring_function(data_t *data) {
  static char buf[32];

  snprintf(buf, 32, "<<function %p>>", (void *) data -> fnc);
  return buf;
}

/*
 * data_t public functions
 */

data_t * data_create(datatype_t type, ...) {
  va_list  arg;

  va_start(arg, type);
  switch (type) {
    case Int:
      return data_create_int(va_arg(arg, long));
    case Float:
      return data_create_float(va_arg(arg, double));
    case Bool:
      return data_create_bool(va_arg(arg, long));
    case String:
      return data_create_string(va_arg(arg, char *));
    case Function:
      return data_create_function(va_arg(arg, voidptr_t));
  }
}

data_t * data_create_int(long value) {
  data_t *ret;

  ret = _data_create(Int);
  ret -> intval = value;
  return ret;
}

data_t * data_create_float(double value) {
  data_t *ret;

  ret = _data_create(Float);
  ret -> dblval = value;
  return ret;
}

data_t * data_create_bool(long value) {
  data_t *ret;

  ret = _data_create(Bool);
  ret -> intval = value ? 1 : 0;
  return ret;
}

data_t * data_create_string(char * value) {
  data_t *ret;

  ret = _data_create(String);
  ret -> ptrval = strdup(value);
  return ret;
}

data_t * data_create_function(voidptr_t fnc) {
  data_t *ret;

  ret = _data_create(Function);
  ret -> fnc = fnc;
  return ret;
}

void data_free(data_t *data) {
  if (data) {
    if (descriptors[data -> type].free) {
      descriptors[data -> type].free(data -> ptrval);
    }
    free(data);
  }
}

data_t * data_copy(data_t *src) {
  data_t *ret;

  ret = NEW(data_t);
  if (ret) {
    memcpy(ret, src, sizeof(data_t));
    if (descriptors[src -> type].copy) {
      descriptors[src -> type].copy(ret, src);
    }
  }
  return ret;
}

char * data_tostring(data_t *data) {
  if (descriptors[data -> type].tostring) {
    return descriptors[data -> type].tostring(data);
  } else {
    return "<< ?? >>";
  }
}

// --------------------
// context_t

context_t * context_create(context_t *up) {
  context_t *ret;

  ret = NEW(context_t);
  if (ret) {
    ret -> up = up;
    ret -> vars = dict_create((cmp_t) strcmp);
    if (ret -> vars) {
      dict_set_free_key(ret -> vars, (free_t) free);
      dict_set_free_data(ret -> vars, (free_t) data_free);
      dict_set_hash(ret -> vars, (hash_t) strhash);
    } else {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

void context_free(context_t *ctx) {
  if (ctx) {
    dict_free(ctx -> vars);
    free(ctx);
  }
}

context_t * context_up(context_t *ctx) {
  return ctx -> up;
}

data_t * context_resolve(context_t *ctx, char *var) {
  data_t *ret;

  ret = (data_t *) dict_get(ctx -> vars, var);
  if (!ret && ctx -> up) {
    ret = context_resolve(ctx -> up, var);
  }
  return ret;
}

context_t * context_set(context_t *ctx, char *var, data_t *value) {
  data_t *data;

  data = data_copy(value);
  if (data) {
    return (dict_put(ctx -> vars, var, data)) ? ctx : NULL;
  } else {
    return NULL;
  }
}

// --------------------
// expr_t static methods

expr_t * _expr_create(expr_t *expr, eval_t eval, void *data, context_t *context) {
  expr_t *ret;
  void   *ok;
  
  ok = NULL;
  ret = NEW(expr_t);
  if (ret) {
    ret -> up = expr;
    ret -> eval = eval;
    ret -> data = data;
    ret -> context = context;
    ret -> nodes = list_create();
    ok = ret -> nodes;
    if (ok) {
      list_set_free(ret -> nodes, (free_t) expr_free);
      ok = ret;
    } else {
      expr_free(ret);
    }
  }
  return (expr_t *) ok;
}

list_t * _expr_eval_node(expr_t *node, list_t *params) {
  data_t *param;
  void *ok;
  
  ok = params;
  if (ok) {
    param = expr_evaluate(node);
    ok = param;
    if (ok) {
      ok = list_append(params, param);
      if (ok) {
	ok = params;
      }
    }
  }
  return (list_t *) ok;
}

data_t * _evaluate_literal(context_t *ctx, void *data, list_t *params) {
  data_t *d;
  
  d = (data_t *) data;
  debug("%s", data_tostring(d));
  return d;
}

data_t * _evaluate_deref(context_t *ctx, void *data, list_t *params) {
  return context_resolve(ctx, (char *) data);
}

data_t * _evaluate_call(context_t *ctx, void *data, list_t *params) {
  data_t *fnc;
  eval_t eval;
  
  fnc = (data_t *) data;
  eval = (eval_t) (fnc -> fnc);
  return eval(ctx, data, params);
}

expr_t * _expr_literal(expr_t *expr, datatype_t type, void *ptr) {
  expr_t *ret;
  data_t *data;
  
  ret = NULL;
  data = data_create(type, ptr);
  if (data) {
    ret = expr_create(expr, _evaluate_literal, data);
    if (ret) {
      expr_set_data_free(ret, (free_t) data_free);
    } else {
      data_free(data);
    }
  }
  return ret;
}

// --------------------
// expr_t

expr_t * expr_create(expr_t *expr, eval_t eval, void *data) {
  expr_t *ret;
  
  ret = _expr_create(expr, eval, data, (expr) ? expr -> context : NULL);
  if (ret) {
    ret = expr_add_node(expr, ret);
  }
  return ret;
}

expr_t * expr_set_context(expr_t *expr, context_t *ctx) {
  expr -> context = ctx;
  return expr;
}

expr_t * expr_set_data_free(expr_t *expr, free_t data_free) {
  expr -> data_free = data_free;
  return expr;
}

expr_t * expr_add_node(expr_t *expr, expr_t *node) {
  expr_t *ret;

  ret = expr;
  if (expr) {
    if (!list_append(expr -> nodes, ret)) {
      ret = NULL;
    } else {
      node -> up = expr;
    }
  }
  return ret;
}

void expr_free(expr_t *expr) {
  if (expr) {
    if (expr -> nodes) list_free(expr -> nodes);
    if (expr -> data && expr -> data_free) {
      expr -> data_free(expr -> data);
    }
    free(expr);
  }
}

data_t * expr_evaluate(expr_t *expr) {
  list_t *params;
  data_t *ret;
  void *ok;
  
  ret = NULL;
  params = list_create();
  ok = params;
  if (ok) {
    ok = list_reduce(expr -> nodes, (reduce_t) _expr_eval_node, params);
    if (ok) {
      ret = expr -> eval(expr -> context, expr -> data, params);
    }
    list_free(params);
  }
  return ret;
}

expr_t * expr_str_literal(expr_t *expr, char *str) {
  return _expr_literal(expr, String, str);
}

expr_t * expr_int_literal(expr_t *expr, int val) {
  return _expr_literal(expr, Int, &val);
}

expr_t * expr_float_literal(expr_t *expr, float val) {
  return _expr_literal(expr, Float, &val);
}

expr_t * expr_bool_literal(expr_t *expr, unsigned char val) {
  return _expr_literal(expr, Bool, &val);
}

expr_t * expr_deref(expr_t *expr, char *name) {
  expr_t *ret;
  void *data;
  
  ret = NULL;
  data = strdup(name);
  if (data) {
    ret = expr_create(expr, _evaluate_deref, name);
    if (ret) {
      expr_set_data_free(ret, (free_t) free);
    } else {
      free(data);
    }
  }
  return ret;
}

expr_t * expr_funccall(expr_t *expr, eval_t fnc) {
  expr_t *ret;
  data_t *data;
  
  data = data_create(Function, (voidptr_t) fnc);
  ret = expr_create(expr, _evaluate_call, data);
  expr_set_data_free(ret, (free_t) data_free);
  return ret;
}


#ifdef FOOKNARF

/* Move me */

static void expr_test(void);

void expr_test(void) {
  expr_t *expr, *e;
  data_t *result;
  int res;

  //expr = expr_funccall(NULL, _add);
  expr_int_literal(expr, 1);
  expr_int_literal(expr, 2);
  //e = expr_funccall(expr, _mult);
  expr_int_literal(e, 2);
  expr_int_literal(e, 3);

  result = expr_evaluate(expr);
  data_get(result, &res);
  debug("result: %d", res);
  data_free(result);
  expr_free(expr);
}

#endif /* FOOKNARF */
