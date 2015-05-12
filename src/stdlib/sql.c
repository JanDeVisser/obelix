/*
 * /obelix/src/stdlib/sql.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <stdio.h>
#include <unistd.h>

#include <sqlite3.h>

#include <array.h>
#include <data.h>
#include <dict.h>
#include <exception.h>
#include <name.h>
#include <str.h>
#include <wrapper.h>

/* -------------------------------------------------------------------------*/

typedef data_t (*dbdriver)(data_t *, array_t *, dict_t *);

static void     _sql_init(void) __attribute__((constructor));
extern data_t * _function_dbconnect(char *, array_t *, dict_t *);

extern data_t * sqlite_connect(data_t *, array_t *, dict_t *);
extern data_t * pgsql_connect(data_t *, array_t *, dict_t *);

static dict_t *_drivers;
static int     ErrorSQL;
static int     SQLiteConnection;

typedef struct _sqliteconn {
  data_t    data;
  sqlite3  *conn;
  char     *uri;
} sqliteconn_t;

static vtable_t _wrapper_vtable_sqlite_connection[] = {
  { .id = FunctionCopy,     .fnc = (void_t) sqliteconn_copy },
  { .id = FunctionCmp,      .fnc = (void_t) sqliteconn_cmp },
  { .id = FunctionHash,     .fnc = (void_t) sqliteconn_hash },
  { .id = FunctionFree,     .fnc = (void_t) sqliteconn_free },
  { .id = FunctionToString, .fnc = (void_t) sqliteconn_tostring },
  { .id = FunctionResolve,  .fnc = (void_t) sqliteconn_resolve },
  { .id = FunctionCall,     .fnc = (void_t) sqliteconn_execute },
  { .id = FunctionQuery,    .fnc = (void_t) sqliteconn_query },
  { .id = FunctionLeave,    .fnc = (void_t) sqliteconn_leave },
  { .id = FunctionNone,     .fnc = NULL }
};

/* -------------------------------------------------------------------------*/

void _sql_init(void) {
  _drivers = strvoid_dict_create();
  dict_put(_drivers, "sqlite", sqlite_connect);
  dict_put(_drivers, "pgsql", pgsql_connect);
  ErrorSQL = exception_register("ErrorSQL");
  SQLiteConnection = wrapper_register(-1, "sqliteconnection", _wrapper_vtable_sqlite_connection);
}

/* -------------------------------------------------------------------------*/

data_t * _function_dbconnect(char *func_name, array_t *params, dict_t *kwargs) {
  data_t   *uri;
  char     *uri_str;
  char     *ptr;
  char     *prefix;
  dbdriver  driver;
  data_t   *ret = NULL;

  (void) func_name;
  assert(array_size(params));
  uri = (data_t *) array_get(params, 0);
  assert(uri);
  uri_str = data_charval(uri);
  ptr = strstr(uri_str, "://");
  if (!ptr) {
    ret = data_exception(ErrorParameterValue,
                         "Database URI '%s' has no type prefix",
                         uri_str);
  }
  if (!ret) {
    prefix = (char *) new(ptr - uri_str + 1);
    strncpy(prefix, uri_str, ptr - uri_str);
    driver = (dbdriver) dict_get(_drivers, prefix);
    if (!driver) {
      ret = data_exception(ErrorParameterValue,
                          "Database URI '%s' has unknown type prefix '%s'",
                          uri_str, prefix);
    }
    free(prefix);
  }
  if (!ret) {
    if (params) {
      params = array_slice(params, 1, 0);
    }
    ret = driver(data_create(String, ptr + 1), params, kwargs);
    array_free(params);
  }
  return ret;
}

data_t * pgsql_connect(data_t *uri, array_t *params, dict_t *kwargs) {
  return data_null();
}

data_t * sqlite_connect(data_t *uri, array_t *params, dict_t *kwargs) {
  data_t  *ret;
  sqlite3 *conn;
  int      err;
  
  if (sqlite3_open(data_charval(uri), &conn) != SQLITE_OK) {
    ret = data_exception(ErrorSQL,
                         "Error opening SQLite database: %s",
                         sqlite3_errmsg(conn));
  }
  return data_create(SQLiteConnection, conn);
}

data_t * _data_new_sqlconnection(int type, va_list arg) {
  module_t *module;
  data_t   *ret;
  
  module = va_arg(arg, module_t *);
  ret = &module -> data;
  if (!ret -> ptrval) {
    ret -> ptrval = mod_copy(module);
  }
  return ret;
}


static methoddescr_t _methoddescr_socket[] = {
  { .type = -1,     .name = "close",     .method = _socket_close,     .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "listen",    .method = _socket_listen,    .argtypes = { Callable, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "interrupt", .method = _socket_interrupt, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,        .method = NULL,              .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

#define data_is_socket(d) ((d) && (data_type((d)) == Socket))
#define data_socketval(d) ((socket_wrapper_t *) ((data_is_socket((d)) ? (d) -> ptrval : NULL)))


/* -------------------------------------------------------------------------*/

void _net_init(void) {
  int ix;

  logging_register_category("net", &net_debug);
  Socket = typedescr_register(&_typedescr_socket);
  for (ix = 0; _methoddescr_socket[ix].type != NoType; ix++) {
    if (_methoddescr_socket[ix].type == -1) {
      _methoddescr_socket[ix].type = Socket;
    }
  }
  typedescr_register_methods(_methoddescr_socket);
}

