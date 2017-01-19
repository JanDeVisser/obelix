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
  sqliteconn_t *conn;
  sqlite3_stmt *stmt;
} sqlitestmt_t;

static sqliteconn_t * _sqliteconn_new(sqliteconn_t *, va_list);
static void           _sqliteconn_free(sqliteconn_t *);
static data_t *       _sqliteconn_enter(sqliteconn_t *);
static data_t *       _sqliteconn_query(sqliteconn_t *, data_t *);
static data_t *       _sqliteconn_leave(sqliteconn_t *, data_t *);

_unused_ static vtable_t _vtable_SQLiteConnection[] = {
  { .id = FunctionNew,   .fnc = (void_t) _sqliteconn_new },
  { .id = FunctionFree,  .fnc = (void_t) _sqliteconn_free },
  { .id = FunctionEnter, .fnc = (void_t) _sqliteconn_enter },
  { .id = FunctionQuery, .fnc = (void_t) _sqliteconn_query },
  { .id = FunctionLeave, .fnc = (void_t) _sqliteconn_leave },
  { .id = FunctionNone,  .fnc = NULL }
};

static data_t *       _sqlitestmt_prepare(sqlitestmt_t *);
static sqlitestmt_t * _sqlitestmt_close(sqlitestmt_t *);
static data_t *       _sqlitestmt_bind_param(sqlitestmt_t *, int, data_t *);

static data_t *       _sqlitestmt_new(sqlitestmt_t *, va_list);
static void           _sqlitestmt_free(sqlitestmt_t *);
static data_t *       _sqlitestmt_interpolate(sqlitestmt_t *, arguments_t *);
static data_t *       _sqlitestmt_has_next(sqlitestmt_t *);
static datalist_t *   _sqlitestmt_next(sqlitestmt_t *);
static data_t *       _sqlitestmt_execute(sqlitestmt_t *, arguments_t *);

_unused_ static vtable_t _vtable_SQLiteStmt[] = {
  { .id = FunctionNew,         .fnc = (void_t) _sqlitestmt_new },
  { .id = FunctionFree,        .fnc = (void_t) _sqlitestmt_free },
  { .id = FunctionInterpolate, .fnc = (void_t) _sqlitestmt_interpolate },
  { .id = FunctionIter,        .fnc = (void_t) data_copy },
  { .id = FunctionHasNext,     .fnc = (void_t) _sqlitestmt_has_next },
  { .id = FunctionNext,        .fnc = (void_t) _sqlitestmt_next },
  { .id = FunctionCall,        .fnc = (void_t) _sqlitestmt_execute },
  { .id = FunctionNone,        .fnc = NULL }
};

static int     SQLiteConnection = -1;
static int     SQLiteStmt = -1;

type_skel(sqliteconn, SQLiteConnection, sqliteconn_t);
type_skel(sqlitestmt, SQLiteStmt, sqlitestmt_t);

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
    ret = data_create(SQLiteStmt, c, query);
  }
  return ret;
}

/* -- S Q L I T E S T M T ------------------------------------------------- */

data_t * _sqlitestmt_prepare(sqlitestmt_t *stmt) {
  data_t *ret = NULL;

  if (!stmt -> stmt) {
    if (sqlite3_prepare_v2(stmt -> conn -> conn, stmt -> _d.str, -1, &stmt -> stmt, NULL) != SQLITE_OK) {
      ret = data_exception(ErrorSQL, "Could not prepare SQL statement '%s'", stmt -> _d.str);
      stmt -> stmt = NULL;
    }
  }
  return ret;
}

sqlitestmt_t * _sqlitestmt_close(sqlitestmt_t *stmt) {
  if (stmt -> stmt) {
    sqlite3_finalize(stmt -> stmt);
    stmt -> stmt = NULL;
  }
  sqliteconn_free(stmt -> conn);
  return stmt;
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
    ret = sqlite3_bind_text(stmt -> stmt, ix, s, (int) strlen(s), SQLITE_STATIC);
  }
  return (ret != SQLITE_OK)
    ? data_exception(ErrorSQL,
        "Error binding value '%s' to parameter %d in query '%s'",
        data_tostring(param), ix + 1, data_tostring((data_t *) stmt))
    : NULL;
}

/* ------------------------------------------------------------------------ */

data_t * _sqlitestmt_new(sqlitestmt_t *stmt, va_list args) {
  sqliteconn_t *c = va_arg(args, sqliteconn_t *);
  data_t       *query = va_arg(args, data_t *);
  data_t       *ret = (data_t *) stmt;

  stmt -> _d.str = strdup(data_tostring(query));
  stmt -> _d.free_str = DontFreeData;
  stmt -> conn = sqliteconn_copy(c);
  stmt -> stmt = NULL;
  return ret;
}

void _sqlitestmt_free(sqlitestmt_t *stmt) {
  if (stmt) {
    _sqlitestmt_close(stmt);
    free(stmt -> _d.str);
  }
}

data_t * _sqlitestmt_interpolate(sqlitestmt_t *stmt, arguments_t *args) {
  int             ix;
  data_t         *param;
  data_t         *ret = NULL;
  dictiterator_t *di;
  entry_t        *kw;

  if ((ret = _sqlitestmt_prepare(stmt))) {
    return ret;
  }
  if (args) {
    if (arguments_args_size(args)) {
      for (ix = 0; ix < arguments_args_size(args); ix++) {
        param = arguments_get_arg(args, ix);
        if ((ret = _sqlitestmt_bind_param(stmt, ix + 1, param))) {
          break;
        }
      }
    }
    if (arguments_kwargs_size(args)) {
      for (di = di_create(args -> kwargs -> attributes); !ret && di_has_next(di);) {
        kw = di_next(di);
        if ((ix = sqlite3_bind_parameter_index(stmt->stmt, (char *) kw->key))) {
          if ((ret = _sqlitestmt_bind_param(stmt, ix + 1, (data_t *) kw->value))) {
            break;
          }
        }
      }
      di_free(di);
    }
  }
  return (ret) ? ret : (data_t *) stmt;
}

data_t * _sqlitestmt_execute(sqlitestmt_t *stmt, arguments_t *args) {
  data_t  *ret = (data_t *) stmt;
  long     count;
  data_t  *row;

  if (args && (arguments_args_size(args) || arguments_kwargs_size(args))) {
    _sqlitestmt_interpolate(stmt, args);
  }
  count = 0;
  for (row = _sqlitestmt_has_next(stmt);
       !data_is_exception(row) && data_intval(row);
       row = _sqlitestmt_has_next(stmt)) {
    count++;
  }
  if (!data_is_exception(row)) {
    debug(sql, "Successful execution of '%s'. Returns %d tuples", stmt -> _d.str, count);
    ret = int_to_data(count);
  }
  return ret;
}

data_t * _sqlitestmt_has_next(sqlitestmt_t *stmt) {
  data_t *ret;
  int     retval;

  if ((ret = _sqlitestmt_prepare(stmt))) {
    return ret;
  }
  retval = sqlite3_step(stmt -> stmt);
  switch (retval) {
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

datalist_t * _sqlitestmt_next(sqlitestmt_t *stmt) {
  int         ix;
  int         numcols;
  int         type;
  datalist_t *rs;

  rs = datalist_create(NULL);
  numcols = sqlite3_column_count(stmt -> stmt);
  for (ix = 0; ix < numcols; ix++) {
    type = sqlite3_column_type(stmt -> stmt, ix);
    switch (type) {
      case SQLITE_INTEGER:
        datalist_push(rs, int_to_data(sqlite3_column_int(stmt -> stmt, ix)));
        break;
      case SQLITE_FLOAT:
        datalist_push(rs, flt_to_data(sqlite3_column_double(stmt -> stmt, ix)));
        break;
      case SQLITE_NULL:
      case SQLITE_BLOB: /* FIXME */
        datalist_push(rs, data_null());
        break;
      case SQLITE_TEXT:
        datalist_push(rs, (data_t *) str_copy_chars((char *) sqlite3_column_text(stmt -> stmt, ix)));
        break;
    }
  }
  return rs;
}

__PLUGIN__ typedescr_t * sqlite_register(void) {
  typedescr_register_with_name(SQLiteConnection, "sqlite", sqliteconn_t);
  typedescr_register(SQLiteStmt, sqlitestmt_t);
  return typedescr_get(SQLiteConnection);
}
