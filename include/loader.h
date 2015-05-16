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
#include <name.h>
#include <namespace.h>
#include <script.h>

typedef enum _obelix_option {
  ObelixOptionList,
  ObelixOptionTrace,
  ObelixOptionLAST
} obelix_option_t;

typedef struct _scriptloader {
  name_t      *load_path;
  char        *system_dir;
  grammar_t   *grammar;
  parser_t    *parser;
  namespace_t *ns;
  array_t     *options;
} scriptloader_t;

/*
 * scriptloader_t prototypes
 */

extern scriptloader_t * scriptloader_create(char *, name_t *, char *);
extern scriptloader_t * scriptloader_get(void);
extern void             scriptloader_free(scriptloader_t *);
extern scriptloader_t * scriptloader_set_option(scriptloader_t *, obelix_option_t, long);
extern data_t *         scriptloader_load_fromreader(scriptloader_t *, module_t *, data_t *);
extern data_t *         scriptloader_load(scriptloader_t *, module_t *);
extern data_t *         scriptloader_import(scriptloader_t *, name_t *);
extern data_t *         scriptloader_run(scriptloader_t *, name_t *, array_t *, dict_t *);

#endif /* __LOADER_H__ */
