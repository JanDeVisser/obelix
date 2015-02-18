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

#include <error.h>
#include <namespace.h>
#include <script.h>
#include <scriptparse.h>

static int      _script_pop_operation(parser_t *, char *);
static name_t * _script_pop_and_build_varname(parser_t *);

/* ----------------------------------------------------------------------- */

int _script_pop_operation(parser_t *parser, char *buf) {
  data_t *data;
  int     op;
  
  data = datastack_pop(parser -> stack);
  switch (data_type(data)) {
    case String:
      op = data_charval(data)[0];
      break;
    case Int:
      op = data_intval(data);
      break;
  }
  data_free(data);
  buf[0] = op;
  buf[1] = 0;
  if (parser_debug) {
    debug(" -- operation: %s", buf);
  }
  return op;
}

name_t * _script_pop_and_build_varname(parser_t *parser) {
  data_t *data;
  data_t *count;
  int     ix;
  name_t *ret;
  str_t  *debugstr;

  count = (data_t *) datastack_pop(parser -> stack);
  if (parser_debug) {
    debug("  -- #components: %d", data_intval(count));
  }
  ret = name_create(data_intval(count));
  for (ix = data_intval(count) - 1; ix >= 0; ix--) {
    data = datastack_pop(parser -> stack);
    assert(data_type(data) == String);
    name_extend(ret, data_charval(data));
    data_free(data);
  }
  data_free(count);
  if (parser_debug) {
    debug("  -- varname: %s", name_tostring(ret));
  }
  return ret;
}

/* ----------------------------------------------------------------------- */

parser_t * script_parse_init(parser_t *parser) {
  char        *name;
  namespace_t *ns = NULL;
  data_t      *data;

  if (parser_debug) {
    debug("script_parse_init");
  }
  data = parser_get(parser, "name");
  name = data_charval(data);
  data = parser_get(parser, "ns");
  ns = (data) ? data -> ptrval : NULL;
  assert(ns);
  parser -> data = script_create(ns, NULL, name);
  return parser;
}

parser_t * script_parse_done(parser_t *parser) {
  data_t        *data;
  script_t      *script;
  instruction_t *jump;
  array_t       *error_var;

  if (parser_debug) {
    debug("script_parse_done");
  }
  script = (script_t *) parser -> data;

  data = data_create(Int, 0);
  script_push_instruction(script, instruction_create_pushval(data));
  data_free(data);
  script_push_instruction(script, instruction_create_jump("END"));
  
  script -> label = strdup("ERROR");
  error_var = data_array_create(1);
  array_push(error_var, data_create(String, "$$ERROR"));
  script_push_instruction(script, instruction_create_pushvar(error_var));
  array_free(error_var);
  
  script -> label = strdup("END");
  script_parse_emit_nop(parser);
  if (script_debug) {
    script_list(script);
  }
  return parser;
}

parser_t * script_parse_emit_assign(parser_t *parser) {
  name_t   *varname;
  script_t *script;

  script = parser -> data;
  varname = _script_pop_and_build_varname(parser);
  script_push_instruction(script, instruction_create_assign(varname));
  array_free(varname);
  return parser;
}

parser_t * script_parse_emit_pushvar(parser_t *parser) {
  script_t *script;
  name_t   *varname;

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
    debug(" -- val: %s", data_debugstr(data));
  }
  script_push_instruction(script, instruction_create_pushval(data));
  data_free(data);
  return parser;
}

parser_t *script_parse_push_signed_val(parser_t *parser) {
  data_t   *data;
  data_t   *signed_val;
  char      buf[2];
  script_t *script;
  array_t  *func_name;

  script = parser -> data;
  data = token_todata(parser -> last_token);
  assert(data);
  _script_pop_operation(parser, buf);
  if (parser_debug) {
    debug(" -- val: %s %s", buf, data_debugstr(data));
  }
  signed_val = data_execute(data, buf, NULL, NULL);
  assert(data_type(signed_val) == data_type(data));
  script_push_instruction(script, instruction_create_pushval(signed_val));
  data_free(data);
  return parser;  
}

parser_t *script_parse_emit_unary_op(parser_t *parser) {
  char      buf[2];
  script_t *script;
  array_t  *func_name;

  script = parser -> data;
  _script_pop_operation(parser, buf);
  func_name = data_array_create(1);
  array_push(func_name, data_create(String, buf));
  script_push_instruction(script, instruction_create_function(func_name, 1));
  array_free(func_name);
  return parser;  
}

parser_t * script_parse_emit_infix_op(parser_t *parser) {
  char      buf[2];
  script_t *script;
  array_t  *func_name;

  script = parser -> data;
  _script_pop_operation(parser, buf);
  func_name = data_array_create(1);
  array_push(func_name, data_create(String, buf));
  script_push_instruction(script,
    instruction_create_function(func_name, 2));
  array_free(func_name);
  return parser;
}

parser_t * script_parse_jump(parser_t *parser) {
  script_t *script;
  data_t   *label;

  script = parser -> data;
  label = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- label: %s", data_debugstr(label));
  }
  script_push_instruction(script, instruction_create_jump(data_charval(label)));
  return parser;
}

parser_t * script_parse_emit_func_call(parser_t *parser) {
  script_t *script;
  name_t   *func_name;
  data_t   *param_count;

  script = parser -> data;
  param_count = datastack_pop(parser -> stack);
  func_name = _script_pop_and_build_varname(parser);
  if (parser_debug) {
    debug(" -- param_count: %d", data_intval(param_count));
  }
  script_push_instruction(script,
    instruction_create_function(func_name, data_intval(param_count)));
  data_free(param_count);
  array_free(func_name);
  return parser;
}

parser_t * script_parse_import(parser_t *parser) {
  script_t *script;
  name_t   *module;

  script = parser -> data;
  module = _script_pop_and_build_varname(parser);
  script_push_instruction(script, instruction_create_import(module));
  array_free(module);
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
    debug(" -- label: %s", data_debugstr(label));
  }
  script -> label = strdup(data_charval(label));
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
    debug(" -- label: %s", data_debugstr(label));
  }
  script -> label = strdup(data_charval(label));
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
    debug(" -- end block label: %s", data_debugstr(label));
  }
  block_label = strdup(data_charval(label));
  data_free(label);

  /*
   * Second label: The one pushed after the while statement. This is the one
   * we have to jump back to:
   */
  label = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- jump back label: %s", data_debugstr(label));
  }
  jump = instruction_create_jump(data_charval(label));
  script_push_instruction(script, jump);
  data_free(label);

  script -> label = block_label;

  return parser;
}

parser_t * script_parse_start_function(parser_t *parser) {
  script_t  *up;
  script_t  *func;
  data_t    *data;
  char      *fname;
  int        native;
  array_t   *params;
  int        ix;

  up = (script_t *) parser -> data;

  /* Top of stack: number of parameters and parameters */
  /* Note that we abuse the 'build_varname' function   */
  params = _script_pop_and_build_varname(parser);

  /* Next on stack: function name */
  data = datastack_pop(parser -> stack);
  fname = strdup(data_charval(data));
  data_free(data);

  func = script_create(NULL, up, fname);
  func -> params = str_array_create(array_size(params));
  for (ix = 0; ix < array_size(params); ix++) {
    array_push(func -> params, array_get(params, ix));
  }
  free(fname);
  array_free(params);
  if (parser_debug) {
    debug(" -- defining function %s", func -> name);
  }
  parser -> data = func;
  return parser;
}

parser_t * script_parse_end_function(parser_t *parser) {
  script_t  *func;

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

parser_t * script_parse_end_native_function(parser_t *parser) {
  script_t  *func;
  char      *c_func_name;
  voidptr_t  fnc;

  func = (script_t *) parser -> data;
  c_func_name = strdup(parser -> last_token -> token);

  /* TODO Split c_func_name into lib and func */

  fnc = (voidptr_t) resolve_function(c_func_name);
  if (fnc) {
    func = script_make_native(func, function_create(func -> name, fnc));
    if (parser_debug) {
      debug(" -- defined native function %s", func -> name);
    }
  } else {
    /* FIXME error handling
    return data_error(ErrorName,
                      "Could not find native function '%s'", fname);
     */
    return NULL;
  }
  parser -> data = func -> up;
  return parser;
}


