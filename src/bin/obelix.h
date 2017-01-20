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
#include <grammar.h>
#include <name.h>
#include <net.h>
#include <vm.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COOKIE_SZ       33

typedef enum _obelix_option {
  ObelixOptionList,
  ObelixOptionTrace,
  ObelixOptionLAST
} obelix_option_t;

typedef struct _scriptloader {
  data_t         _d;
  datalist_t    *load_path;
  char          *system_dir;
  grammar_t     *grammar;
  namespace_t   *ns;
  array_t       *options;
  char          *cookie;
  time_t         lastused;
} scriptloader_t;

typedef struct _obelix {
  data_t        _d;
  int           argc;
  char        **argv;
  name_t       *script;
  arguments_t  *script_args;
  char         *grammar;
  char         *debug;
  char         *log_level;
  char         *logfile;
  char         *basepath;
  char         *syspath;
  array_t      *options;
  int           server;
  char         *init_file;
  char         *cookie;
  dictionary_t *loaders;
} obelix_t;

extern name_t *            obelix_build_name(char *);

extern obelix_t *          obelix_initialize(int, char **);
extern scriptloader_t *    obelix_create_loader(obelix_t *);
extern data_t *            obelix_get_loader(obelix_t *, char *);
extern obelix_t       *    obelix_decommission_loader(obelix_t *, char *);
extern obelix_t *          obelix_set_option(obelix_t *, obelix_option_t, long);
extern long                obelix_get_option(obelix_t *, obelix_option_t);
extern data_t *            obelix_run(obelix_t *, name_t *, arguments_t *);

extern int Obelix;

type_skel(obelix, Obelix, obelix_t);

/* ------------------------------------------------------------------------ */

extern scriptloader_t * scriptloader_create(char *, array_t *, char *);
extern scriptloader_t * scriptloader_get(void);
extern scriptloader_t * scriptloader_set_option(scriptloader_t *, obelix_option_t, long);
extern long             scriptloader_get_option(scriptloader_t *, obelix_option_t);
extern scriptloader_t * scriptloader_set_options(scriptloader_t *, array_t *);
extern scriptloader_t * scriptloader_add_loadpath(scriptloader_t *, char *);
extern scriptloader_t * scriptloader_extend_loadpath(scriptloader_t *, array_t *);
extern data_t *         scriptloader_load_fromreader(scriptloader_t *, module_t *, data_t *);
extern data_t *         scriptloader_load(scriptloader_t *, module_t *);
extern data_t *         scriptloader_import(scriptloader_t *, name_t *);
extern data_t *         scriptloader_run(scriptloader_t *, name_t *, arguments_t *);
extern data_t *         scriptloader_eval(scriptloader_t *, data_t *);
extern data_t *         scriptloader_source_initfile(scriptloader_t *);

extern int ScriptLoader;

type_skel(scriptloader, ScriptLoader, scriptloader_t);

/* ------------------------------------------------------------------------ */

extern int obelix_debug;

#ifdef __cplusplus
}
#endif

#endif /* __OBELIX_H__ */
