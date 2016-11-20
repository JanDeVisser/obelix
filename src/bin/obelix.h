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
  log_level_t   log_level;
  char         *basepath;
  char         *syspath;
  array_t      *options;
  int           server;
  dict_t       *loaders;
} obelix_t;

extern name_t *            obelix_build_name(char *);

extern obelix_t *          obelix_initialize(int, char **);
extern scriptloader_t *    obelix_create_loader(obelix_t *);
extern scriptloader_t *    obelix_get_loader(obelix_t *, char *);
extern obelix_t       *    obelix_decommission_loader(obelix_t *, scriptloader_t *);
extern obelix_t *          obelix_set_option(obelix_t *, obelix_option_t, long);
extern long                obelix_get_option(obelix_t *, obelix_option_t);
extern data_t *            obelix_run(obelix_t *, name_t *, array_t *, dict_t *);

extern int Obelix;

#define data_is_obelix(d)  ((d) && data_hastype((d), Obelix))
#define data_as_obelix(d)  (data_is_obelix((d)) ? ((obelix_t *) (d)) : NULL)
#define obelix_copy(o)     ((obelix_t *) data_copy((data_t *) (o)))
#define obelix_tostring(o) (data_tostring((data_t *) (o))
#define obelix_free(o)     (data_free((data_t *) (o)))

#ifdef __cplusplus
}
#endif

#endif /* __OBELIX_H__ */

