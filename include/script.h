/*
 * /obelix/include/script.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include <core.h>
#include <data.h>
#include <datastack.h>
#include <dict.h>
#include <instruction.h>
#include <list.h>
#include <parser.h>
#include <object.h>

extern int script_debug;

/* data_t typecodes */
extern int Script;

typedef struct _script {
  struct _script *up;
  char           *name;
  array_t        *params;
  int             native;
  union {
    list_t       *instructions;
    method_t      native_method;
  };
  dict_t         *functions;
  dict_t         *labels;
  char           *label;
  ns_t           *ns;
  int             refs;
} script_t;

typedef struct _closure {
  struct _closure *up;
  script_t        *script;
  object_t        *this;
  dict_t          *variables;
  datastack_t     *stack;
  int              refs;
} closure_t;

typedef struct _scriptloader {
  char        *userpath;
  char        *system_dir;
  grammar_t   *grammar;
  parser_t    *parser;
  ns_t        *ns;
} scriptloader_t;

extern data_t *         data_create_script(script_t *);
extern data_t *         data_create_closure(script_t *);

/*
 * script_t prototypes
 */
extern script_t *       script_create(ns_t *, script_t *, char *);
extern script_t *       script_create_native(script_t *, function_t *);
extern void             script_free(script_t *);
extern char *           script_tostring(script_t *);
extern char *           script_get_name(script_t *);
extern void             script_list(script_t *script);

extern script_t *       script_push_instruction(script_t *, instruction_t *);
extern script_t *       script_create_native(script_t *, function_t *);
extern data_t *         script_create_object(script_t *, array_t *, dict_t *);
extern closure_t *      script_create_closure(script_t *, data_t *, array_t *, dict_t *);
extern data_t *         script_execute(script_t *, data_t *, array_t *, dict_t *);

/*
 * closure_t prototypes
 */
extern void             closure_free(closure_t *);
extern char *           closure_tostring(script_t *);
extern char *           closure_get_name(script_t *);
extern data_t *         closure_pop(closure_t *);
extern closure_t *      closure_push(closure_t *, data_t *);
extern data_t *         closure_set(closure_t *, array_t *, data_t *);
extern data_t *         closure_get(closure_t *, char *);
extern data_t *         closure_resolve(closure_t *, array_t *);
extern data_t *         closure_execute(closure_t *);

#define closure_get_name(c)   closure_tostring((c))

/*
 * scriptloader_t prototypes
 */
extern scriptloader_t * scriptloader_create(char *, char *);
extern scriptloader_t * scriptloader_get(void);
extern void             scriptloader_free(scriptloader_t *);
extern object_t *       scriptloader_load_fromreader(scriptloader_t *, char *, reader_t *);
extern object_t *       scriptloader_load(scriptloader_t *, char *);
extern data_t *         scriptloader_execute(scriptloader_t *, char *);

#endif /* __SCRIPT_H__ */
