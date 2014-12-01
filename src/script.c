/*
 * /obelix/src/script.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <math.h>
#include <stdio.h>

#include <data.h>
#include <script.h>
#include <str.h>

int script_debug = 0;

/*
 * S T A T I C  F U N C T I O N S
 */

static data_t *         _data_new_script(data_t *, va_list);
static int              _data_cmp_script(data_t *, data_t *);
static char *           _data_tostring_script(data_t *);
static void             _data_script_register(void);

static data_t *         _plus(data_t *, data_t *);
static data_t *         _minus(data_t *, data_t *);
static data_t *         _times(data_t *, data_t *);
static data_t *         _div(data_t *, data_t *);
static data_t *         _modulo(data_t *, data_t *);
static data_t *         _pow(data_t *, data_t *);
static data_t *         _gt(data_t *, data_t *);
static data_t *         _geq(data_t *, data_t *);
static data_t *         _lt(data_t *, data_t *);
static data_t *         _leq(data_t *, data_t *);
static data_t *         _eq(data_t *, data_t *);
static data_t *         _neq(data_t *, data_t *);

static data_t *         _script_function_print(script_ctx_t *, char *, array_t *);
static data_t *         _script_function_mathop(script_ctx_t *, char *, array_t *);
static data_t *         _script_function_script(script_ctx_t *, char *, array_t *);

static listnode_t *     _script_execute_instruction(instruction_t *, script_t *);
static void             _script_list_visitor(instruction_t *);

static function_def_t _builtins[] = {
    { name: "print",      fnc: _script_function_print,  min_params: 1, max_params: -1 },
    { name: "$$script$$", fnc: _script_function_script, min_params: 0, max_params: -1 },
    { name: "+",          fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: "-",          fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: "*",          fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: "/",          fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: "%",          fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: "^",          fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: ">=",         fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: ">",          fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: "<=",         fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: "<",          fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: "==",         fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: "!=",         fnc: _script_function_mathop, min_params: 2, max_params:  2 },
    { name: NULL,         fnc: NULL,                    min_params: 0, max_params:  0 }
};

/*
 * data_t Script type 
 */

int Script = 0;

static typedescr_t typedescr_script = {
  type:                  -1,
  new:      (new_t)      _data_new_script,
  copy:                  NULL,
  cmp:      (cmp_t)      _data_cmp_script,
  free:     (free_t)     script_free,
  tostring: (tostring_t) _data_tostring_script,
  parse:                 NULL,
};

data_t * _data_new_script(data_t *ret, va_list arg) {
  script_t *script;

  script = va_arg(arg, script_t *);
  debug("data_create_script(%s)", script -> name);
  ret -> ptrval = script;
  return ret;
}

int _data_cmp_script(data_t *d1, data_t *d2) {
  script_t *s1;
  script_t *s2;

  _data_script_register();
  s1 = d1 -> ptrval;
  s2 = d2 -> ptrval;
  return (s1 -> up == s2 -> up) 
    ? strcmp(s1 -> name, s2 -> name)
    : s1 -> up - s2 -> up;
}

char * _data_tostring_script(data_t *d) {
  static char buf[32];

  _data_script_register();
  snprintf(buf, 32, "<<script %s>>", ((script_t *) d) -> name);
  return buf;
}

void _data_script_register(void) {
  if (!Script) {
    Script = datatype_register(&typedescr_script);
  }
}

data_t * data_create_script(script_t *script) {
  data_t *ret;

  _data_script_register();
  debug("data_create_script(%s)", script -> name);
  return data_create(Script, script);
}

/*
 * script_t static functions
 */

data_t * _plus(data_t *d1, data_t *d2) {
  str_t  *s;
  data_t *ret;
  int     type;

  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    type = ((data_type(d1) == data_type(d2)) && (data_type(d1) == Int))
      ? Int : Float;
    ret = data_create(type, 
		((data_type(d1) == Int) ? d1 -> intval : d1 -> dblval) + 
		((data_type(d2) == Int) ? d2 -> intval : d2 -> dblval));
  } else {
    s = str_copy_chars(data_tostring(d1));
    str_append_chars(s, data_tostring(d2));
    ret = data_create_string(str_chars(s));
    str_free(s);
  }
  return ret;
}

data_t * _minus(data_t *d1, data_t *d2) {
  data_t *ret;
  int     type;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    type = ((data_type(d1) == data_type(d2)) && (data_type(d1) == Int))
      ? Int : Float;
    ret = data_create(type, 
		((data_type(d1) == Int) ? d1 -> intval : d1 -> dblval) + 
		((data_type(d2) == Int) ? d2 -> intval : d2 -> dblval));
  }
  return ret;
}

data_t * _times(data_t *d1, data_t *d2) {
  data_t *ret;
  int     type;
  str_t  *s;
  int     ix;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    type = ((data_type(d1) == data_type(d2)) && (data_type(d1) == Int))
      ? Int : Float;
    ret = data_create(type, 
		((data_type(d1) == Int) ? d1 -> intval : d1 -> dblval) + 
		((data_type(d2) == Int) ? d2 -> intval : d2 -> dblval));
  } else if ((data_type(d1) == String) && (data_type(d2) == Int)) {
    s = str_create(0);
    for (ix = 0; ix < d2 -> intval; ix++) {
      str_append_chars(s, data_tostring(d1));
    }
    ret = data_create_string(str_chars(s));
    str_free(s);    
  }
  return ret;
}

data_t * _div(data_t *d1, data_t *d2) {
  data_t *ret;
  int     type;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    type = ((data_type(d1) == data_type(d2)) && (data_type(d1) == Int))
      ? Int : Float;
    ret = data_create(type, 
		((data_type(d1) == Int) ? d1 -> intval : d1 -> dblval) /
		((data_type(d2) == Int) ? d2 -> intval : d2 -> dblval));
  }
  return ret;
}

data_t * _modulo(data_t *d1, data_t *d2) {
  data_t *ret;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    ret = data_create_int(
      (long) ((data_type(d1) == Int) ? d1 -> intval : round(d1 -> dblval)) %
      (long) ((data_type(d2) == Int) ? d2 -> intval : round(d2 -> dblval)));
  }
  return ret;
}

data_t * _pow(data_t *d1, data_t *d2) {
  data_t *ret;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    ret = data_create_float(pow(
      ((data_type(d1) == Int) ? (double) d1 -> intval : d1 -> dblval),
      ((data_type(d2) == Int) ? (double) d2 -> intval : d2 -> dblval)));
  }
  return ret;
}

data_t * _gt(data_t *d1, data_t *d2) {
  return data_create_bool(data_cmp(d1, d2) > 0);
}

data_t * _geq(data_t *d1, data_t *d2) {
  return data_create_bool(data_cmp(d1, d2) >= 0);
}

data_t * _lt(data_t *d1, data_t *d2) {
  return data_create_bool(data_cmp(d1, d2) < 0);
}

data_t * _leq(data_t *d1, data_t *d2) {
  return data_create_bool(data_cmp(d1, d2) <= 0);
}

data_t * _eq(data_t *d1, data_t *d2) {
  return data_create_bool(!data_cmp(d1, d2));
}

data_t * _neq(data_t *d1, data_t *d2) {
  return data_create_bool(data_cmp(d1, d2));
}

typedef struct _mathop_def {
  char     *token;
  mathop_t  fnc;
} mathop_def_t;

static mathop_def_t mathops[] = {
  { token: "+",   fnc: _plus },
  { token: "-",   fnc: _minus },
  { token: "*",   fnc: _times },
  { token: "/",   fnc: _div },
  { token: "%",   fnc: _modulo },
  { token: "^",   fnc: _pow },
  { token: ">",   fnc: _gt },
  { token: ">=",  fnc: _geq },
  { token: "<",   fnc: _lt },
  { token: "<=",  fnc: _leq },
  { token: "==",  fnc: _lt },
  { token: "!=",  fnc: _leq },
  { token: NULL,  fnc: NULL }
};

data_t * _script_function_mathop(script_ctx_t *script, char *op, array_t *params) {
  char     *varname;
  data_t   *d1;
  data_t   *d2;
  data_t   *ret;
  long      l1, l2, result;
  int       ix;
  mathop_t  fnc;

  assert(array_size(params) == 2);
  d1 = array_get(params, 0);
  assert(d1);
  d2 = array_get(params, 1);
  assert(d2);
  fnc = NULL;
  for (ix = 0; mathops[ix].token; ix++) {
    if (!strcmp(mathops[ix].token, op)) {
      fnc = mathops[ix].fnc;
      break;
    }
  }
  assert(fnc);
  return fnc(d1, d2);
}

data_t * _script_function_print(script_ctx_t *script, char *name, array_t *params) {
  char          *varname;
  data_t        *value;
  char          *fmt;
  char          *ptr;
  char           buf[1000], tmp[1000];

  assert(array_size(params));
  value = (data_t *) array_get(params, 0);
  assert(value);

  /*
   * Ideally we want to do a printf-style function. The idea is to have the
   * first argument as the format string, and following parameters as values.
   * Since there is no API for sprintf that takes a user-accessible data
   * structure, just the va_list one, we will have to go through the format
   * string and do it one by one.
   *
   * But that's future. Now we just print the first parameter :-)
   */
  info(data_tostring(value));
  return data_create_int(1);
}

data_t * _script_function_script(script_ctx_t *script, char *name, array_t *params) {
  data_t   *value;
  int       ix;
  script_t *func;

  func = (script_t *) script;
  assert(array_size(func -> params) == array_size(params));
  for (ix = 0; ix < array_size(params); ix++) {
    script_set(func, array_get(func -> params, ix),
                     array_get(params, ix));
  }
  value = script_execute(func);
  value = value ? value : data_create_int(0);
  if (script_debug) {
    debug(" -- return: '%s'", data_tostring(value));
  }
  return value;
}

/*
 * script_t public functions
 */

script_t * script_create(script_t *up) {
  script_t       *ret;
  function_def_t *def;

  ret = NEW(script_t);
  ret -> variables = strvoid_dict_create();
  dict_set_free_data(ret -> variables, (free_t) data_free);
  dict_set_tostring_data(ret -> variables, (tostring_t) data_tostring);
  ret -> stack = list_create();
  list_set_free(ret -> stack, (free_t) data_free);
  list_set_tostring(ret -> stack, (tostring_t) data_tostring);
  ret -> instructions = list_create();
  list_set_free(ret -> instructions, (free_t) instruction_free);
  list_set_tostring(ret -> instructions, (tostring_t) instruction_tostring);

  ret -> labels = strvoid_dict_create();
  ret -> label = NULL;
  ret -> params = NULL;
  ret -> name = NULL;

  ret -> functions = strvoid_dict_create();
  dict_set_free_data(ret -> functions, (free_t) function_def_free);

  ret -> up = up;
  if (!up) {
    for (def = _builtins; def -> name; def++) {
      script_register(ret, def);
    }
  }
  return ret;
}

script_t * script_set_name(script_t *script, char *name) {
  free(script -> name);
  script -> name = strdup((name && strlen(name)) ? name : "__main__");
  return script;
}

void script_free(script_t *script) {
  if (script) {
    dict_free(script -> variables);
    list_free(script -> stack);
    list_free(script -> instructions);
    dict_free(script -> functions);
    dict_free(script -> labels);
    array_free(script -> params);
    free(script -> name);
    free(script);
   }
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

parser_t * script_parse_init(parser_t *parser) {
  if (script_debug) {
    debug("script_parse_init");
  }
  parser -> data = script_set_name(
    script_create(NULL), NULL);
  return parser;
}

parser_t * script_parse_push_last_token(parser_t *parser) {
  token_t  *token;
  script_t *script;

  script = parser -> data;
  token = parser -> last_token;
  if (script_debug) {
    debug("last_token: %s",
          (token) ? token_tostring(token, NULL, 0) : "--NULL--");
  }
  assert(token);
  script_push(script, data_create_string(token_token(token)));
  return parser;
}

parser_t * script_parse_init_param_count(parser_t *parser) {
  script_t *script;

  script = parser -> data;
  script_push(script, data_create_int(0));
  return parser;
}

parser_t * script_parse_param_count(parser_t *parser) {
  script_t *script;
  data_t   *data;

  script = parser -> data;
  data = script_pop(script);
  data -> intval++;
  if (script_debug) {
    debug("  -- count: %d", data -> intval);
  }
  script_push(script, data);
  return parser;
}

parser_t * script_parse_push_param(parser_t *parser) {
  script_t *script;
  data_t   *data;

  script = parser -> data;
  data = script_pop(script);
  data -> intval++;
  if (script_debug) {
    debug("  -- count: %d", data -> intval);
  }
  script_parse_push_last_token(parser);
  script_push(script, data);
  return parser;
}

parser_t * script_parse_emit_assign(parser_t *parser) {
  char     *varname;
  data_t   *data;
  script_t *script;

  script = parser -> data;
  data = script_pop(script);
  varname = (char *) data -> ptrval;
  if (script_debug) {
    debug("  -- varname: %s", varname);
  }
  script_push_instruction(script, instruction_create_assign(varname));
  data_free(data);
  return parser;
}

parser_t * script_parse_emit_pushvar(parser_t *parser) {
  token_t  *token;
  script_t *script;
  char     *varname;

  script = parser -> data;
  token = parser -> last_token;
  varname = token_token(token);
  if (script_debug) {
    debug("  -- varname: %s", varname);
  }
  script_push_instruction(script, instruction_create_pushvar(varname));
  return parser;
}

parser_t * script_parse_emit_pushval(parser_t *parser) {
  token_t  *token;
  script_t *script;
  data_t   *data;
  char     *str;
  int       type;

  script = parser -> data;
  token = parser -> last_token;
  str = token_token(token);
  type = -1;
  switch (token_code(token)) {
    case TokenCodeDQuotedStr:
    case TokenCodeSQuotedStr:
      type = String;
      break;
    case TokenCodeHexNumber:
    case TokenCodeInteger:
      type = Int;
      break;
    case TokenCodeFloat:
      type = Float;
      break;
  }
  data = data_parse(type, str);
  assert(data);
  if (script_debug) {
    debug(" -- val: %ld", data_tostring(data));
  }
  script_push_instruction(script, instruction_create_pushval(data));
  return parser;
}

parser_t * script_parse_emit_mathop(parser_t *parser) {
  char     *op;
  data_t   *data;
  script_t *script;

  script = parser -> data;
  data = script_pop(script);
  op = (char *) data -> ptrval;
  if (script_debug) {
    debug(" -- op: %s", op);
  }
  script_push_instruction(script,
    instruction_create_function(op, 2));
  data_free(data);
  return parser;
}

parser_t * script_parse_emit_func_call(parser_t *parser) {
  script_t       *script;
  data_t         *func_name;
  data_t         *param_count;

  script = parser -> data;
  param_count = script_pop(script);
  func_name = script_pop(script);
  if (script_debug) {
    debug(" -- param_count: %d", param_count -> intval);
    debug(" -- func_name: %s", (char *) func_name -> ptrval);
  }
  script_push_instruction(script,
    instruction_create_function((char *) func_name -> ptrval,
                                param_count -> intval));
  data_free(param_count);
  data_free(func_name);
  return parser;
}

parser_t * script_parse_emit_test(parser_t *parser) {
  script_t      *script;
  instruction_t *test;
  char           label[10];

  script = parser -> data;
  strrand(label, 9);
  test = instruction_create_test(label);
  script_push(script, data_create_string(label));
  script_push_instruction(script, test);
  return parser;
}

parser_t * script_parse_emit_jump(parser_t *parser) {
  script_t      *script;
  instruction_t *test;
  char           label[10];

  script = parser -> data;
  strrand(label, 9);
  test = instruction_create_jump(label);
  script_push(script, data_create_string(test -> name));
  script_push_instruction(script, test);
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
  script -> label = new(10);
  strrand(script -> label, 9);
  script_push(script, data_create_string(script -> label));
  return parser;
}

parser_t * script_parse_emit_else(parser_t *parser) {
  script_t      *script;
  instruction_t *jump;
  data_t        *label;
  char           newlabel[10];

  script = parser -> data;
  label = script_pop(script);
  if (script_debug) {
    debug(" -- label: %s", (char *) label -> ptrval);
  }
  script -> label = strdup(label -> ptrval);
  data_free(label);
  strrand(newlabel, 9);
  jump = instruction_create_jump(newlabel);
  script_push(script, data_create_string(newlabel));
  script_push_instruction(script, jump);
  return parser;
}

parser_t * script_parse_emit_end(parser_t *parser) {
  script_t      *script;
  data_t        *label;

  script = parser -> data;
  label = script_pop(script);
  if (script_debug) {
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
  label = script_pop(script);
  if (script_debug) {
    debug(" -- end block label: %s", (char *) label -> ptrval);
  }
  block_label = strdup(label -> ptrval);
  data_free(label);

  /*
   * Second label: The one pushed after the while statement. This is the one
   * we have to jump back to:
   */
  label = script_pop(script);
  if (script_debug) {
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
  func = script_create(up);

  data = script_pop(up);
  count = data -> intval;
  data_free(data);
  func -> params = array_create(count);
  array_set_free(func -> params, (free_t) free);
  array_set_tostring(func -> params, (tostring_t) chars);
  for (ix = count - 1; ix >= 0; ix--) {
    data = script_pop(up);
    array_set(func -> params, ix, strdup(data -> ptrval));
    data_free(data);
  }
  data = script_pop(up);
  script_set(up, (char *) data -> ptrval, data_create_script(func));
  script_set_name(func, (char *) data -> ptrval);
  data_free(data);
  if (script_debug) {
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

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

script_t * script_push_instruction(script_t *script, instruction_t *instruction) {
  listnode_t *node;

  list_push(script -> instructions, instruction);
  if (script -> label) {
    instruction_set_label(instruction, script -> label);
    free(script -> label);
    script -> label = NULL;
  }
  if (instruction -> label) {
    node = list_tail_pointer(script -> instructions);
    dict_put(script -> labels, strdup(instruction ->label), node);
  }
  return script;
}

void _script_stack_visitor(data_t *entry) {
  debug("   . %s", data_tostring(entry));
}

void script_stack(script_t *script) {
  debug("-- Script Stack --------------------------------------------------");
  list_visit(script -> stack, (visit_t) _script_stack_visitor);
  debug("------------------------------------------------------------------");
}

data_t * _script_pop(script_t *script) {
  data_t *ret;
  ret = (data_t *) list_pop(script -> stack);
  assert(ret);
  if (script_debug) {
    debug("  Popped %s", data_tostring(ret));
  }
  return ret;
}

script_t * _script_push(script_t *script, data_t *entry) {
  list_push(script -> stack, entry);
  if (script_debug) {
    script_stack(script);
  }
  return script;
}

script_t * _script_set(script_t *script, char *varname, data_t *value) {
  dict_put(script -> variables, strdup(varname), data_copy(value));
  return script;
}

data_t * _script_get(script_t *script, char *varname) {
  data_t *ret;

  ret = (data_t *) dict_get(script -> variables, varname);
  if (!ret && script -> up) {
    ret = script_get(script -> up, varname);
  }
  return ret;
}

script_t * script_register(script_t *script, function_def_t *def) {
  dict_put(script -> functions, strdup(def -> name), function_def_copy(def));
  return script;
}

function_def_t * script_get_function(script_t *script, char *name) {
  function_def_t *ret;

  ret = (function_def_t *) dict_get(script -> functions, name);
  if (!ret && script -> up) {
    ret = script_get_function(script -> up, name);
  }
  return ret;
}

listnode_t * _script_execute_instruction(instruction_t *instr, script_t *script) {
  char       *label;
  listnode_t *node;

  label = instruction_execute(instr, script);
  return (label) ? (listnode_t *) dict_get(script -> labels, label) : NULL;
}

data_t * script_execute(script_t *script) {
  data_t *ret;
  list_clear(script -> stack);
  list_process(script -> instructions, (reduce_t) _script_execute_instruction, script);
  ret = (list_notempty(script -> stack)) ? script_pop(script) : NULL;
  if (script_debug) {
    debug("    Execution of %s done: %s", script -> name, data_tostring(ret));
  }
  return ret;
}

void _script_list_visitor(instruction_t *instruction) {
  debug(instruction_tostring(instruction));
}

void script_list(script_t *script) {
  debug("==================================================================");
  debug("Script Listing - %s", (script -> name ? script -> name : "<<anon>>"));
  debug("------------------------------------------------------------------");
  debug("%-11.11s%-15.15s", "label","Instruction");
  debug("------------------------------------------------------------------");
  list_visit(script -> instructions, (visit_t) _script_list_visitor);
  debug("==================================================================");
}

int script_parse_execute(reader_t *grammar, reader_t *text) {
  parser_t     *parser;
  script_t     *script;
  str_t        *debugstr;
  data_t       *ret;
  int           retval;

  parser = parser_read_grammar(grammar);
  retval = -1;
  if (parser) {
    parser_parse(parser, text);
    script = (script_t *) parser -> data;
    if (script -> label) {
      script_parse_emit_nop(parser);
    }
    if (list_notempty(script -> stack)) {
      debugstr = list_tostr(script -> stack);
      error("Script stack not empty after parse!\n%s", str_chars(debugstr));
      str_free(debugstr);
    }
    if (script_debug) {
      script_list(script);
    }
    ret = script_execute(script);
    if (script_debug) {
      debug("Return value: %s", data_tostring(ret));
      debugstr = dict_tostr(script -> variables);
      debug("Script variables on exit:\n%s", str_chars(debugstr));
      str_free(debugstr);
    }
    script_free(script);
    parser_free(parser);
    retval = (ret && (ret -> type == Int)) ? (int) ret -> intval : 0;
    data_free(ret);
  }
  return retval;
}



