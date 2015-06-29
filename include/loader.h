/*
 * /obelix/include/loader.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __LOADER_H__
#define __LOADER_H__

#include <core.h>
#include <grammar.h>
#include <name.h>
#include <namespace.h>
#include <parser.h>
#include <script.h>

typedef enum _obelix_option {
  ObelixOptionList,
  ObelixOptionTrace,
  ObelixOptionLAST
} obelix_option_t;

typedef struct _scriptloader {
  data_t       _d;
  data_t      *load_path;
  char        *system_dir;
  grammar_t   *grammar;
  parser_t    *parser;
  namespace_t *ns;
  array_t     *options;
} scriptloader_t;

/*
 * scriptloader_t prototypes
 */

extern scriptloader_t * scriptloader_create(char *, array_t *, char *);
extern scriptloader_t * scriptloader_get(void);
extern scriptloader_t * scriptloader_set_option(scriptloader_t *, obelix_option_t, long);
extern long             scriptloader_get_option(scriptloader_t *, obelix_option_t);
extern data_t *         scriptloader_load_fromreader(scriptloader_t *, module_t *, data_t *);
extern data_t *         scriptloader_load(scriptloader_t *, module_t *);
extern data_t *         scriptloader_import(scriptloader_t *, name_t *);
extern data_t *         scriptloader_run(scriptloader_t *, name_t *, array_t *, dict_t *);

extern int ScriptLoader;

#define data_is_scriptloader(d)  ((d) && data_hastype((d), ScriptLoader))
#define data_as_scriptloader(d)  (data_is_scriptloader((d)) ? ((scriptloader_t *) (d)) : NULL)
#define scriptloader_copy(o)     ((scriptloader_t *) data_copy((data_t *) (o)))
#define scriptloader_tostring(o) (data_tostring((data_t *) (o))
#define scriptloader_free(o)     (data_free((data_t *) (o)))

#endif /* __LOADER_H__ */
