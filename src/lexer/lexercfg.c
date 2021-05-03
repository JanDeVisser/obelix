/*
 * lexercfg.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#include <ctype.h>
#include <stdio.h>

#include "liblexer.h"
#include <exception.h>

/* ------------------------------------------------------------------------ */

extern inline void      _lexer_config_init(void);
static lexer_config_t * _lexer_config_new(lexer_config_t *, va_list);
static void             _lexer_config_free(lexer_config_t *);
static void *           _lexer_config_reduce_children(lexer_config_t *, reduce_t, void *);
static char *           _lexer_config_staticstring(lexer_config_t *);
static data_t *         _lexer_config_resolve(lexer_config_t *, char *);
static data_t *         _lexer_config_set(lexer_config_t *, char *, data_t *);
static data_t *         _lexer_config_mth_add_scanner(lexer_config_t *, char *, arguments_t *);
static data_t *         _lexer_config_mth_tokenize(lexer_config_t *, char *, arguments_t *);

static vtable_t _vtable_LexerConfig[] = {
  { .id = FunctionNew,          .fnc = (void_t) _lexer_config_new },
  { .id = FunctionFree,         .fnc = (void_t) _lexer_config_free },
  { .id = FunctionReduce,       .fnc = (void_t) _lexer_config_reduce_children },
  { .id = FunctionStaticString, .fnc = (void_t) _lexer_config_staticstring },
  { .id = FunctionResolve,      .fnc = (void_t) _lexer_config_resolve },
  { .id = FunctionSet,          .fnc = (void_t) _lexer_config_set },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t _methods_LexerConfig[] = {
  { .type = -1,     .name = "add",      .method = (method_t) _lexer_config_mth_add_scanner, .argtypes = { Any, NoType, NoType },      .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "tokenize", .method = (method_t) _lexer_config_mth_tokenize,    .argtypes = { InputStream, Any, NoType }, .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,       .method = NULL,                .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

int LexerConfig = -1;

/* -- L E X E R  C O N F I G --------------------------------------------- */

void _lexer_config_init(void) {
  if (LexerConfig < 0) {
    typedescr_register_with_methods(LexerConfig, lexer_config_t);
  }
}

lexer_config_t * _lexer_config_new(lexer_config_t *config, va_list args) {
  config -> bufsize = LEXER_BUFSIZE;
  config -> build_func = NULL;
  config -> data = NULL;
  config -> scanners = datalist_create(NULL);
  config -> num_scanners = 0;
  return config;
}


void _lexer_config_free(lexer_config_t *config) {
  if (config) {
    free(config -> build_func);
  }
}

void * _lexer_config_reduce_children(lexer_config_t *config, reduce_t reducer, void *ctx) {
  ctx = reducer(config->scanners, ctx);
  return reducer(config->data, ctx);
}

char * _lexer_config_staticstring(lexer_config_t *config) {
  return "Lexer Configuration";
}

data_t * _lexer_config_resolve(lexer_config_t *config, char *name) {
  int               scanner_ix;
  scanner_config_t *scanner;

  if (!strcmp(name, "buffersize")) {
    return int_to_data(config -> bufsize);
  } else {
    for (scanner_ix = 0; scanner_ix < config->num_scanners; scanner_ix++) {
      scanner = data_as_scanner_config(datalist_get(config->scanners, scanner_ix));
      if (!strcmp(data_typename(scanner), name)) {
        return data_as_data(scanner);
      }
    }
    return NULL;
  }
}

data_t * _lexer_config_set(lexer_config_t *config, char *name, data_t *value) {
  data_t *ret = NULL;

  if (!strcmp(name, "buffersize")) {
    if (!value) {
      lexer_config_set_bufsize(config, LEXER_BUFSIZE);
    } else if (data_is_int(value)) {
      lexer_config_set_bufsize(config, (size_t) data_intval(value));
    } else {
      ret = data_exception(ErrorType,
                           "LexerConfig.buffersize expects 'int', not '%s'",
                           data_typename(value));
    }
  }
  return ret;
}

data_t * _lexer_config_mth_add_scanner(lexer_config_t *config, char *n, arguments_t *args) {
  char *code;

  code = arguments_arg_tostring(args, 0);
  return (data_t *) lexer_config_add_scanner(config, code);

}

data_t * _lexer_config_mth_tokenize(lexer_config_t *config, char *n, arguments_t *args) {
  data_t      *lexer;
  arguments_t *tail;
  data_t      *ret;

  lexer = (data_t *) lexer_create(config, data_uncopy(arguments_get_arg(args, 0)));
  tail = arguments_shift(args, &lexer);
  ret = data_call(lexer, tail);
  data_free(lexer);
  arguments_free(tail);
  return ret;
}

/* ------------------------------------------------------------------------ */

scanner_config_t * _lexer_config_add_scanner(lexer_config_t *config, char *code) {
  scanner_config_t *scanner;
  scanner_config_t *next;
  scanner_config_t *prev;
  int               priority;
  int               ix;

  debug(lexer, "Adding scanner w/ code '%s'", code);
  scanner = scanner_config_create(code, config);
  if (scanner) {
    priority = scanner -> priority;
    for (ix = config->num_scanners - 1;
         ix >= 0 && (data_as_scanner_config(datalist_get(config->scanners, ix))->priority < priority);
         ix--) {
      datalist_set(config->scanners, ix+1, datalist_get(config->scanners, ix));
    }
    datalist_set(config->scanners, ix+1, scanner);
    config -> num_scanners++;
    debug(lexer, "Created scanner config '%s'", scanner_config_tostring(scanner));
  } else {
    debug(lexer, "Could not create scanner with code '%s'", code);
  }
  return scanner;
}

/* ------------------------------------------------------------------------ */

lexer_config_t * lexer_config_create(void) {
  lexer_init();
  return (lexer_config_t *) data_create(LexerConfig);
}

scanner_config_t * lexer_config_add_scanner(lexer_config_t *config, const char *code_config) {
  char             *copy;
  char             *ptr = NULL;
  char             *code;
  char             *param = NULL;
  data_t           *param_data = NULL;
  scanner_config_t *scanner = NULL;

  assert(code_config);
  copy = strdup(code_config);
  ptr = strchr(copy, ':');
  if (ptr) {
    *ptr = 0;
    param = ptr + 1;
    if (!*param) {
      param = NULL;
    } else {
      param = strtrim(param);
    }
  }
  code = strtrim(copy);
  debug(lexer, "lexer_config_add_scanner('%s', '%s')", code, param);
  if (code && *code) {
    if (param) {
      param_data = (data_t *) str_wrap(param);
    }
    lexer_config_set(config, code, param_data);
    data_free(param_data);
    scanner = lexer_config_get_scanner(config, code);
  }
  free(copy);
  return scanner;
}

scanner_config_t * lexer_config_get_scanner(lexer_config_t *config, const char *code) {
  data_t *scanner;
  int     ix;

  for (ix = 0; ix < config->num_scanners; ix++) {
    scanner = datalist_get(config->scanners, ix);
    if (strcmp(data_typename(scanner), code) == 0) {
      return data_as_scanner_config(scanner);
    }
  }
  return NULL;
}

data_t * lexer_config_set(lexer_config_t *config, char *code, data_t *param) {
  scanner_config_t *scanner;

  debug(lexer, "lexer_config_set('%s', '%s:%s')", code, data_typename(param), data_encode(param));
  scanner = lexer_config_get_scanner(config, code);
  if (!scanner) {
    scanner = _lexer_config_add_scanner(config, code);
  }
  return (scanner)
    ? (scanner_config_configure(scanner, param)
        ? (data_t *) config
        : data_exception(ErrorParameterValue,
                        "Could not set parameter '%s' on scanner with code '%s'",
                      data_tostring(param), code))
    : NULL;
}

data_t * lexer_config_get(lexer_config_t *config, char *code, char *name) {
  scanner_config_t *scanner;
  data_t           *ret = NULL;

  debug(lexer, "lexer_config_get('%s', '%s')", code, name);
  scanner = lexer_config_get_scanner(config, code);
  if (scanner) {
    ret = data_get_attribute((data_t *) scanner, name);
    if (data_is_exception_with_code(config, ErrorType)) {
      data_free(ret);
      ret = NULL;
    }
  }
  return ret;
}

size_t lexer_config_get_bufsize(lexer_config_t *config) {
  return config -> bufsize;
}

lexer_config_t * lexer_config_set_bufsize(lexer_config_t *config, size_t bufsize) {
  config -> bufsize = bufsize;
  return config;
}

lexer_config_t * lexer_config_tokenize(lexer_config_t *config, reduce_t tokenizer, data_t *stream) {
  lexer_t *lexer;

  lexer = lexer_create(config, stream);
  lexer_tokenize(lexer, tokenizer, config);
  lexer_free(lexer);
  return config;
}

lexer_config_t * lexer_config_dump(lexer_config_t *config) {
  int               ix;

  printf("lexer_config_t * %s(lexer_config_t *lexer_config) {\n"
         "  scanner_config_t *scanner_config;\n\n"
         "  lexer_config_set_bufsize(lexer_config, %ld);\n",
      (config -> build_func) ? config -> build_func : "lexer_config_build",
      lexer_config_get_bufsize(config));
  for (ix = 0; ix < config->num_scanners; ix++) {
    scanner_config_dump(data_as_scanner_config(datalist_get(config->scanners, ix)));
  }
  printf("  return lexer_config;\n"
         "}\n\n");
  return config;
}
