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

#include <string.h>
#include "expr.h"

#define COPY_BYTES(ptr, type)  _memcpy_bytes((ptr), sizeof(type))

static void *   _memcpy_bytes(void *, size_t);
static void *   _copy_int(int *);
static void *   _copy_float(double *);
static void *   _copy_bool(unsigned char *);
static expr_t * _expr_create(expr_t *, eval_t, void *, context_t *);
static list_t * _expr_eval_node(expr_t *, list_t *);
static expr_t * _expr_literal(expr_t *, datatype_t, void *);

static data_t * _evaluate_literal(context_t *ctx, void *data, list_t *params);
static data_t * _evaluate_deref(context_t *ctx, void *data, list_t *params);
static data_t * _evaluate_call(context_t *ctx, void *data, list_t *params);

// -----------------------
// Data conversion support

static typedescr_t descriptors[] = {
  { type: String,   assign: (assignment_t) strdup,      release: (free_t) free, size: sizeof(char *) },
  { type: Int,      assign: (assignment_t) _copy_int,   release: (free_t) free, size: sizeof(int) },
  { type: Float,    assign: (assignment_t) _copy_float, release: (free_t) free, size: sizeof(float) },
  { type: Bool,     assign: (assignment_t) _copy_bool,  release: (free_t) free, size: sizeof(unsigned char) },
  { type: Function, assign: NULL,                       release: NULL }
};

void * _memcpy_bytes(void *val, size_t bytes) {
  void *ret = malloc(bytes);
  if (ret) {
    memcpy(ret, val, bytes);
  }
  return ret;
}

void * _copy_int(int *val) {
  return COPY_BYTES(val, int);
}

void * _copy_float(double *val) {
  return COPY_BYTES(val, double);
}

void * _copy_bool(unsigned char *val) {
  return COPY_BYTES(val, unsigned char);
}

// --------------------
// data_t

data_t * data_create(datatype_t type, void *value) {
  data_t *ret;

  ret = NEW(data_t);
  if (ret) {
    ret -> type = type;
    ret -> value = (descriptors[type].assign)
      ? descriptors[type].assign(value)
      : value;
  }
  return ret; 
}

void data_free(data_t *data) {
  if (descriptors[data -> type].release) {
    descriptors[data -> type].release(data -> value);
  }
  free(data);
}

data_t * data_copy(data_t *src) {
  data_t *ret;

  ret = NEW(data_t);
  if (ret) {
    memcpy(ret, src, sizeof(data_t));
    if (descriptors[src -> type].assign) {
      ret -> value = descriptors[src -> type].assign(src -> value);
    }
  }
  return ret;
}

data_t * data_get(data_t *data, void *ptr) {
  if (descriptors[data -> type].copy) {
    descriptors[data -> type].copy(ptr, data -> value);
  } else {
    memcpy(ptr, data -> value, descriptors[data -> type].size);
  }
  return data;
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
  dict_free(ctx -> vars);
  free(ctx);
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
  char *fmt;
  
  d = (data_t *) data;
  switch (d -> type) {
    case Int:
    case Bool:
      fmt = "%d";
      break;
    case Float:
      fmt = "%f";
      break;
    case String:
      fmt = "%s";
      break;
  }
  debug(fmt, *((int *) d -> value));
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
  if (ret && expr) {
    if (!list_append(expr -> nodes, ret)) {
      ret = NULL;
    }
  }
  return ret;
}

expr_t * expr_set_context(expr_t *expr, context_t *ctx) {
  expr -> context = ctx;
  return expr;
}

expr_t * expr_set_data_free(expr_t *expr, free_t data_free) {
  expr -> data_free = data_free;
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
  
  ret = NULL;
  data = data_create(Function, NULL);
  if (data) {
    data -> fnc = (eval_t) fnc;
    ret = expr_create(expr, _evaluate_call, data);
    if (ret) {
      expr_set_data_free(ret, (free_t) data_free);
    } else {
      data_free(data);
    }
  }
  return ret;
}