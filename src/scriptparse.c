/*
 * /obelix/src/scriptparse.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <script.h>

parser_t * script_parse_init(parser_t *parser) {
  char     *name;
  script_t *up;
  object_t *ns;
  data_t   *data;

  if (parser_debug) {
    debug("script_parse_init");
  }
  data = parser_get(parser, "name");
  name = (char *) data -> ptrval;
  data = parser_get(parser, "up");
  up = (script_t *) data -> ptrval;
  data = parser_get(parser, "ns");
  ns = (object_t *) data -> ptrval;
  parser -> data = script_create(ns, up, name);
  return parser;
}

parser_t * script_parse_done(parser_t *parser) {
  script_t      *script;
  instruction_t *jump;

  if (parser_debug) {
    debug("script_parse_done");
  }
  script = (script_t *) parser -> data;
  if (script -> label) {
    script_parse_emit_nop(parser);
  }
  jump = instruction_create_jump("END");
  script_push_instruction(script, jump);
  script -> label = strdup("ERROR");
  /*
   * TODO Error handling code
   */
  script_parse_emit_nop(parser);
  script -> label = strdup("END");
  script_parse_emit_nop(parser);
  if (script_debug) {
    script_list(script);
  }
  return parser;
}

array_t * _script_pop_and_build_varname(parser_t *parser) {
  data_t         *data;
  data_t         *count;
  int             ix;
  array_t        *ret;
  str_t          *debugstr;

  count = (data_t *) dict_pop(parser -> variables, "count");
  if (parser_debug) {
    debug("  -- components: %d", count -> intval);
  }
  ret = str_array_create(count -> intval);
  for (ix = count -> intval - 1; ix >= 0; ix--) {
    data = datastack_pop(parser -> stack);
    array_set(ret, ix, strdup((char *) data -> ptrval));
    data_free(data);
  }
  data_free(count);
  if (parser_debug) {
    array_debug("  -- varname: %s", ret);
  }
  return ret;
}

parser_t * script_parse_emit_assign(parser_t *parser) {
  array_t  *varname;
  script_t *script;

  script = parser -> data;
  varname = _script_pop_and_build_varname(parser);
  script_push_instruction(script, instruction_create_assign(varname));
  array_free(varname);
  return parser;
}

parser_t * script_parse_emit_pushvar(parser_t *parser) {
  script_t *script;
  array_t  *varname;

  script = parser -> data;
  varname = _script_pop_and_build_varname(parser);
  script_push_instruction(script, instruction_create_pushvar(varname));
  array_free(varname);
  return parser;
}

parser_t * script_parse_emit_pushval(parser_t *parser) {
  script_t *script;
  data_t   *data;

  script = parser -> data;
  data = token_todata(parser -> last_token);
  assert(data);
  if (parser_debug) {
    debug(" -- val: %s", data_tostring(data));
  }
  script_push_instruction(script, instruction_create_pushval(data));
  return parser;
}

parser_t * script_parse_emit_mathop(parser_t *parser) {
  char     *op;
  data_t   *data;
  script_t *script;

  script = parser -> data;
  data = datastack_pop(parser -> stack);
  op = (char *) data -> ptrval;
  if (parser_debug) {
    debug(" -- op: %s", op);
  }
  script_push_instruction(script,
    instruction_create_function(op, 2));
  data_free(data);
  return parser;
}

parser_t * script_parse_emit_func_call(parser_t *parser) {
  script_t *script;
  array_t  *func_name;
  data_t   *param_count;

  script = parser -> data;
  param_count = (data_t *) dict_pop(parser -> variables, "param_count");
  func_name = _script_pop_and_build_varname(parser);
  if (parser_debug) {
    debug(" -- param_count: %d", param_count -> intval);
    debug(" -- func_name: %s", func_name);
  }
  script_push_instruction(script,
    instruction_create_function(func_name, param_count -> intval));
  data_free(param_count);
  array_free(func_name);
  return parser;
}

parser_t * script_parse_emit_new_list(parser_t *parser) {
  script_t *script;
  data_t   *count;

  script = parser -> data;
  count = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- #entries: %d", count -> intval);
  }
  script_push_instruction(script,
    instruction_create_function("list", count -> intval));
  data_free(count);
  return parser;
}

parser_t * script_parse_import(parser_t *parser) {
  script_t *script;
  array_t  *module;

  script = parser -> data;
  module = _script_pop_and_build_varname(parser);
  if (parser_debug) {
    debug(" -- module: %s", module);
  }
  script_push_instruction(script,
    instruction_create_import(module));
  free(module);
  return parser;
}

parser_t * script_parse_emit_test(parser_t *parser) {
  script_t      *script;
  instruction_t *test;
  char           label[9];

  script = parser -> data;
  strrand(label, 8);
  test = instruction_create_test(label);
  datastack_push_string(parser -> stack, label);
  script_push_instruction(script, test);
  return parser;
}

parser_t * script_parse_emit_jump(parser_t *parser) {
  script_t      *script;
  instruction_t *jump;
  char           label[9];

  script = parser -> data;
  strrand(label, 8);
  jump = instruction_create_jump(label);
  datastack_push_string(parser -> stack, jump -> name);
  script_push_instruction(script, jump);
  return parser;
}

parser_t * script_parse_emit_pop(parser_t *parser) {
  script_t      *script;

  script = parser -> data;
  script_push_instruction(script, instruction_create_pop());
  return parser;
}

parser_t * script_parse_emit_nop(parser_t *parser) {
  script_t      *script;

  script = parser -> data;
  script_push_instruction(script, instruction_create_nop());
  return parser;
}

parser_t * script_parse_push_label(parser_t *parser) {
  script_t      *script;

  script = parser -> data;
  script -> label = new(9);
  strrand(script -> label, 8);
  datastack_push_string(parser -> stack, script -> label);
  return parser;
}

parser_t * script_parse_emit_else(parser_t *parser) {
  script_t      *script;
  instruction_t *jump;
  data_t        *label;
  char           newlabel[9];

  script = parser -> data;
  label = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- label: %s", (char *) label -> ptrval);
  }
  script -> label = strdup(label -> ptrval);
  data_free(label);
  strrand(newlabel, 8);
  jump = instruction_create_jump(newlabel);
  datastack_push_string(parser -> stack, newlabel);
  script_push_instruction(script, jump);
  return parser;
}

parser_t * script_parse_emit_end(parser_t *parser) {
  script_t      *script;
  data_t        *label;

  script = parser -> data;
  label = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- label: %s", (char *) label -> ptrval);
  }
  script -> label = strdup(label -> ptrval);
  data_free(label);
  return parser;
}

parser_t * script_parse_emit_end_while(parser_t *parser) {
  script_t      *script;
  data_t        *label;
  instruction_t *jump;
  char          *block_label;

  script = parser -> data;

  /*
   * First label: The one pushed at the end of the expression. This is the
   * label to be set at the end of the loop:
   */
  label = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- end block label: %s", (char *) label -> ptrval);
  }
  block_label = strdup(label -> ptrval);
  data_free(label);

  /*
   * Second label: The one pushed after the while statement. This is the one
   * we have to jump back to:
   */
  label = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- jump back label: %s", (char *) label -> ptrval);
  }
  jump = instruction_create_jump((char *) label -> ptrval);
  script_push_instruction(script, jump);
  data_free(label);

  script -> label = block_label;

  return parser;
}

parser_t * script_parse_start_function(parser_t *parser) {
  script_t *up;
  script_t *func;
  long      count;
  data_t   *data;
  int       ix;
  array_t  *params;
  char     *name;

  up = (script_t *) parser -> data;

  data = datastack_pop(parser -> stack);
  count = data -> intval;
  data_free(data);
  params = str_array_create(count);
  for (ix = count - 1; ix >= 0; ix--) {
    data = datastack_pop(parser -> stack);
    array_set(params, ix, strdup(data -> ptrval));
    data_free(data);
  }
  data = datastack_pop(parser -> stack);
  func = script_create(up, (char *) data -> ptrval);
  func -> params = params;
  data_free(data);
  if (parser_debug) {
    debug(" -- definining function %s, #params %d", func -> name, count);
  }
  parser -> data = func;
  return parser;
}

parser_t * script_parse_end_function(parser_t *parser) {
  script_t *func;

  func = (script_t *) parser -> data;
  if (func -> label) {
    script_parse_emit_nop(parser);
  }
  if (script_debug) {
    script_list(func);
  }
  parser -> data = func -> up;
  return parser;
}



