/*
 * /obelix/src/lib/uri.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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


#include "libnet.h"
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <exception.h>
#include <lexer.h>
#include <grammar.h>
#include <nvp.h>
#include <parser.h>

static void        _uri_grammar_init(void);
static uri_t *     _uri_new(uri_t *, va_list);
static void        _uri_free(uri_t *);

extern grammar_t * uri_grammar_build(void);

static vtable_t _vtable_URI[] = {
  { .id = FunctionNew,         .fnc = (void_t) _uri_new },
  { .id = FunctionFree,        .fnc = (void_t) _uri_free },
  { .id = FunctionNone,        .fnc = NULL }
};

int net_debug = -1;
int URI = -1;

static grammar_t       *_uri_grammar = NULL;
static pthread_once_t   _uri_grammar_once = PTHREAD_ONCE_INIT;

#define uri_gramar_init() pthread_once(&_uri_grammar_once, _uri_grammar_init)

/* ------------------------------------------------------------------------ */

void _uri_grammar_init(void) {
  _uri_grammar = uri_grammar_build();
}

uri_t * _uri_new(uri_t *uri, va_list args) {
  parser_t *parser;
  str_t    *s;

  uri -> _d.str = strdup(va_arg(args, char *));
  uri -> _d.free_str = DontFreeData;
  uri -> error = NULL;
  uri -> scheme = NULL;
  uri -> user = NULL;
  uri -> password = NULL;
  uri -> host = NULL;
  uri -> port = 0;
  name_free(uri -> path);
  uri -> path = NULL;
  uri -> fragment = NULL;
  uri -> query = strstr_dict_create();

  parser = parser_create(_uri_grammar);
  parser -> data = uri;
  s = str_copy_chars(uri -> _d.str);
  if ((uri -> error = parser_parse(parser, (data_t *) s))) {
    uri -> scheme = NULL;
    uri -> user = NULL;
    uri -> password = NULL;
    uri -> host = NULL;
    uri -> port = 0;
    name_free(uri -> path);
    uri -> path = NULL;
    dict_clear(uri -> query);
    uri -> fragment = NULL;
  }
  str_free(s);
  parser_free(parser);
  return uri;
}

void _uri_free(uri_t *uri) {
  if (uri) {
    free(uri -> _d.str);
    data_free(uri -> error);
    free(uri -> scheme);
    free(uri -> user);
    free(uri -> password);
    free(uri -> host);
    name_free(uri -> path);
    dict_free(uri -> query);
    free(uri -> fragment);
  }
}

/* ------------------------------------------------------------------------ */

void net_init(void) {
  if (URI < 0) {
    logging_register_category("net", &net_debug);
    dictionary_init();
    typedescr_register(URI, uri_t);
    uri_gramar_init();
  }
}

uri_t * uri_create(char *uri) {
  net_init();
  return (uri_t *) data_create(URI, uri);
}

/* ------------------------------------------------------------------------ */

OBLNET_IMPEXP parser_t * uri_parse_init(parser_t *parser) {
  debug(net, "parsing starting");
  return parser;
}

OBLNET_IMPEXP parser_t * uri_parse_set_scheme(parser_t *parser) {
  uri_t *uri = (uri_t *) parser -> data;

  uri -> scheme = strdup(token_token(parser -> last_token));
  debug(net, "scheme: '%s'", uri -> scheme);
  return parser;
}

OBLNET_IMPEXP parser_t * uri_parse_set_user(parser_t *parser) {
  uri_t *uri = (uri_t *) parser -> data;
  str_t *user = (str_t *) datastack_pop(parser -> stack);

  uri -> user = str_reassign(user);
  debug(net, "user: '%s'", uri -> user);
  return parser;
}

OBLNET_IMPEXP parser_t * uri_parse_set_password(parser_t *parser) {
  uri_t *uri = (uri_t *) parser -> data;
  str_t *password = (str_t *) datastack_pop(parser -> stack);

  uri -> password = str_reassign(password);
  debug(net, "password: '%s'", uri -> password);
  return parser;
}

OBLNET_IMPEXP parser_t * uri_parse_set_host(parser_t *parser) {
  uri_t *uri = (uri_t *) parser -> data;
  str_t *host = (str_t *) datastack_pop(parser -> stack);

  uri -> host = str_reassign(host);
  debug(net, "host: '%s'", uri -> host);
  return parser;
}

OBLNET_IMPEXP parser_t * uri_parse_set_port(parser_t *parser) {
  uri_t *uri = (uri_t *) parser -> data;
  str_t *portstr = (str_t *) datastack_pop(parser -> stack);
  long   port;

  if (!strtoint(str_chars(portstr), &port)) {
    uri -> port = (int) port;
  } else {
    parser -> error = data_exception(
      ErrorType, "Port must be a number, not '%s'", str_chars(portstr));
  }
  str_free(portstr);
  debug(net, "port: %d", uri -> port);
  return parser;
}

OBLNET_IMPEXP parser_t * uri_parse_set_path(parser_t *parser) {
  uri_t  *uri = (uri_t *) parser -> data;

  uri -> path = (name_t *) datastack_pop(parser -> stack);
  debug(net, "path: '%s'", name_tostring_sep(uri -> path, "/"));
  return parser;
}

OBLNET_IMPEXP parser_t * uri_parse_set_query(parser_t *parser) {
  uri_t   *uri = (uri_t *) parser -> data;
  array_t *query = data_as_array(datastack_pop(parser -> stack));
  int      ix;
  nvp_t   *param;
  char    *n;
  char    *v;

  for (ix = 0; ix < array_size(query); ix++) {
    param = (nvp_t *) array_get(query, ix);
    n = data_tostring(param -> name);
    param -> name -> str = NULL;
    v = data_tostring(param -> value);
    param -> value -> str = NULL;
    dict_put(uri -> query, n, v);
  }
  debug(net, "query:\n%s", dict_tostring(uri -> query));
  return parser;
}

OBLNET_IMPEXP parser_t * uri_parse_set_fragment(parser_t *parser) {
  uri_t *uri = (uri_t *) parser -> data;

  uri -> fragment = strdup(token_token(parser -> last_token));
  debug(net, "fragment: '%s'", uri -> fragment);
  return parser;
}

OBLNET_IMPEXP parser_t * uri_parse_done(parser_t *parser) {
  debug(net, "parsing done");
  return parser;
}
