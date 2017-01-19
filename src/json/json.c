/*
 * /obelix/src/lib/json.c - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
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

#include "libjson.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <grammar.h>
#include <nvp.h>
#include <parser.h>

/* ------------------------------------------------------------------------ */

static void        _json_init(void);
extern grammar_t * json_grammar_build(void);

int json_debug = -1;

static grammar_t       *_json_grammar = NULL;
static pthread_once_t   _json_once = PTHREAD_ONCE_INIT;

#define json_init()     pthread_once(&_json_once, _json_init)

/* ------------------------------------------------------------------------ */

void _json_init(void) {
  logging_register_module(json);
  _json_grammar = json_grammar_build();
}

/* ------------------------------------------------------------------------ */

char * json_encode(data_t *value) {
  data_t *serialized;
  char   *ret;

  json_init();
  if (!value) {
    return NULL;
  }
  serialized = data_serialize(value);
  if (data_is_exception(serialized)) {
    return NULL;
  }
  ret = data_encode(serialized);
  data_free(serialized);
  return ret;
}

data_t * json_decode(data_t *jsontext) {
  parser_t *parser;
  data_t   *ret;

  json_init();
  if (!data_hastype(jsontext, InputStream)) {
    return data_exception(ErrorType, "Cannot decode type '%s'",
        data_typename(jsontext));
  }
  parser = parser_create(_json_grammar);
  parser -> data = NULL;
  if (!(ret = parser_parse(parser, jsontext))) {
    ret = parser -> data;
  }
  parser_free(parser);
  return ret;
}

/* ------------------------------------------------------------------------ */

__PLUGIN__ data_t * _function_encode(char *func_name, arguments_t *args) {
  data_t *value;

  if (!args ||
      !arguments_args_size(args) ||
      !(value = data_uncopy(arguments_get_arg(args, 0)))) {
    return data_null();
  }
  return (data_t *) str_adopt(json_encode(value));
}

__PLUGIN__ data_t * _function_decode(char *func_name, arguments_t *args) {
  data_t *encoded;

  if (!args ||
      !arguments_args_size(args) ||
      !(encoded = data_uncopy(arguments_get_arg(args, 0)))) {
    return data_null();
  }
  return json_decode(encoded);
}

/* ------------------------------------------------------------------------ */

__PLUGIN__ parser_t * json_parse_get_value(parser_t *parser) {
  data_t *obj = datastack_pop(parser -> stack);

  parser -> data = data_deserialize(obj);
  data_free(obj);
  return parser;
}

__PLUGIN__ parser_t * json_parse_to_dictionary(parser_t *parser) {
  datalist_t   *list = (datalist_t *) datastack_pop(parser -> stack);
  int           ix;
  nvp_t        *nvp;
  dictionary_t *dict = dictionary_create(NULL);

  for (ix = 0; ix < datalist_size(list); ix++) {
    nvp = (nvp_t *) datalist_get(list, ix);
    dictionary_set(dict, data_tostring(nvp -> name), nvp -> value);
  }
  datalist_free(list);
  datastack_push(parser -> stack, dict);
  return parser;
}
