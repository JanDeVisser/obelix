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

int lexa_debug;

typedef struct _lexa {
  dict_t         *scanners;
  lexer_config_t *config;
  char           *fname;
  data_t         *stream;
} lexa_t;

lexa_t * lexa_create(void) {
  lexa_t *ret;

  ret = NEW(lexa_t);
  ret -> scanners = strdata_dict_create();
  ret -> config = NULL;
  ret -> fname = NULL;
  return ret;
}

void lexa_free(lexa_t *lexa) {
  if (lexa) {
    dict_free(lexa -> scanners);
    lexer_config_free(lexa -> config);
    free(lexa -> fname);
    free(lexa);
  }
}

lexa_t * _lexa_build_scanner(entry_t *entry, lexa_t *lexa) {
  scanner_config_t *scanner;

  if (lexa_debug) {
    debug("Building scanner '%s' with config '%s'",
          (char *) entry -> key,
          data_tostring((data_t *) entry -> value));
  }
  scanner = lexer_config_add_scanner(lexa -> config, (char *) entry -> key);
  scanner_config_configure(scanner, (data_t *) entry -> value);
  return lexa;
}

lexa_t * lexa_build_lexer(lexa_t *lexa) {
  lexa -> config = lexer_config_create();
  dict_reduce(lexa -> scanners, (reduce_t) _lexa_build_scanner, lexa);
  return lexa;
}

void * _lexa_tokenize(token_t *token, void *lexer) {
  printf("%s ", token_tostring(token));
  return lexer;
}

lexa_t * lexa_tokenize(lexa_t *lexa) {
  lexer_config_tokenize(lexa -> config, (reduce_t) _lexa_tokenize, lexa -> stream);
}

lexa_t * lexa_add_scanner(lexa_t *lexa, char *code_config) {
  char   *copy;
  char   *ptr;
  data_t *config = data_null();

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

void debug_settings(char *debug) {
  int      debug_all = 0;
  array_t *cats;
  int      ix;
  
  if (debug) {
    debug("debug optarg: %s", debug);
    cats = array_split(debug, ",");
    for (ix = 0; ix < array_size(cats); ix++) {
      logging_enable(str_array_get(cats, ix));
    }
  }
}

int main(int argc, char **argv) {
  lexa_t   *lexa;
  char     *debug = NULL;
  int       opt;

  lexer_init();
  identifier_register();
  whitespace_register();
  logging_register_category("lexa", &lexa_debug);
  lexa = lexa_create();
  while ((opt = getopt(argc, argv, "d:v:s:")) != -1) {
    switch (opt) {
      case 'd':
        debug = optarg;
        break;
      case 's':
        lexa_add_scanner(lexa, optarg);
        break;
      case 'v':
        logging_set_level(atoi(optarg));
        break;
    }
  }
  debug_settings(debug);
  if (optind >= argc) {
    lexa -> fname = strdup("<<stdin>>");
    lexa -> stream = (data_t *) file_create(1);
  } else {
    lexa -> fname = strdup(argv[optind]);
    lexa -> stream = (data_t *) file_open(lexa -> fname);
  }
  lexa_build_lexer(lexa);
  lexa_tokenize(lexa);
  lexa_free(lexa);
  return 0;
}

