/*
 * /obelix/src/sql/sql.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include "libsql.h"
#include <function.h>

/* -------------------------------------------------------------------------*/

static         void          _sql_init(void);
static         typedescr_t * _dbconn_register(typedescr_t *);
static         int           _dbconn_get_driver(char *);
__PLUGIN__     data_t *      _function_dbconnect(char *, arguments_t *);

static dbconn_t * _dbconn_new(dbconn_t *, va_list);
static void       _dbconn_free(dbconn_t *);
static char *     _dbconn_tostring(dbconn_t *);
static data_t *   _dbconn_resolve(dbconn_t *, char *);

static data_t *   _dbconn_execute(dbconn_t *, char *, arguments_t *);
static data_t *   _dbconn_tx(dbconn_t *, char *, arguments_t *);

_unused_ static vtable_t _vtable_DBConnection[] = {
  { .id = FunctionNew,      .fnc = (void_t) _dbconn_new },
  { .id = FunctionFree,     .fnc = (void_t) _dbconn_free },
  { .id = FunctionToString, .fnc = (void_t) _dbconn_tostring },
  { .id = FunctionResolve,  .fnc = (void_t) _dbconn_resolve },
  { .id = FunctionNone,     .fnc = NULL }
};

_unused_ static methoddescr_t _methods_DBConnection[] = {
  { .type = -1,     .name = "execute", .method = (method_t) _dbconn_execute, .argtypes = { String, Any, Any },         .minargs = 1, .varargs = 1 },
  { .type = -1,     .name = "tx",      .method = (method_t) _dbconn_tx,      .argtypes = { Any, Any, Any },            .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,                       .argtypes = { NoType, NoType, NoType },   .minargs = 0, .varargs = 0 }
};

static tx_t *   _tx_new(tx_t *, va_list);
static void     _tx_free(tx_t *);
static char *   _tx_tostring(tx_t *);
static data_t * _tx_enter(tx_t *);
static data_t * _tx_leave(tx_t *, data_t *);
static data_t * _tx_query(tx_t *, data_t *);

_unused_ static vtable_t _vtable_DBTransaction[] = {
  { .id = FunctionNew,         .fnc = (void_t) _tx_new },
  { .id = FunctionFree,        .fnc = (void_t) _tx_free },
  { .id = FunctionAllocString, .fnc = (void_t) _tx_tostring },
  { .id = FunctionEnter,       .fnc = (void_t) _tx_enter },
  { .id = FunctionLeave,       .fnc = (void_t) _tx_leave },
  { .id = FunctionQuery,       .fnc = (void_t) _tx_query },
  { .id = FunctionNone,        .fnc = NULL }
};

_unused_ int  sql_debug = -1;
int           ErrorSQL = -1;
int           DBConnection = -1;
int           DBTransaction = -1;

static dict_t   *_drivers = NULL;
static mutex_t  *_driver_mutex = NULL;

/* -------------------------------------------------------------------------*/

void _sql_init(void) {
  if (DBConnection < 0) {
    logging_register_module(sql);
    if (!_drivers) {
      _driver_mutex = mutex_create();
      _drivers = dict_create(NULL);
      dict_set_key_type(_drivers, type_str);
      dict_set_data_type(_drivers, type_int);
    }
    if (ErrorSQL < 1) {
      exception_register(ErrorSQL);
    }
    typedescr_register_with_methods(DBConnection, dbconn_t);
    typedescr_register(DBTransaction, tx_t);
  }
  assert(DBConnection);
}

/* -- D B C O N N  B A S E  C L A S S ------------------------------------- */

dbconn_t *_dbconn_new(dbconn_t *conn, va_list args) {
  uri_t *uri = va_arg(args, uri_t *);

  conn -> status = DBConnUninitialized;
  conn -> uri = uri_copy(uri);
  conn -> status = DBConnInitialized;
  return conn;
}

void _dbconn_free(dbconn_t *conn) {
  if (conn) {
    uri_free(conn -> uri);
  }
}

char * _dbconn_tostring(dbconn_t *conn) {
  return uri_tostring(conn -> uri);
}

data_t * _dbconn_resolve(dbconn_t *conn, char *name) {
  if (!strcmp(name, "uri")) {
    return data_copy((data_t *) conn -> uri);
  } else if (!strcmp(name, "status")) {
    return int_to_data(conn -> status);
  } else {
    return NULL;
  }
}

data_t * _dbconn_execute(dbconn_t *conn, _unused_ char *name, arguments_t *args) {
  data_t      *stmt;
  arguments_t *query_args;
  data_t      *query;
  data_t      *ret;

  query_args = arguments_shift(args, &query);
  stmt = data_query((data_t *) conn, query);
  ret = data_call(stmt, query_args);
  arguments_free(query_args);
  return ret;
}

data_t * _dbconn_tx(dbconn_t *conn, _unused_ char *name, _unused_ arguments_t *args) {
  return data_create(DBTransaction, conn);
}

/* -------------------------------------------------------------------------*/

typedescr_t * _dbconn_register(typedescr_t *def) {
  _sql_init();
  mutex_lock(_driver_mutex);
  typedescr_assign_inheritance(typetype(def), DBConnection);
  debug(sql, "Registering SQL driver '%s' (%d)", typename(def), typetype(def));
  dict_put(_drivers, strdup(typename(def)), (void *) ((intptr_t) typetype(def)));
  mutex_unlock(_driver_mutex);
  return def;
}

int _dbconn_get_driver(char *name) {
  function_t  *fnc;
  char        *fncname;
  typedescr_t *td = NULL;
  create_t     regfnc;

  if (dict_has_key(_drivers, name)) {
    return (int) (intptr_t) dict_get(_drivers, name);
  }

  asprintf(&fncname, "%s_register", name);
  debug(sql, "Loading SQL driver '%s'. regfnc '%s'", name, fncname);
  fnc = function_create(fncname, NULL);
  if (fnc -> fnc) {
    regfnc = (create_t) fnc -> fnc;
    td = regfnc();
    debug(sql, "SQL driver '%s' has type %d", name, typetype(td));
    _dbconn_register(td);
  } else {
    error("Registration function '%s' for SQL driver '%s' cannot be resolved",
          fncname, name);
  }
  function_free(fnc);
  free(fncname);
  return typetype(td);
}

/* -------------------------------------------------------------------------*/

data_t * dbconn_create(char *connectstr) {
  uri_t    *uri;
  int       driver = -1;
  data_t   *ret = NULL;

  _sql_init();
  uri = uri_create(connectstr);
  if (uri -> error) {
    ret = data_exception(ErrorParameterValue,
                         "Error parsing Database URI '%s': %s",
                         uri_tostring(uri), data_tostring(uri -> error));
  }
  if (!ret) {
    driver = _dbconn_get_driver(uri -> scheme);
    if (!driver) {
      ret = data_exception(ErrorParameterValue,
                          "Database URI '%s' has unknown type prefix '%s'",
                           uri_tostring(uri), uri -> scheme);
    }
  }
  if (!ret) {
    ret = data_create(driver, uri);
  }
  return ret;
}

/* -------------------------------------------------------------------------*/

__PLUGIN__ data_t * _function_dbconnect(char *func_name, arguments_t *args);

/* -- T X _ T  ------------------------------------------------------------ */

tx_t *_tx_new(tx_t *tx, va_list args) {
  dbconn_t *conn = va_arg(args, dbconn_t *);

  tx -> conn = dbconn_copy(conn);
  return tx;
}

void _tx_free(tx_t *tx) {
  if (tx) {
    dbconn_free(tx -> conn);
  }
}

char * _tx_tostring(tx_t *tx) {
  char *buf;

  asprintf(&buf, "Transaction for '%s'", dbconn_tostring(tx -> conn));
  return buf;
}

data_t * _tx_enter(tx_t *tx) {
  data_t      *ret;
  arguments_t *args;

  args = arguments_create_args(1, str_wrap("BEGIN"));
  ret = data_execute((data_t *) tx -> conn, "execute", args);
  arguments_free(args);
  return (data_is_exception(ret)) ? ret : (data_t *) tx;
}

data_t * _tx_leave(tx_t *tx, data_t *error) {
  data_t  *ret;
  arguments_t *args;

  args = arguments_create_args(1,
      str_wrap((data_is_exception(error)) ? "ROLLBACK" : "COMMIT"));
  ret = data_execute((data_t *) tx -> conn, "execute", args);
  arguments_free(args);
  return (data_is_exception(ret)) ? ret : error;
}

data_t * _tx_query(tx_t *tx, data_t *query) {
  return data_query((data_t *) tx -> conn, query);
}

__PLUGIN__ data_t *_function_dbconnect(char *func_name, arguments_t *args) {
  _sql_init();
  debug(sql, "Connecting to database %s", arguments_arg_tostring(args, 0));
  return (!args || !arguments_args_size(args))
         ? data_exception(ErrorArgCount, "No database URI specified in function '%s'", func_name)
         : dbconn_create(arguments_arg_tostring(args, 0));
}
