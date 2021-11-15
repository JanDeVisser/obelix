/*
 * /obelix/src/parser/generator.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include "libruntime.h"

#include <vm.h>

static inline void   _generator_init(void);
static generator_t * _generator_new(generator_t *, va_list);
static void          _generator_free(generator_t *);
static char *        _generator_allocstring(generator_t *);
static data_t *      _generator_set(generator_t *, char *, data_t *);
static data_t *      _generator_resolve(generator_t *, char *);
static data_t *      _generator_call(generator_t *, arguments_t *);
static data_t *      _generator_iter(generator_t *);
static data_t *      _generator_has_next(generator_t *);
static data_t *      _generator_interrupt(data_t *, char *, arguments_t *);
static generator_t * _generator_next(generator_t *);

static vtable_t _vtable_Generator[] = {
  { .id = FunctionNew,         .fnc = (void_t) _generator_new },
  { .id = FunctionFree,        .fnc = (void_t) _generator_free },
  { .id = FunctionAllocString, .fnc = (void_t) _generator_allocstring },
  { .id = FunctionIter,        .fnc = (void_t) _generator_iter },
  { .id = FunctionNext,        .fnc = (void_t) generator_next },
  { .id = FunctionHasNext,     .fnc = (void_t) _generator_has_next },
  { .id = FunctionCall,        .fnc = (void_t) _generator_call },
  { .id = FunctionSet,         .fnc = (void_t) _generator_set },
  { .id = FunctionResolve,     .fnc = (void_t) _generator_resolve },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_Generator[] = {
  { .type = -1,      .name = "stop", .method = _generator_interrupt, .argtypes = { NoType, NoType, NoType }, .minargs = 1, .varargs = 1 },
  { .type = NoType,  .name = NULL,   .method = NULL,                 .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

int Generator = -1;

/* ------------------------------------------------------------------------ */

void _generator_init(void) {
  if (Generator < 1) {
    typedescr_register_with_methods(Generator, generator_t);
  }
}

/* -- G E N E R A T O R   D A T A T Y P E --------------------------------- */

generator_t * _generator_new(generator_t *generator, va_list args) {
  generator -> closure = closure_copy(va_arg(args, closure_t *));
  generator -> ast = ast_Expr_copy(va_arg(args, ast_Expr_t *));
  generator -> status = exception_copy(va_arg(args, exception_t *));
  return generator;
}

void _generator_free(generator_t *generator) {
  if (generator) {
    closure_free(generator -> closure);
    ast_Expr_free(generator -> ast);
    exception_free(generator -> status);
    free(generator);
  }
}

char * _generator_allocstring(generator_t *generator) {
  char *buf;

  asprintf(&buf, "<<Generator %s>>",
           closure_tostring(generator -> closure));
  return buf;
}

data_t * _generator_iter(generator_t *generator) {
  return data_copy((data_t *) generator);
}

data_t * _generator_has_next(generator_t *generator) {
  return int_as_bool(generator_has_next(generator));
}

generator_t * _generator_next(generator_t *generator) {
  exception_free(generator -> status);
  generator -> status = closure_yield(generator -> closure, generator -> ast);
  return generator;
}

data_t * _generator_call(generator_t *generator, arguments_t *args) {
  // Materialize the generated values to a list.
  return data_null();
}

data_t * _generator_resolve(generator_t *generator, char *name) {
  return closure_resolve(generator -> closure, name);
}

data_t *_generator_set(generator_t *generator, char *name, data_t *value) {
  return closure_set(generator -> closure, name, value);
}

data_t * _generator_interrupt(data_t *generator, char *name, arguments_t *args) {
  generator_interrupt(data_as_generator(generator));
  return generator;
}


/* ------------------------------------------------------------------------ */

generator_t * generator_create(closure_t *closure, exception_t *status) {
  _generator_init();
  return (generator_t *) data_create(Generator, closure, status);
}

int generator_has_next(generator_t *generator) {
  if (!generator -> status) {
    _generator_next(generator);
  }
  return generator -> status -> code == ErrorYield;
}

data_t * generator_next(generator_t *generator) {
  data_t *ret = NULL;

  if (!generator -> status) {
    _generator_next(generator);
  }
  switch (generator -> status -> code) {
    case ErrorYield:
      ret = data_copy(generator -> status -> throwable);
      exception_free(generator -> status);
      generator -> status = NULL;
      break;
    default:
      ret = data_copy((data_t *) generator -> status);
      break;
  }
  return ret;
}

generator_t * generator_interrupt(generator_t *generator) {
  exception_free(generator -> status);
  generator -> status = exception_create(ErrorExhausted,
                                         "Generator Interrupted");
  return generator;
}

/* ------------------------------------------------------------------------ */
