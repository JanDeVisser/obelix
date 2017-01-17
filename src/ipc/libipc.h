/*
 * /obelix/src/parser/libparser.h - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __LIBIPC_H__
#define __LIBIPC_H__

#include <oblconfig.h>
#define OBLIPC_IMPEXP __DLL_EXPORT__

#include <stdio.h>
#include <data.h>
#include <ipc.h>
#include <net.h>

#define OBELIX_DEFAULT_PORT           14400

#define OBLSERVER_CODE_WELCOME        100
#define OBLSERVER_CODE_READY          200
#define OBLSERVER_CODE_DATA           400
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

#define OBLSERVER_CODE_HELLO          300
#define OBLSERVER_CODE_CALL           330
#define OBLSERVER_CODE_QUIT           399

#define OBLSERVER_TAG_HELLO           "HELLO"
#define OBLSERVER_TAG_CALL            "CALL"
#define OBLSERVER_TAG_QUIT            "QUIT"

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

#define OBLSERVER_HELLO               OBLSERVER_MESSAGE(OBLSERVER_CODE_HELLO, OBLSERVER_TAG_HELLO)
#define OBLSERVER_CALL                OBLSERVER_MESSAGE(OBLSERVER_CODE_CALL, OBLSERVER_TAG_CALL)
#define OBLSERVER_QUIT                OBLSERVER_MESSAGE(OBLSERVER_CODE_QUIT, OBLSERVER_TAG_QUIT)

extern void ipc_init(void);

extern int           ipc_debug;
extern code_label_t  message_codes[]

#endif /* __LIBIPC_H__ */
