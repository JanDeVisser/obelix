/*
 * /obelix/src/bin/server.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __SERVER_H__
#define __SERVER_H__

#include <config.h>
#include <loader.h>
#include <file.h>

#include "obelix.h"

#ifdef __cplusplus
extern "C" {
#endif
  
#define OBLSERVER_WELCOME             "100 WELCOME"
#define OBLSERVER_READY               "200 READY"
#define OBLSERVER_DATA                "300 DATA"
  
#define OBLSERVER_ERROR_RUNTIME       "501 ERROR RUNTIME"
#define OBLSERVER_ERROR_SYNTAX        "502 ERROR SYNTAX"
#define OBLSERVER_ERROR_PROTOCOL      "503 ERROR PROTOCOL"
#define OBLSERVER_ERROR_INTERNAL      "504 ERROR INTERNAL"
  
#define OBLSERVER_COOKIE              "800 COOKIE"
#define OBLSERVER_BYE                 "900 BYE"

#define OBLSERVER_CODE_WELCOME        100
#define OBLSERVER_CODE_READY          200
#define OBLSERVER_CODE_DATA           300
  
#define OBLSERVER_CODE_ERROR_RUNTIME  501
#define OBLSERVER_CODE_ERROR_SYNTAX   502
#define OBLSERVER_CODE_ERROR_PROTOCOL 503
#define OBLSERVER_CODE_ERROR_INTERNAL 504
  
#define OBLSERVER_CODE_COOKIE         800
#define OBLSERVER_CODE_BYE            900

#define OBLSERVER_CMD_HELLO           "HELLO"  
#define OBLSERVER_CMD_ATTACH          "ATTACH"  
#define OBLSERVER_CMD_PATH            "PATH"
#define OBLSERVER_CMD_RUN             "RUN"
#define OBLSERVER_CMD_EVAL            "EVAL"
#define OBLSERVER_CMD_DETACH          "DETACH"
#define OBLSERVER_CMD_QUIT            "QUIT"

typedef struct _oblserver {
  data_t          _d;
  scriptloader_t *loader;
  obelix_t       *obelix;
  stream_t       *stream;
} oblserver_t;

extern oblserver_t * oblserver_create(obelix_t *, stream_t *);
extern oblserver_t * oblserver_run(oblserver_t *);
extern int           oblserver_start(obelix_t *);

extern int Server;

#define data_is_oblserver(d)  ((d) && data_hastype((d), Server))
#define data_as_oblserver(d)  (data_is_server((d)) ? ((oblserver_t *) (d)) : NULL)
#define oblserver_copy(o)     ((oblserver_t *) data_copy((data_t *) (o)))
#define oblserver_tostring(o) (data_tostring((data_t *) (o))
#define oblserver_free(o)     (data_free((data_t *) (o)))

#ifdef __cplusplus
}
#endif

#endif /* __SERVER_H__ */

