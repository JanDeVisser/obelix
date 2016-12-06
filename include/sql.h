/*
 * /obelix/inclure/uri.h - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __SQL_H__
#define __SQL_H__

#include <oblconfig.h>
#include <data.h>
#include <net.h>

#ifndef OBLSQL_IMPEXP
  #define OBLSQL_IMPEXP	__DLL_IMPORT__
#endif /* OBLSQL_IMPEXP */

typedef enum _dbconn_status {
  DBConnUninitialized = 0,
  DBConnInitialized,
  DBConnConnected,
  DBConnException
} dbconn_status_t;

typedef struct _dbconn {
  data_t  _d;
  uri_t  *uri;
  int     status;
} dbconn_t;

typedef struct _tx {
  data_t    _d;
  dbconn_t *conn;
} tx_t;

OBLSQL_IMPEXP data_t * dbconn_create(char *);

OBLSQL_IMPEXP int ErrorSQL;
OBLSQL_IMPEXP int DBConnection;
OBLSQL_IMPEXP int DBTransaction;

#define data_is_dbconn(d)  ((d) && data_hastype((d), DBConnection))
#define data_as_dbconn(d)  (data_is_dbconn((d)) ? ((uri_t *) (d)) : NULL)
#define dbconn_copy(d)     ((dbconn_t *) data_copy((data_t *) (d)))
#define dbconn_free(d)     (data_free((data_t *) (d)))
#define dbconn_tostring(d) (data_tostring((data_t *) (d)))

#define data_is_tx(d)      ((d) && data_hastype((d), DBTransaction))
#define data_as_tx(d)      (data_is_tx((d)) ? ((uri_t *) (d)) : NULL)
#define tx_copy(d)         ((tx_t *) data_copy((data_t *) (d)))
#define tx_free(d)         (data_free((data_t *) (d)))
#define tx_tostring(d)     (data_tostring((data_t *) (d)))

#endif /* __SQL_H__ */
