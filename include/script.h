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

extern int script_debug;
extern int Script;  // data_t typecode

typedef struct _script {
  struct _script *up;
  char           *name;
  char           *fullname;
  dict_t         *variables;
  array_t        *params;
  list_t         *instructions;
  dict_t         *labels;
  char           *label;
  int             refs;
} script_t;

typedef struct _closure {
  script_t    *script;
  dict_t      *variables;
  datastack_t *stack;
  int          refs;
} closure_t;

typedef struct _script_loader {
  char      *path;
  grammar_t *grammar;
  parser_t  *parser;
  script_t  *rootscript;
} scriptloader_t;

extern data_t *         data_create_script(script_t *);

extern script_t *       script_create(script_t *, char *);
extern void             script_free(script_t *);
extern char *           script_get_fullname(script_t *);
extern char *           script_get_modulename(script_t *);
extern char *           script_get_classname(script_t *);
extern script_t *       script_set(script_t *, char *, data_t *);
extern data_t *         script_get(script_t *, char *);
extern void             script_list(script_t *script);

/* Parsing functions */
extern parser_t *       script_parse_init(parser_t *);
extern parser_t *       script_parse_push_last_token(parser_t *);
extern parser_t *       script_parse_param_count(parser_t *);
extern parser_t *       script_parse_push_param(parser_t *);
extern parser_t *       script_parse_emit_assign(parser_t *);
extern parser_t *       script_parse_emit_pushvar(parser_t *);
extern parser_t *       script_parse_emit_pushval(parser_t *);
extern parser_t *       script_parse_emit_mathop(parser_t *);
extern parser_t *       script_parse_emit_func_call(parser_t *);
extern parser_t *       script_parse_emit_import(parser_t *);
extern parser_t *       script_parse_emit_jump(parser_t *);
extern parser_t *       script_parse_emit_pop(parser_t *);
extern parser_t *       script_parse_emit_nop(parser_t *);
extern parser_t *       script_parse_emit_test(parser_t *);
extern parser_t *       script_parse_push_label(parser_t *);
extern parser_t *       script_parse_emit_else(parser_t *);
extern parser_t *       script_parse_emit_end(parser_t *);
extern parser_t *       script_parse_emit_end_while(parser_t *);
extern parser_t *       script_parse_start_function(parser_t *);
extern parser_t *       script_parse_end_function(parser_t *);
extern script_t *       script_push_instruction(script_t *, instruction_t *);

/* Execution functions */
extern closure_t *      script_create_closure(script_t *, array_t *);
extern data_t *         script_execute(script_t *, array_t *);

extern void             closure_free(closure_t *);
extern data_t *         closure_pop(closure_t *);
extern closure_t *      closure_push(closure_t *, data_t *);
extern closure_t *      closure_set(closure_t *, char *, data_t *);
extern data_t *         closure_get(closure_t *, char *);
extern data_t *         closure_resolve(closure_t *, char *);
extern data_t *         closure_execute(closure_t *);

extern scriptloader_t * scriptloader_create(char *, char *);
extern scriptloader_t * scriptloader_get(void);
extern void             scriptloader_free(scriptloader_t *);
extern script_t *       scriptloader_load_fromreader(scriptloader_t *, char *, reader_t *);
extern script_t *       scriptloader_load(scriptloader_t *, char *);
extern data_t *         scriptloader_execute(scriptloader_t *, char *);

#endif /* __SCRIPT_H__ */
