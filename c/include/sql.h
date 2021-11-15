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

extern data_t * dbconn_create(char *);

extern int ErrorSQL;
extern int DBConnection;
extern int DBTransaction;

type_skel(dbconn, DBConnection, dbconn_t);
type_skel(tx, DBTransaction, tx_t);

#endif /* __SQL_H__ */
