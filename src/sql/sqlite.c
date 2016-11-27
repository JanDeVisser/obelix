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

static data_t *   _sqliteconn_new(sqliteconn_t, va_list);
static void       _sqliteconn_free(sqliteconn_t *);
static data_t *   _sqliteconn_query(sqliteconn_t *, data_t *);

static vtable_t _vtable_SQLiteConnection[] = {
  { .id = FunctionNew,      .fnc = (void_t) _sqliteconn_new },
  { .id = FunctionFree,     .fnc = (void_t) _sqliteconn_free },
  { .id = FunctionResolve,  .fnc = (void_t) _sqliteconn_resolve },
  { .id = FunctionCall,     .fnc = (void_t) _sqliteconn_execute },
  { .id = FunctionQuery,    .fnc = (void_t) _sqliteconn_query },
  { .id = FunctionLeave,    .fnc = (void_t) _sqliteconn_leave },
  { .id = FunctionNone,     .fnc = NULL }
};

static int     SQLiteConnection = -1;

/* -- S Q L I T E C O N N E C T I O N   D A T A   T Y P E ----------------- */

data_t * _sqliteconn_new(sqliteconn_t *c, va_list args) {
  uri_t  *uri = va_arg(args, uri_t *);
  data_t *ret = (data_t *) conn;

  if (sqlite3_open(data_tostring(name_tostring_sep(uri -> path, "/")), &c -> conn) != SQLITE_OK) {
    ret = data_exception(ErrorSQL,
                         "Error opening SQLite database: %s",
                         sqlite3_errmsg(c -> conn));
  }
  return ret;
}

void _sqliteconn_free(sqliteconn_t *c) {
  if (c) {
    sqlite3_close(c -> conn);
  }
}

data_t * _sqliteconn_query(sqliteconn_t *conn, data_t *query) {
}

__DLL_EXPORT__ typedescr_t * sqlite_register(void) {
  typedescr_register_with_name(SQLiteConnection, "sqlite", sqliteconn_t);
  return typedescr_get(SQLiteConnection);
}
