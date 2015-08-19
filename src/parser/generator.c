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

	#include <generator.h>

static void     _generator_init(void) __attribute__((constructor));
static void     _generator_free(generator_t *generator);
static char *   _generator_allocstring(generator_t *generator);

static vtable_t _vtable_generator[] = {
  { .id = FunctionFree,        .fnc = (void_t) _generator_free },
  { .id = FunctionAllocString, .fnc = (void_t) _generator_allocstring },
  { .id = FunctionNone,        .fnc = NULL }
};

int Generator = -1;

/* ------------------------------------------------------------------------ */

void _generator_init(void) {
  Generator = typedescr_create_and_register("generator",
                                            _vtable_generator,
                                            NULL);
}

/* -- G E N E R A T O R   D A T A T Y P E --------------------------------- */

void _generator_free(generator_t *generator) {
  if (generator) {
    closure_free(generator -> closure);
    vm_free(generator -> vm);
    data_free(generator -> value);
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
  return data_copy(generator);
}

data_t * _generator_next(generator_t *generator) {
  data_t *ret = NULL;

  if (generator -> status) {
    if (generator -> status -> code == ErrorYield) {
      ret = generator -> status -> throwable;
      generator -> status = closure_yield(generator -> status, generator -> vm);
      if (generator -> status -> code != ErrorYield) {
      }
    } else if (generator -> status -> code != ErrorExhausted) {
      ret = generator -> status;
    }
  } else {
    fatal("Generator '%s' has no status??", generator_tostring(generator));
  }
  return ret;
}

data_t * _generator_has_next(generator_t *generator) {
  return data_create(Bool, generator -> status && (generator -> status -> code == ErrorYield));
}

data_t * _generator_call(generator_t *generator, array_t *args, dict_t *kwargs) {
  // Materialize the generated values to a list.
	  return data_null();
}

/* ------------------------------------------------------------------------ */

generator_t * generator_create(closure_t *closure, vm_t *vm, exception_t *status) {
  generator_t *ret = (generator_t) data_new(Generator, generator_t);

  ret -> closure = closure_copy(closure);
  ret -> vm = vm_copy(vm);
  ret -> status = exception_copy(status);
  return ret;
}

inline generator_t * generator_copy(generator_t *generator) {
  return (generator_t *) data_copy((data_t *) generator);
}

inline char * generator_tostring(generator_t *generator) {
  return data_tostring((data_t *) generator);
}

inline void generator_free(generator_t *generator) {
  data_free((data_t *) generator);
}

inline int data_is_generator(data_t *data) {
  return data && data_hastype(data, Closure);
}

inline generator_t * data_as_generator(data_t *data) {
  data_is_generator(data) ? ((generator_t *) data) : NULL;
}

/* ------------------------------------------------------------------------ */
