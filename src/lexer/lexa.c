/*
 * obelix.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <data.h>
#include <file.h>
#include <lexer.h>
#include <logging.h>
#include <lexa.h>

int lexa_debug;
int Lexa = -1;

static void     _lexa_free(lexa_t *);
static char *   _lexa_staticstring(lexa_t *);
static data_t * _lexa_call(lexa_t *, array_t *, dict_t *);

static vtable_t _vtable_lexa[] = {
  { .id = FunctionFree,         .fnc = (void_t) _lexa_free },
  { .id = FunctionStaticString, .fnc = (void_t) _lexa_staticstring },
  { .id = FunctionCall,         .fnc = (void_t) _lexa_call },
  { .id = FunctionNone,         .fnc = NULL }
};

/* -- L E X A  S T A T I C  F U N C T I O N S ----------------------------- */

void _lexa_free(lexa_t *lexa) {
  if (lexa) {
    dict_free(lexa -> scanners);
    lexer_config_free(lexa -> config);
    free(lexa -> fname);
    free(lexa -> debug);
  }
}

char * _lexa_staticstring(lexa_t *lexa) {
  return "Lexa";
}

data_t * _lexa_call(lexa_t *lexa, array_t *args, dict_t *kwargs) {
  return (data_t *) lexa_tokenize(lexa);
}

/* -- L E X A  P U B L I C  F U N C T I O N S ----------------------------- */

lexa_t * lexa_create(void) {
  lexa_t *ret;

  if (Lexa < 0) {
    lexer_init();
    identifier_register();
    whitespace_register();
    logging_register_category("lexa", &lexa_debug);
    Lexa = typedescr_create_and_register(Lexa, "lexa", _vtable_lexa, NULL);
  }
  ret = data_new(Lexa, lexa_t);
  ret -> debug = NULL;
  ret -> log_level = LogLevelDebug;
  ret -> scanners = strdata_dict_create();
  ret -> config = NULL;
  ret -> fname = NULL;
  return ret;
}

lexa_t * _lexa_build_scanner(entry_t *entry, lexa_t *lexa) {
  scanner_config_t *scanner;

  lexa_debug_settings(lexa);
  mdebug(lexa, "Building scanner '%s' with config '%s'", (char *) entry -> key, data_tostring((data_t *) entry -> value));
  scanner = lexer_config_add_scanner(lexa -> config, (char *) entry -> key);
  mdebug(lexa, "Built scanner '%s'. match: %p", scanner_config_tostring(scanner), scanner -> match);
  scanner_config_configure(scanner, (data_t *) entry -> value);
  return lexa;
}

lexa_t * lexa_build_lexer(lexa_t *lexa) {
  lexa_debug_settings(lexa);
  lexa -> config = lexer_config_create();
  dict_reduce(lexa -> scanners, (reduce_t) _lexa_build_scanner, lexa);
  return lexa;
}

void * _lexa_tokenize(token_t *token, void *config) {
  printf("%s ", token_tostring(token));
  return config;
}

lexa_t * lexa_tokenize(lexa_t *lexa) {
  lexa_debug_settings(lexa);
  if (!lexa -> config) {
    lexa_build_lexer(lexa);
  }
  assert(lexa -> config);
  lexer_config_tokenize(lexa -> config, (reduce_t) _lexa_tokenize, lexa -> stream);
}

lexa_t * lexa_debug_settings(lexa_t *lexa) {
  array_t *cats;
  int      ix;

  if (lexa -> debug) {
    logging_set_level(lexa -> log_level);
    debug("debug optarg: %s", lexa -> debug);
    cats = array_split(lexa -> debug, ",");
    for (ix = 0; ix < array_size(cats); ix++) {
      logging_enable(str_array_get(cats, ix));
    }
    array_free(cats);
    free(lexa -> debug);
    lexa -> debug = NULL;
  }
  return lexa;
}

lexa_t * lexa_add_scanner(lexa_t *lexa, char *code_config) {
  char   *copy;
  char   *ptr;
  data_t *config = data_null();

  lexa_debug_settings(lexa);
  copy = strdup(code_config);
  ptr = strchr(copy, '=');
  if (ptr) {
    *ptr = 0;
    config = (data_t *) str_copy_chars(ptr+1);
  }
  dict_put(lexa -> scanners, strdup(copy), config);
  free(copy);
  return lexa;
}

