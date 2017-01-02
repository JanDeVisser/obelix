/*
 * /obelix/src/bin/obelix.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __OBELIX_H__
#define __OBELIX_H__

#include "scriptparse.h"
#include <dict.h>
#include <loader.h>
#include <name.h>
#include <net.h>

#define OBELIX_DEFAULT_PORT           14400

#define OBLSERVER_CODE_WELCOME        100
#define OBLSERVER_CODE_READY          200
#define OBLSERVER_CODE_DATA           300
#define OBLSERVER_CODE_ERROR_RUNTIME  501
#define OBLSERVER_CODE_ERROR_SYNTAX   502
#define OBLSERVER_CODE_ERROR_PROTOCOL 503
#define OBLSERVER_CODE_ERROR_INTERNAL 504
#define OBLSERVER_CODE_COOKIE         800
#define OBLSERVER_CODE_BYE            900

#define OBLSERVER_TAG_WELCOME         "WELCOME"
#define OBLSERVER_TAG_READY           "READY"
#define OBLSERVER_TAG_DATA            "DATA"
#define OBLSERVER_TAG_ERROR_RUNTIME   "ERROR RUNTIME"
#define OBLSERVER_TAG_ERROR_SYNTAX    "ERROR SYNTAX"
#define OBLSERVER_TAG_ERROR_PROTOCOL  "ERROR PROTOCOL"
#define OBLSERVER_TAG_ERROR_INTERNAL  "ERROR INTERNAL"
#define OBLSERVER_TAG_COOKIE          "COOKIE"
#define OBLSERVER_TAG_BYE             "BYE"

#define STRINGIFY(code)               #code
#define OBLSERVER_MESSAGE(code, tag)  STRINGIFY(code) " " tag
#define OBLSERVER_WELCOME             OBLSERVER_MESSAGE(OBLSERVER_CODE_WELCOME, OBLSERVER_TAG_WELCOME)
#define OBLSERVER_READY               OBLSERVER_MESSAGE(OBLSERVER_CODE_READY, OBLSERVER_TAG_READY)
#define OBLSERVER_DATA                OBLSERVER_MESSAGE(OBLSERVER_CODE_DATA, OBLSERVER_TAG_DATA)
#define OBLSERVER_ERROR_RUNTIME       OBLSERVER_MESSAGE(OBLSERVER_CODE_ERROR_RUNTIME, OBLSERVER_TAG_ERROR_RUNTIME)
#define OBLSERVER_ERROR_SYNTAX        OBLSERVER_MESSAGE(OBLSERVER_CODE_ERROR_SYNTAX, OBLSERVER_TAG_ERROR_SYNTAX)
#define OBLSERVER_ERROR_PROTOCOL      OBLSERVER_MESSAGE(OBLSERVER_CODE_ERROR_PROTOCOL, OBLSERVER_TAG_ERROR_PROTOCOL)
#define OBLSERVER_ERROR_INTERNAL      OBLSERVER_MESSAGE(OBLSERVER_CODE_ERROR_INTERNAL, OBLSERVER_TAG_ERROR_INTERNAL)
#define OBLSERVER_COOKIE              OBLSERVER_MESSAGE(OBLSERVER_CODE_COOKIE, OBLSERVER_TAG_COOKIE)
#define OBLSERVER_BYE                 OBLSERVER_MESSAGE(OBLSERVER_CODE_BYE, OBLSERVER_TAG_BYE)

#define OBLSERVER_CMD_HELLO           "HELLO"
#define OBLSERVER_CMD_ATTACH          "ATTACH"
#define OBLSERVER_CMD_PATH            "PATH"
#define OBLSERVER_CMD_RUN             "RUN"
#define OBLSERVER_CMD_EVAL            "EVAL"
#define OBLSERVER_CMD_DETACH          "DETACH"
#define OBLSERVER_CMD_MOUNT           "MOUNT"
#define OBLSERVER_CMD_QUIT            "QUIT"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _obelix {
  data_t        _d;
  int           argc;
  char        **argv;
  name_t       *script;
  array_t      *script_args;
  char         *grammar;
  char         *debug;
  char         *log_level;
  char         *logfile;
  char         *basepath;
  char         *syspath;
  array_t      *options;
  int           server;
  dict_t       *loaders;
  hierarchy_t  *mountpoints;
} obelix_t;

extern name_t *            obelix_build_name(char *);

extern obelix_t *          obelix_initialize(int, char **);
extern scriptloader_t *    obelix_create_loader(obelix_t *);
extern scriptloader_t *    obelix_get_loader(obelix_t *, char *);
extern obelix_t       *    obelix_decommission_loader(obelix_t *, scriptloader_t *);
extern obelix_t *          obelix_set_option(obelix_t *, obelix_option_t, long);
extern long                obelix_get_option(obelix_t *, obelix_option_t);
extern data_t *            obelix_mount(obelix_t *, char *, uri_t *);
extern data_t *            obelix_unmount(obelix_t *, char *);
extern data_t *            obelix_run(obelix_t *, name_t *, array_t *, dict_t *);

extern int Obelix;

#define data_is_obelix(d)  ((d) && data_hastype((d), Obelix))
#define data_as_obelix(d)  (data_is_obelix((d)) ? ((obelix_t *) (d)) : NULL)
#define obelix_copy(o)     ((obelix_t *) data_copy((data_t *) (o)))
#define obelix_tostring(o) (data_tostring((data_t *) (o))
#define obelix_free(o)     (data_free((data_t *) (o)))

/* ------------------------------------------------------------------------ */

typedef struct _oblserver {
  data_t          _d;
  scriptloader_t *loader;
  obelix_t       *obelix;
  stream_t       *stream;
} oblserver_t;

extern oblserver_t *          oblserver_create(obelix_t *, stream_t *);
extern oblserver_t *          oblserver_run(oblserver_t *);
extern int                    oblserver_start(obelix_t *, int);

extern int                    Server;
extern code_label_t           server_codes[];

#define data_is_oblserver(d)  ((d) && data_hastype((d), Server))
#define data_as_oblserver(d)  (data_is_server((d)) ? ((oblserver_t *) (d)) : NULL)
#define oblserver_copy(o)     ((oblserver_t *) data_copy((data_t *) (o)))
#define oblserver_tostring(o) (data_tostring((data_t *) (o))
#define oblserver_free(o)     (data_free((data_t *) (o)))

/* ------------------------------------------------------------------------ */

typedef struct _oblclient oblclient_t;

typedef struct _clientpool {
  data_t       _d;
  condition_t *wait;
  obelix_t    *obelix;
  char        *prefix;
  char        *version;
  uri_t       *server;
  int          maxclients;
  int          current;
  list_t      *clients;
} clientpool_t;

extern clientpool_t *          clientpool_create(obelix_t *, char *, uri_t *);
extern data_t *                clientpool_checkout(clientpool_t *);
extern clientpool_t *          clientpool_return(clientpool_t *, struct _oblclient *);
extern data_t *                clientpool_run(clientpool_t *, name_t *, array_t *, dict_t *);

#define data_is_clientpool(d)  ((d) && data_hastype((d), ClientPool))
#define data_as_clientpool(d)  (data_is_server((d)) ? ((clientpool_t *) (d)) : NULL)
#define clientpool_copy(o)     ((clientpool_t *) data_copy((data_t *) (o)))
#define clientpool_tostring(o) (data_tostring((data_t *) (o))
#define clientpool_free(o)     (data_free((data_t *) (o)))

extern int ClientPool;

/* ------------------------------------------------------------------------ */

typedef struct _oblclient {
  data_t        _d;
  clientpool_t *pool;
  socket_t     *socket;
} oblclient_t;

extern data_t *               oblclient_create(clientpool_t *);
extern data_t *               oblclient_run(oblclient_t *, char *, array_t *, dict_t *);

extern int Client;

#define data_is_oblclient(d)  ((d) && data_hastype((d), Client))
#define data_as_oblclient(d)  (data_is_server((d)) ? ((oblclient_t *) (d)) : NULL)
#define oblclient_copy(o)     ((oblclient_t *) data_copy((data_t *) (o)))
#define oblclient_tostring(o) (data_tostring((data_t *) (o))
#define oblclient_free(o)     (data_free((data_t *) (o)))

extern int ErrorProtocol;
extern int obelix_debug;

#ifdef __cplusplus
}
#endif

#endif /* __OBELIX_H__ */
