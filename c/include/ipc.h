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

#ifndef __IPC_H__
#define __IPC_H__

#include <oblconfig.h>

#include <arguments.h>
#include <data.h>
#include <dictionary.h>
#include <name.h>
#include <net.h>

#define FunctionRegisterServer          FunctionUsr1
#define FunctionUnregisterServer        FunctionUsr2
#define FunctionRemoteCall              FunctionUsr3

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */

typedef struct _mountpoint {
  data_t       _d;
  uri_t       *remote;
  condition_t *wait;
  char        *prefix;
  char        *version;
  int          maxclients;
  int          current;
  list_t      *clients;
} mountpoint_t;

typedef struct _client {
  data_t        _d;
  mountpoint_t *mountpoint;
  stream_t     *socket;
} client_t;

typedef struct _remote {
  data_t        _d;
  mountpoint_t *mountpoint;
  name_t       *name;
} remote_t;

typedef struct _server {
  data_t    _d;
  data_t   *engine;
  stream_t *stream;
  data_t   *mountpoint;
  data_t   *data;
} server_t;

typedef struct _servermessage {
  data_t      _d;
  int         code;
  char       *tag;
  datalist_t *args;
  data_t     *payload;
  char       *encoded;
  int         payload_size;
} servermessage_t;

/* ------------------------------------------------------------------------ */

extern int Mountpoint;

type_skel(mountpoint, Mountpoint, mountpoint_t);

extern data_t *       mountpoint_create(uri_t *, char *);
extern data_t *       mountpoint_checkout_client(mountpoint_t *);
extern mountpoint_t * mountpoint_return_client(mountpoint_t *, client_t *);

/* ------------------------------------------------------------------------ */

extern int Remote;

type_skel(remote, Remote, remote_t);

/* ------------------------------------------------------------------------ */

extern int Client;

type_skel(client, Client, client_t);

extern data_t *      client_create(mountpoint_t *);
extern data_t *      client_run(client_t *, remote_t *, arguments_t *);

/* ------------------------------------------------------------------------ */

extern server_t *    server_create(data_t *, stream_t *);
extern server_t *    server_run(server_t *);
extern int           server_start(data_t *, int);

extern int           Server;

type_skel(server, Server, server_t);

extern int ErrorProtocol;

/* ------------------------------------------------------------------------ */

extern int               ServerMessage;

extern servermessage_t * servermessage_create(int, int, ...);
extern data_t *          servermessage_vmatch(servermessage_t *, int, int , va_list);
extern data_t *          servermessage_match(servermessage_t *, int, int , ...);
extern data_t *          servermessage_match_payload(servermessage_t *, int);
extern servermessage_t * servermessage_push_int(servermessage_t *, int);
extern servermessage_t * servermessage_push(servermessage_t *, char *);
extern servermessage_t * servermessage_set_payload(servermessage_t *, data_t *);

type_skel(servermessage, ServerMessage, servermessage_t);

extern code_label_t  message_codes[];

/* ------------------------------------------------------------------------ */

extern data_t *      protocol_write(stream_t *, char *, int);
extern data_t *      protocol_vprintf(stream_t *, char *, va_list);
extern data_t *      protocol_printf(stream_t *, char *, ...);
extern data_t *      protocol_newline(stream_t *);
extern data_t *      protocol_readline(stream_t *);

extern data_t *      protocol_send_data(stream_t *, int code, data_t *);
extern data_t *      protocol_send_message(stream_t *, servermessage_t *);
extern data_t *      protocol_send_handshake(stream_t *, mountpoint_t *);
extern data_t *      protocol_return_result(stream_t *, data_t *);

extern data_t *      protocol_expect(stream_t *, int, int, ...);
extern data_t *      protocol_read_message(stream_t *);

extern name_t *      protocol_build_name(char *);

#ifdef	__cplusplus
}
#endif

#endif /* __IPC_H__ */
