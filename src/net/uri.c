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
static data_t *    _uri_resolve(uri_t *, char *);
static void        _uri_free(uri_t *);

static data_t *    _uri_create(data_t *, char *, array_t *, dict_t *);

extern grammar_t * uri_grammar_build(void);

static vtable_t _vtable_URI[] = {
  { .id = FunctionNew,         .fnc = (void_t) _uri_new },
  { .id = FunctionFree,        .fnc = (void_t) _uri_free },
  { .id = FunctionResolve,     .fnc = (void_t) _uri_resolve },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_URI[] = {
  { .type = Any,    .name = "uri", .method = _uri_create, .argtypes = { String, Any, Any },       .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,  .method = NULL,        .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
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

#define WRAP_IFNOTNULL(d) ((d) ? ((data_t *) str_copy_chars((d))) : data_null())

data_t * _uri_resolve(uri_t *uri, char *name) {
  if (!strcmp(name, "scheme")) {
    return WRAP_IFNOTNULL(uri -> scheme);
  } else if (!strcmp(name, "user")) {
    return WRAP_IFNOTNULL(uri -> user);
  } else if (!strcmp(name, "password")) {
    return WRAP_IFNOTNULL(uri -> password);
  } else if (!strcmp(name, "host")) {
    return WRAP_IFNOTNULL(uri -> host);
  } else if (!strcmp(name, "port")) {
    return int_to_data(uri -> port);
  } else if (!strcmp(name, "path")) {
    return (uri -> path) ? (data_t *) name_copy(uri -> path) : data_null();
  } else if (!strcmp(name, "query")) {
    return (data_t *) dictionary_create_from_dict(uri -> query);
  } else if (!strcmp(name, "fragment")) {
    return WRAP_IFNOTNULL(uri -> fragment);
  } else if (!strcmp(name, "error")) {
    return (uri -> error) ? data_copy(uri -> error) : data_null();
  } else if (!strcmp(name, "ok")) {
    return (data_t *) bool_get(uri -> error == NULL);
  } else {
    return NULL;
  }
}

/* ----------------------------------------------------------------------- */

data_t * _uri_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  uri_t *uri;

  (void) self;
  (void) name;
  (void) kwargs;
  net_init();
  uri = uri_create(data_tostring(data_array_get(args, 0)));
  return (!uri -> error) ? (data_t *) uri : uri -> error;
}

/* ------------------------------------------------------------------------ */

void net_init(void) {
  if (URI < 0) {
    logging_register_category("net", &net_debug);
    dictionary_init();
    typedescr_register_with_methods(URI, uri_t);
    uri_gramar_init();
  }
}

uri_t * uri_create(char *uri) {
  net_init();
  return (uri_t *) data_create(URI, uri);
}

int uri_path_absolute(uri_t *uri) {
  return uri -> path && name_size(uri -> path) && !strcmp(name_first(uri -> path), "/");
}

char * uri_path(uri_t *uri) {
  char *path;

  path = (uri -> path) ? name_tostring_sep(uri -> path, "/") : NULL;
  if (uri_path_absolute(uri)) {
    path++;
  }
  return path;
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

OBLNET_IMPEXP parser_t * uri_parse_set_credentials(parser_t *parser) {
  uri_t *uri = (uri_t *) parser -> data;
  str_t *password = (str_t *) datastack_pop(parser -> stack);
  str_t *user = (str_t *) datastack_pop(parser -> stack);

  uri -> password = str_reassign(password);
  uri -> user = str_reassign(user);
  debug(net, "user: '%s' password: '%s'", uri -> host, uri -> password);
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
