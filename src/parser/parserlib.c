/*
 * /obelix/src/parser/parserlib.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
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
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>

#include "libparser.h"
#include <nvp.h>

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

__PLUGIN__ parser_t * parser_log(parser_t *parser, data_t *msg) {
  info("parser_log: %s", data_tostring(msg));
  return parser;
}

__PLUGIN__ parser_t * parser_set_variable(parser_t *parser, data_t *keyval) {
  nvp_t *nvp;
  
  if (data_is_nvp(keyval)) {
    nvp = data_as_nvp(nvp_copy(data_as_nvp(keyval)));
  } else {
    nvp = nvp_parse(data_tostring(keyval));
  }

  if (nvp) {
    parser_set(parser, data_tostring(nvp -> name), data_copy(nvp -> value));
    nvp_free(nvp);
  } else {
    parser = NULL;
  }
  return parser;
}

__PLUGIN__ parser_t * parser_pushval(parser_t *parser, data_t *data) {
  debug(parser, "    Pushing value %s", data_tostring(data));
  datastack_push(parser -> stack, data_copy(data));
  return parser;
}

__PLUGIN__ parser_t * parser_push(parser_t *parser) {
  data_t   *data = token_todata(parser -> last_token);
  parser_t *ret = parser_pushval(parser, data);

  data_free(data);
  return ret;
}

__PLUGIN__ parser_t * parser_push_token(parser_t *parser) {
  return parser_pushval(parser, (data_t *) parser -> last_token);
}

__PLUGIN__ parser_t * parser_push_const(parser_t *parser, data_t *constval) {
  data_t *data = data_decode(data_tostring(constval));

  debug(parser, " -- encoded constant: %s", data_tostring(constval));
  assert(data);
  debug(parser, " -- constant: %s:'%s'", data_typename(data), data_tostring(data));
  return parser_pushval(parser, data_uncopy(data));
}

__PLUGIN__ parser_t * parser_discard(parser_t *parser) {
  data_t   *data = datastack_pop(parser -> stack);

  debug(parser, "    Discarding value %s", data_tostring(data));
  data_free(data);
  return parser;
}

__PLUGIN__ parser_t * parser_dup(parser_t *parser) {
  return parser_pushval(parser, datastack_peek(parser -> stack));
}

__PLUGIN__ parser_t * parser_push_tokenstring(parser_t *parser) {
  data_t   *data = str_to_data(token_token(parser -> last_token));
  parser_t *ret = parser_pushval(parser, data);

  data_free(data);
  return ret;
}

__PLUGIN__ parser_t * parser_bookmark(parser_t *parser) {
  debug(parser, "    Setting bookmark at depth %d", datastack_depth(parser -> stack));
  datastack_bookmark(parser -> stack);
  return parser;
}

__PLUGIN__ parser_t * parser_pop_bookmark(parser_t *parser) {
  array_t *arr;

  debug(parser, "    pop bookmark");
  arr = datastack_rollup(parser -> stack);
  datastack_push(parser -> stack,
    (array_size(arr)) ? data_copy(data_array_get(arr, 0)) : data_null());
  array_free(arr);
  return parser;
}

__PLUGIN__ parser_t * parser_rollup_list(parser_t *parser) {
  array_t    *arr;
  datalist_t *list;

  arr = datastack_rollup(parser -> stack);
  list = datalist_create(arr);
  debug(parser, "    Rolled up list '%s' from bookmark", data_tostring(list));
  datastack_push(parser -> stack, list);
  array_free(arr);
  return parser;
}

__PLUGIN__ parser_t * parser_rollup_name(parser_t *parser) {
  name_t  *name;

  name = datastack_rollup_name(parser -> stack);
  debug(parser, "    Rolled up name '%s' from bookmark", name_tostring(name));
  datastack_push(parser -> stack, (data_t *) name_copy(name));
  name_free(name);
  return parser;
}

__PLUGIN__ parser_t * parser_rollup_nvp(parser_t *parser) {
  data_t *name;
  data_t *value;
  nvp_t  *nvp;

  value = datastack_pop(parser -> stack);
  name = datastack_pop(parser -> stack);
  nvp = nvp_create(data_deserialize(name), data_deserialize(value));
  debug(parser, "    Rolled up nvp '%s'", nvp_tostring(nvp));
  datastack_push(parser -> stack, (data_t *) nvp);
  data_free(name);
  data_free(value);
  return parser;
}

__PLUGIN__ parser_t * parser_new_counter(parser_t *parser) {
  debug(parser, "    Setting new counter");
  datastack_new_counter(parser -> stack);
  return parser;
}

__PLUGIN__ parser_t * parser_incr(parser_t *parser) {
  debug(parser, "    Incrementing counter");
  datastack_increment(parser -> stack);
  return parser;
}

__PLUGIN__ parser_t * parser_count(parser_t *parser) {
  debug(parser, "    Pushing count to stack");
  datastack_push(parser -> stack,
                 int_to_data(datastack_count(parser -> stack)));
  return parser;
}

__PLUGIN__ parser_t * parser_discard_counter(parser_t *parser) {
  debug(parser, "    Discarding counter");
  datastack_count(parser -> stack);
  return parser;
}
