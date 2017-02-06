/*
 * /obelix/include/application.h - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <data.h>
#include <dictionary.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CMDLINE_OPTION_FLAG_OPTIONAL_ARG  0x0001
#define CMDLINE_OPTION_FLAG_REQUIRED_ARG  0x0002
#define CMDLINE_OPTION_FLAG_MANY_ARG      0x0004
#define CMDLINE_OPTION_FLAG_ALLOWS_ARG    0x0007

typedef struct _cmdline_option {
  char  shortopt;
  char *longopt;
  char *description;
  int   flags;
} cmdline_option_t;

typedef struct _app_description {
  char             *name;
  char             *description;
  char             *legal;
  cmdline_option_t *options;
} app_description_t;

typedef struct _application {
  data_t              _d;
  app_description_t  *descr;
  int                 argc;
  array_t            *argv;
  char               *executable;
  dictionary_t       *options;
  datalist_t         *positional_args;
  data_t             *error;
} application_t;

OBLCORE_IMPEXP application_t * application_create(app_description_t *, int, char **);
OBLCORE_IMPEXP void            application_terminate(void);
OBLCORE_IMPEXP int             Application;

type_skel(application, Application, application_t);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __APPLICATION_H__ */
