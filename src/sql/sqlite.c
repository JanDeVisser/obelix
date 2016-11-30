/*
 * /obelix/src/sql/sqlite.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <sqlite3.h>

#include "libsql.h"

/* -------------------------------------------------------------------------*/

typedef struct _sqliteconn {
  dbconn_t  _dbconn;
  sqlite3  *conn;
} sqliteconn_t;

typedef struct _sqlitestmt {
  data_t        _d;
  sqlite3_stmt *stmt;
} sqlitestmt_t;

static sqliteconn_t * _sqliteconn_new(sqliteconn_t *, va_list);
static void           _sqliteconn_free(sqliteconn_t *);
static data_t *       _sqliteconn_enter(sqliteconn_t *);
static data_t *       _sqliteconn_query(sqliteconn_t *, data_t *);
static data_t *       _sqliteconn_leave(sqliteconn_t *, data_t *);

static vtable_t _vtable_SQLiteConnection[] = {
  { .id = FunctionNew,   .fnc = (void_t) _sqliteconn_new },
  { .id = FunctionFree,  .fnc = (void_t) _sqliteconn_free },
  { .id = FunctionEnter, .fnc = (void_t) _sqliteconn_enter },
  { .id = FunctionQuery, .fnc = (void_t) _sqliteconn_query },
  { .id = FunctionLeave, .fnc = (void_t) _sqliteconn_leave },
  { .id = FunctionNone,  .fnc = NULL }
};

static data_t * _sqlitestmt_new(sqlitestmt_t *, va_list);
static void     _sqlitestmt_free(sqlitestmt_t *);
static data_t * _sqlitestmt_interpolate(sqlitestmt_t *, array_t *, dict_t *);
static data_t * _sqlitestmt_has_next(sqlitestmt_t *);
static data_t * _sqlitestmt_next(sqlitestmt_t *);

static vtable_t _vtable_SQLiteStmt[] = {
  { .id = FunctionNew,         .fnc = (void_t) _sqlitestmt_new },
  { .id = FunctionFree,        .fnc = (void_t) _sqlitestmt_free },
  { .id = FunctionInterpolate, .fnc = (void_t) _sqlitestmt_interpolate },
  { .id = FunctionHasNext,     .fnc = (void_t) _sqlitestmt_has_next },
  { .id = FunctionNext,        .fnc = (void_t) _sqlitestmt_next },
  { .id = FunctionNone,        .fnc = NULL }
};

static int     SQLiteConnection = -1;
static int     SQLiteStmt = -1;

/* -- S Q L I T E C O N N E C T I O N   D A T A   T Y P E ----------------- */

sqliteconn_t * _sqliteconn_new(sqliteconn_t *c, va_list args) {
  if (!uri_path(c -> _dbconn.uri)) {
    c -> _dbconn.status = DBConnException;
    c -> _dbconn.uri -> error = data_exception(
      ErrorParameterValue, "No path specified in sqlite URI '%s'",
      uri_tostring(c -> _dbconn.uri));
  }
  c -> conn = NULL;
  return c;
}

void _sqliteconn_free(sqliteconn_t *c) {
  if (c) {
    _sqliteconn_leave(c, NULL);
  }
}

data_t * _sqliteconn_enter(sqliteconn_t *c) {
  data_t *ret = (data_t *) c;
  char   *path = uri_path(c -> _dbconn.uri);

  if (sqlite3_open(path, &c -> conn) != SQLITE_OK) {
    ret = data_exception(
      ErrorSQL, "Error opening SQLite database: %s", sqlite3_errmsg(c -> conn));
    c -> _dbconn.status = DBConnException;
    c -> conn = NULL;
  } else {
    c -> _dbconn.status = DBConnConnected;
  }
  return ret;
}

data_t * _sqliteconn_leave(sqliteconn_t *c, data_t *error) {
  if (c -> conn && (c -> _dbconn.status == DBConnConnected)) {
    sqlite3_close(c -> conn);
    c -> _dbconn.status = DBConnInitialized;
    c -> conn = NULL;
  }
  return error;
}

data_t * _sqliteconn_query(sqliteconn_t *c, data_t *query) {
  data_t *ret = NULL;

  if (c -> conn && (c -> _dbconn.status == DBConnConnected)) {
    ret = data_create(SQLiteStmt, c, data_tostring(query));
  }
  return ret;
}

/* -- S Q L I T E S T M T ------------------------------------------------- */

data_t * _sqlitestmt_new(sqlitestmt_t *stmt, va_list args) {
  sqliteconn_t *c = va_arg(args, sqliteconn_t *);
  data_t       *query = va_arg(args, data_t *);
  data_t       *ret = (data_t *) stmt;

  stmt -> _d.str = strdup(data_tostring(query));
  stmt -> _d.free_str = DontFreeData;
  if (sqlite3_prepare_v2(c -> conn, stmt -> _d.str, -1, &stmt -> stmt, NULL) != SQLITE_OK) {
    ret = data_exception(ErrorSQL, "Could not prepare SQL statement '%s'", stmt -> _d.str);
    stmt -> stmt = NULL;
  }
  return ret;
}

void _sqlitestmt_free(sqlitestmt_t *stmt) {
  if (stmt) {
    sqlite3_finalize(stmt -> stmt);
    free(stmt -> _d.str);
  }
}

data_t * _sqlitestmt_bind_param(sqlitestmt_t *stmt, int ix, data_t *param) {
  char *s;
  int   ret;

  if (data_isnull(param)) {
    ret = sqlite3_bind_null(stmt -> stmt, ix);
  } else if (data_hastype(param, Int)) {
    ret = sqlite3_bind_int(stmt -> stmt, ix, data_intval(param));
  } else if (data_hastype(param, Float)) {
    ret = sqlite3_bind_double(stmt -> stmt, ix, data_floatval(param));
  } else {
    s = data_tostring(param);
    ret = sqlite3_bind_text(stmt -> stmt, ix, s, strlen(s), SQLITE_STATIC);
  }
  return (ret != SQLITE_OK)
    ? data_exception(ErrorSQL,
        "Error binding value '%s' to parameter %d in query '%s'",
        data_tostring(param), ix + 1, data_tostring((data_t *) stmt))
    : NULL;
}

data_t * _sqlitestmt_interpolate(sqlitestmt_t *stmt, array_t *params, dict_t *kwparams) {
  int             ix;
  data_t         *param;
  data_t         *ret = NULL;
  dictiterator_t *di;
  entry_t        *kw;

  if (params && array_size(params)) {
    for (ix = 0; ix < array_size(params); ix++) {
      param = data_array_get(params, ix);
      if ((ret = _sqlitestmt_bind_param(stmt, ix + 1, param))) {
        break;
      }
    }
  }
  if (!ret && kwparams && dict_size(kwparams)) {
    for (di = di_create(kwparams); !ret && di_has_next(di); ) {
      kw = (entry_t *) di_next(di);
      if ((ix = sqlite3_bind_parameter_index(stmt -> stmt, (char *) kw -> key))) {
        if ((ret = _sqlitestmt_bind_param(stmt, ix + 1, (data_t *) kw -> value))) {
          break;
        }
      }
    }
    di_free(di);
  }
  return (ret) ? ret : (data_t *) stmt;
}

data_t * _sqlitestmt_has_next(sqlitestmt_t *stmt) {
  int ret;

  ret = sqlite3_step(stmt -> stmt);
  switch (ret) {
    case SQLITE_ROW:
      return data_true();
    case SQLITE_DONE:
      return data_false();
    default:
      return data_exception(ErrorSQL,
        "Error calling sqlite3_step on prepared statement for query '%s'",
        data_tostring((data_t *) stmt));
  }
}

data_t *  _sqlitestmt_next(sqlitestmt_t *stmt) {
  int     ix;
  int     numcols;
  int     type;
  data_t *rs;

  rs = data_create_list(NULL);
  numcols = sqlite3_column_count(stmt -> stmt);
  for (ix = 0; ix < numcols; ix++) {
    type = sqlite3_column_type(stmt -> stmt, ix);
    switch (type) {
      case SQLITE_INTEGER:
        data_list_push(rs, int_to_data(sqlite3_column_int(stmt -> stmt, ix)));
        break;
      case SQLITE_FLOAT:
        data_list_push(rs, flt_to_data(sqlite3_column_double(stmt -> stmt, ix)));
        break;
      case SQLITE_NULL:
      case SQLITE_BLOB: /* FIXME */
        data_list_push(rs, data_null());
        break;
      case SQLITE_TEXT:
        data_list_push(rs, (data_t *) str_copy_chars(sqlite3_column_text(stmt -> stmt, ix)));
        break;
    }
  }
  return rs;
}

__DLL_EXPORT__ typedescr_t * sqlite_register(void) {
  typedescr_register_with_name(SQLiteConnection, "sqlite", sqliteconn_t);
  typedescr_register(SQLiteStmt, sqlitestmt_t);
  return typedescr_get(SQLiteConnection);
}
