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

#include <libpq-fe.h>

#include "libsql.h"

/* -------------------------------------------------------------------------*/

typedef struct _pgsqlconn {
  dbconn_t  _dbconn;
  PGconn   *conn;
} pgsqlconn_t;

typedef struct _pgsqlstmt {
  data_t       _d;
  pgsqlconn_t *conn;
  PGresult    *result;
  int          nParams;
  Oid         *paramTypes;
  char       **paramValues;
  int         *paramLengths;
  int         *paramFormats;
  int          current;
} pgsqlstmt_t;

static pgsqlconn_t * _pgsqlconn_new(pgsqlconn_t *, va_list);
static void          _pgsqlconn_free(pgsqlconn_t *);
static data_t *      _pgsqlconn_enter(pgsqlconn_t *);
static data_t *      _pgsqlconn_query(pgsqlconn_t *, data_t *);
static data_t *      _pgsqlconn_leave(pgsqlconn_t *, data_t *);

static vtable_t _vtable_PGSQLConnection[] = {
  { .id = FunctionNew,   .fnc = (void_t) _pgsqlconn_new },
  { .id = FunctionFree,  .fnc = (void_t) _pgsqlconn_free },
  { .id = FunctionEnter, .fnc = (void_t) _pgsqlconn_enter },
  { .id = FunctionQuery, .fnc = (void_t) _pgsqlconn_query },
  { .id = FunctionLeave, .fnc = (void_t) _pgsqlconn_leave },
  { .id = FunctionNone,  .fnc = NULL }
};

static data_t * _pgsqlstmt_new(pgsqlstmt_t *, va_list);
static void     _pgsqlstmt_free(pgsqlstmt_t *);
static data_t * _pgsqlstmt_interpolate(pgsqlstmt_t *, array_t *, dict_t *);
static data_t * _pgsqlstmt_has_next(pgsqlstmt_t *);
static data_t * _pgsqlstmt_next(pgsqlstmt_t *);
static data_t * _pgsqlstmt_execute(pgsqlstmt_t *, array_t *, dict_t *);

static vtable_t _vtable_PGSQLStmt[] = {
  { .id = FunctionNew,         .fnc = (void_t) _pgsqlstmt_new },
  { .id = FunctionFree,        .fnc = (void_t) _pgsqlstmt_free },
  { .id = FunctionInterpolate, .fnc = (void_t) _pgsqlstmt_interpolate },
  { .id = FunctionIter,        .fnc = (void_t) data_copy },
  { .id = FunctionHasNext,     .fnc = (void_t) _pgsqlstmt_has_next },
  { .id = FunctionNext,        .fnc = (void_t) _pgsqlstmt_next },
  { .id = FunctionCall,        .fnc = (void_t) _pgsqlstmt_execute },
  { .id = FunctionNone,        .fnc = NULL }
};

static int     PGSQLConnection = -1;
static int     PGSQLStmt = -1;

/* -- S Q L I T E C O N N E C T I O N   D A T A   T Y P E ----------------- */

pgsqlconn_t * _pgsqlconn_new(pgsqlconn_t *c, va_list args) {
  c -> conn = NULL;
  return c;
}

void _pgsqlconn_free(pgsqlconn_t *c) {
  if (c) {
    _pgsqlconn_leave(c, NULL);
  }
}

data_t * _pgsqlconn_enter(pgsqlconn_t *c) {
  data_t *ret = (data_t *) c;

  c -> conn = PQconnectdb(data_tostring((data_t *) c));
  if (PQstatus(c -> conn) == CONNECTION_BAD) {
    ret = data_exception(ErrorSQL, "Error opening PGSQL database");
    c -> _dbconn.status = DBConnException;
    PQfinish(c -> conn);
    c -> conn = NULL;
  } else {
    debug(sql, "Connected to pgsql database '%s'", data_tostring((data_t *) c));
    c -> _dbconn.status = DBConnConnected;
  }
  return ret;
}

data_t * _pgsqlconn_leave(pgsqlconn_t *c, data_t *error) {
  if (c -> conn && (c -> _dbconn.status == DBConnConnected)) {
    PQfinish(c -> conn);
    c -> _dbconn.status = DBConnInitialized;
    c -> conn = NULL;
    debug(sql, "Disconnected from pgsql database '%s'", data_tostring((data_t *) c));
  }
  return error;
}

data_t * _pgsqlconn_query(pgsqlconn_t *c, data_t *query) {
  data_t *ret = NULL;

  if (c -> conn && (c -> _dbconn.status == DBConnConnected)) {
    ret = data_create(PGSQLStmt, c, query);
  }
  return ret;
}

/* -- S Q L I T E S T M T ------------------------------------------------- */

data_t * _pgsqlstmt_new(pgsqlstmt_t *stmt, va_list args) {
  pgsqlconn_t *c = va_arg(args, pgsqlconn_t *);
  data_t      *query = va_arg(args, data_t *);
  data_t      *ret = (data_t *) stmt;

  stmt -> _d.str = strdup(data_tostring(query));
  stmt -> _d.free_str = DontFreeData;
  stmt -> conn = (pgsqlconn_t *) data_copy((data_t *) c);
  stmt -> result = NULL;
  stmt -> nParams = 0;
  stmt -> paramTypes = NULL;
  stmt -> paramValues = NULL;
  stmt -> paramLengths = NULL;
  stmt -> paramFormats = NULL;
  stmt -> current = 0;
  return ret;
}

void _pgsqlstmt_free(pgsqlstmt_t *stmt) {
  if (stmt) {
    free(stmt -> _d.str);
    if (stmt -> result) {
      PQclear(stmt -> result);
    }
    free(stmt -> paramTypes);
    free(stmt -> paramValues);
    free(stmt -> paramLengths);
    free(stmt -> paramFormats);
  }
}

pgsqlstmt_t * _pgsqlstmt_bind_param(pgsqlstmt_t *stmt, int ix, data_t *param) {
  if (data_isnull(param)) {
    stmt -> paramValues[ix] = NULL;
  } else {
    stmt -> paramValues[ix] = strdup(data_tostring(param));
  }
  debug(sql, "_pgsqlstmt_bind_param(%d): %s", ix, stmt -> paramValues[ix] ? stmt -> paramValues[ix] : "(null)");
  return stmt;
}

data_t * _pgsqlstmt_interpolate(pgsqlstmt_t *stmt, array_t *params, dict_t *kwparams) {
  int             ix;
  data_t         *param;
  data_t         *ret = NULL;
  dictiterator_t *di;
  entry_t        *kw;
  char           *pat = NULL;
  char           *repl;
  char           *key;
  int             bufsize = 0;
  str_t *         q;

  debug(sql, "PGSqlStatement interpolate '%s'", stmt -> _d.str);
  stmt -> nParams = (params) ? array_size(params) : 0;
  q = str_copy_chars(stmt -> _d.str);

  /*
   * Count the number of kwparams that actually appear in the query and replace
   * the ${foo} patterns into $n.
   *
   * All this because pgsql doesn't do named paramaters, only numbered.
   */
  if (!ret && kwparams && dict_size(kwparams)) {
    for (di = di_create(kwparams); !ret && di_has_next(di); ) {
      kw = (entry_t *) di_next(di);
      key = (char *) kw -> key;
      if (bufsize < (strlen(key) + 4)) {
        free(pat);
        bufsize = strlen(key) + 4;
        pat = stralloc(bufsize);
        strcpy(pat, "${");
      }
      strcpy(pat + 2, key);
      pat[strlen(key) + 2] = '}';
      pat[strlen(key) + 3] = 0;
      asprintf(&repl, "$%d", stmt -> nParams + 1);
      if (str_replace_all(q, pat, repl) > 0) {
        stmt -> nParams++;
      }
      free(repl);
    }
    di_free(di);
    free(pat);
    free(stmt -> _d.str);
    stmt -> _d.str = strdup(str_chars(q));
    str_free(q);
  }
  if (stmt -> nParams) {
    stmt -> paramValues = (char **) new_ptrarray(stmt -> nParams);
    ix = 0;
    if (params && array_size(params)) {
      for (ix = 0; ix < array_size(params); ix++) {
        param = data_array_get(params, ix);
        _pgsqlstmt_bind_param(stmt, ix, param);
      }
    }
    if (!params || (stmt -> nParams > array_size(params))) {
      for (di = di_create(kwparams); di_has_next(di); ix++) {
        kw = (entry_t *) di_next(di);
        key = (char *) kw -> key;
        param = (data_t *) kw -> value;
        str_replace_all(q, pat, repl);
        _pgsqlstmt_bind_param(stmt, ix, param);
      }
      di_free(di);
    }
  }
  return (data_t *) stmt;
}

data_t * _pgsqlstmt_execute(pgsqlstmt_t *stmt, array_t *params, dict_t *kwparams) {
  data_t         *ret = (data_t *) stmt;
  long            count;
  ExecStatusType  status;

  if (!stmt -> result) {
    if (((params && array_size(params)) || (kwparams && dict_size(kwparams))) &&
        !stmt -> paramValues) {
      _pgsqlstmt_interpolate(stmt, params, kwparams);
    }
    stmt -> result = PQexecParams(stmt -> conn -> conn,
      stmt -> _d.str, stmt -> nParams, stmt -> paramTypes,
      (const char * const *) stmt -> paramValues,
      stmt -> paramLengths, stmt -> paramFormats, 0);
    status = PQresultStatus(stmt -> result);
    if ((status != PGRES_TUPLES_OK) && (status != PGRES_COMMAND_OK)) {
      ret = data_exception(ErrorSQL, "Exception executing query '%s': %s",
        stmt -> _d.str, PQresultErrorMessage(stmt -> result));
      PQclear(stmt -> result);
      stmt -> result = NULL;
    } else {
      stmt -> current = 0;
      debug(sql, "Successful execution of '%s'. Returns %d tuples",
        stmt -> _d.str, PQntuples(stmt -> result));
      if (!strtoint(PQcmdTuples(stmt -> result), &count)) {
        ret = int_to_data(count);
      } else {
        ret = data_true();
      }
    }
  }
  return ret;
}

data_t * _pgsqlstmt_has_next(pgsqlstmt_t *stmt) {
  data_t *ret = NULL;

  if (!stmt -> result) {
    ret = _pgsqlstmt_execute(stmt, NULL, NULL);
  }
  if (!data_is_exception(ret)) {
    data_free(ret);
    ret = (stmt -> current < PQntuples(stmt -> result)) ? data_true() : data_false();
  }
  return ret;
}

data_t *  _pgsqlstmt_next(pgsqlstmt_t *stmt) {
  int     ix;
  int     numcols;
  int     type;
  data_t *rs;
  data_t *data;
  char   *value;

  assert(stmt -> result);
  debug(sql, "Returning row %d", stmt -> current);
  rs = data_create_list(NULL);
  numcols = PQnfields(stmt -> result);
  for (ix = 0; ix < numcols; ix++) {
    if (PQgetisnull(stmt -> result, stmt -> current, ix)) {
      data = data_null();
    } else {
      /* see postgresql/src/include/catalog/pg_type.h */
      value = PQgetvalue(stmt -> result, stmt -> current, ix);
      data = NULL;
      type = 0;
      switch (PQftype(stmt -> result, ix)) {
        case 16: /* bool */
          type = Bool;
          break;
        case 700:  /* float4 */
        case 701:  /* float8 */
        case 1700: /* numeric */
          type = Float;
          break;
        case 20: /* int8 */
        case 21: /* int2 */
        case 23: /* int4 */
        case 26: /* oid */
        case 28: /* xid */
        case 29: /* cid */
          type = Int;
          break;
        case 18: /* char */
        case 19: /* name */
        case 25: /* text */
        default:
          data = (data_t *) str_copy_chars(value);
          break;
      }
      if (!data) {
        assert(type);
        data = data_parse(type, value);
      }
      data_list_push(rs, data);
    }
  }
  stmt -> current++;
  return rs;
}

__DLL_EXPORT__ typedescr_t * postgresql_register(void) {
  typedescr_register_with_name(PGSQLConnection, "postgresql", pgsqlconn_t);
  typedescr_register(PGSQLStmt, pgsqlstmt_t);
  return typedescr_get(PGSQLConnection);
}
