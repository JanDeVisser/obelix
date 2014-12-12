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
#include <file.h>
#include <grammarparser.h>
#include <script.h>
#include <str.h>

int script_debug = 0;

typedef data_t * (*mathop_t)(data_t *, data_t *);

typedef struct _mathop_def {
  char     *token;
  mathop_t  fnc;
} mathop_def_t;


/*
 * S T A T I C  F U N C T I O N S
 */

static data_t *         _data_new_script(data_t *, va_list);
static data_t *         _data_copy_script(data_t *, data_t *);
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

static data_t *         _script_function_print(closure_t *, char *, array_t *);
static data_t *         _script_function_mathop(closure_t *, char *, array_t *);

static void             _script_list_visitor(instruction_t *);
static closure_t *      _script_variable_copier(entry_t *, closure_t *);

static object_t *       _object_create_root(void);

static listnode_t *     _closure_execute_instruction(instruction_t *, closure_t *);
static script_t *       _closure_get_script(closure_t *, char *);


#define FUNCTION_MATHOP  ((voidptr_t) _script_function_mathop)
static function_t _builtins[] = {
    { name: "print",      fnc: ((voidptr_t) _script_function_print),  min_params: 1, max_params: -1 },
    { name: "+",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "-",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "*",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "/",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "%",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "^",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: ">=",         fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: ">",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "<=",         fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "<",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "==",         fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "!=",         fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: NULL,         fnc: NULL,                    min_params: 0, max_params:  0 }
};

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
  { token: "==",  fnc: _eq },
  { token: "!=",  fnc: _leq },
  { token: NULL,  fnc: NULL }
};

static scriptloader_t * _loader = NULL;

/*
 * data_t Script type 
 */

int Script = 0;

static typedescr_t typedescr_script = {
  type:                  -1,
  typecode:              "S",
  new:      (new_t)      _data_new_script,
  copy:     (copydata_t) _data_copy_script,
  cmp:      (cmp_t)      _data_cmp_script,
  free:     (free_t)     script_free,
  tostring: (tostring_t) _data_tostring_script,
  hash:                  NULL,
  parse:                 NULL,
};

data_t * _data_new_script(data_t *ret, va_list arg) {
  script_t *script;

  script = va_arg(arg, script_t *);
  ret -> ptrval = script;
  ((script_t *) ret -> ptrval) -> refs++;
  return ret;
}

data_t * _data_copy_script(data_t *target, data_t *src) {
  target -> ptrval = src -> ptrval;
  ((script_t *) target -> ptrval) -> refs++;
  return target;
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
  snprintf(buf, 32, "<<script %s>>", ((script_t *) d -> ptrval) -> name);
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
    if (type == Int) {
      ret = data_create_int(d1 -> intval + d2 -> intval);
    } else {
      ret = data_create_float(
        (double) ((data_type(d1) == Int) ? d1 -> intval : d1 -> dblval) +
        (double) ((data_type(d2) == Int) ? d2 -> intval : d2 -> dblval));
    }
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

data_t * _script_function_mathop(closure_t *script, char *op, array_t *params) {
  data_t   *d1;
  data_t   *d2;
  data_t   *ret;
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
  ret = fnc(d1, d2);
  if (script_debug) {
    debug("   %s %s %s = %s", data_debugstr(d1), op, data_debugstr(d2), data_debugstr(ret));
  }
  return ret;
}

data_t * _script_function_print(closure_t *script, char *name, array_t *params) {
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

data_t * _script_function_list(closure_t *script, char *op, array_t *params) {
  data_t   *value;
  data_t   *ret;
  int       ix;
  object_t *obj;

  obj = object_create("list");
  obj -> ptr = list_create();
  list_set_free(obj -> ptr, (free_t) data_free);
  list_set_tostring(obj -> ptr, (tostring_t) data_tostring);
  for (ix = 0; ix < array_size(params); ix++) {
    list_push(obj -> ptr, data_copy(array_get(params, ix)));
  }
  ret = data_create(Object, obj);
  return ret;
}

/*
 * script_t static functions
 */

void _script_list_visitor(instruction_t *instruction) {
  debug(instruction_tostring(instruction));
}

closure_t * _script_variable_copier(entry_t *e, closure_t *closure) {
  closure_set(closure, e -> key, data_copy(e -> value));
  return closure;
}

/*
 * script_t public functions
 */

script_t * script_create(script_t *up, char *name) {
  script_t       *ret;
  char            buf[20];
  data_t         *data;
  function_t     *builtin;
  script_t       *bi;

  ret = NEW(script_t);
  ret -> instructions = list_create();
  list_set_free(ret -> instructions, (free_t) instruction_free);
  list_set_tostring(ret -> instructions, (tostring_t) instruction_tostring);

  ret -> functions = strvoid_dict_create();
  dict_set_tostring_data(ret -> functions, (tostring_t) script_get_fullname);
  dict_set_free_data(ret -> functions, (free_t) script_free);

  ret -> labels = strvoid_dict_create();
  ret -> label = NULL;
  ret -> params = NULL;

  ret -> up = up;
  if (!up) {
    ret -> name = "<<root>>";
    for (builtin = _builtins; builtin -> name; builtin++) {
      bi = script_create_function(builtin);
      dict_put(ret -> functions, builtin -> name, bi);
      bi -> refs++;
    }
  } else {
    if (!name || !name[0]) {
      snprintf(buf, sizeof(buf), "__anon__%d__", hashptr(ret));
      name = buf;
    }
    ret -> name = strdup(name);
    data = data_create_script(ret);
    dict_put(up -> functions, ret -> name, ret);
  }
  ret -> refs++;
  return ret;
}

char * script_get_fullname(script_t *script) {
  char *upname;
  int   sz;

  if (!script -> up) {
    return "";
  }
  
  if (!script -> fullname) {
    upname = script_get_fullname(script -> up);
    sz = ((upname && upname[0]) ? strlen(upname) + 1 : 0) + strlen(script -> name) + 1;
    script -> fullname = new(sz);
    if (upname && upname[0]) {
      snprintf(script -> fullname, sz, "%s.%s", upname, script -> name);
    } else {
      strcpy(script -> fullname, script -> name);
    }
  }
  return script -> fullname;
}

char * script_get_modulename(script_t *script) {
  return (script -> up) ? script_get_fullname(script -> up) : "";
}

char * script_get_classname(script_t *script) {
  return script -> name;
}

void script_free(script_t *script) {
  if (script) {
    script -> refs--;
    if (script -> refs <= 0) {
      list_free(script -> instructions);
      dict_free(script -> labels);
      array_free(script -> params);
      dict_free(script -> functions);
      free(script -> name);
      free(script -> fullname);
      free(script);
    }
  }
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

parser_t * script_parse_init(parser_t *parser) {
  if (parser_debug) {
    debug("script_parse_init");
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
    array_set(ret, ix, strdup((char *) data -> ptrval))
    data_free(data);
  }
  data_free(count);
  if (parser_debug) {
    debugstr = array_tostr(ret)
    debug("  -- varname: %s", str_chars(ret));
    str_free(debugstr);
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
  instruction_t *test;
  char           label[9];

  script = parser -> data;
  strrand(label, 8);
  test = instruction_create_jump(label);
  datastack_push_string(parser -> stack, test -> name);
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

data_t * script_execute(script_t *script, array_t *args, dict_t *kwargs) {
  data_t    *ret;
  closure_t *closure;

  closure = script_create_closure(script, args, kwargs);
  ret = closure_execute(closure);
  closure_free(closure);
  return ret;
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

closure_t * script_create_closure(script_t *script, array_t *args, dict_t *kwargs) {
  closure_t *ret;
  int        ix;

  if (args || (script -> params && array_size(script -> params))) {
    assert(args && (array_size(script -> params) == array_size(args)));
  }
  ret = NEW(closure_t);
  ret -> script = script;
  script -> refs++;

  ret -> variables = strdata_dict_create();
  ret -> stack = datastack_create(script_get_fullname(script));
  datastack_set_debug(ret -> stack, script_debug);

  if (args) {
    for (ix = 0; ix < array_size(args); ix++) {
      closure_set(ret, array_get(script -> params, ix),
                       array_get(args, ix));
    }
  }
  if (kwargs) {
    dict_reduce(kwargs, (reduce_t) _script_variable_copier, ret);
  }
  ret -> refs++;
  return ret;
}


/*
 * closure_t - static functions
 */

listnode_t * _closure_execute_instruction(instruction_t *instr, closure_t *closure) {
  char       *label;
  listnode_t *node;

  label = instruction_execute(instr, closure);
  if (label && script_debug) {
    debug("  Jumping to '%s'", label);
  }
  return (label)
      ? (listnode_t *) dict_get(closure -> script -> labels, label)
      : NULL;
}

/**
 * Returns the script in which the identifier <name> belongs. If name does
 * not contain a dot, it is either local to the closure or lives at the
 * root level. Otherwise we split the name, and walk down from the
 * root level.
 */
script_t * _closure_get_script(closure_t *closure, array_t *name) {
  script_t *script;
  data_t   *s;
  char     *n;
  char     *c;
  char     *ptr;
  str_t    *debugstr;

  if (script_debug) {
    debugstr = array_tostr(name);
    debug("  Getting script for %s", str_chars(debugstr));
    str_free(debugstr);
  }
  for (script = closure -> script; script -> up; script = script -> up);
  if (!strchr(name, '.')) {
    /*
     * NOTE: This ensures that assignment of a name that doesn't live locally
     * but does live at the root level gets assigned at the root level. So
     *
     * print = 42
     *
     * Results in the print function being clobbered. This may or may not be
     * what the hacker intended.
     */
    if (closure_get(closure, name)) {
      if (script_debug) {
	debug("   is closure-local");
      }
      return NULL; /* NULL = closure-local */
    } else if (script_get(closure -> script, name)) {
      if (script_debug) {
        debug("   is script-local");
      }
      return closure -> script;
    } else if (script_get(script, name)) {
      if (script_debug) {
	debug("   is root-level");
      }
      return script;
    } else {
      if (script_debug) {
        debug("   doesn't (yet) exist - making it closure-local");
      }
      return NULL;
    }
  }

  /* Copy the name - it may be a static string */
  n = strdup(name);
  c = n;
  ptr = strchr(c, '.');
  while (ptr) {
    *ptr = 0;
    if (script_debug) {
      debug("  Looking for component '%s' in script '%s'", c, script_get_fullname(script));
    }
    s = script_get(script, c);
    c = ptr + 1;
    if (script_debug) {
      debug("   found '%s'", data_tostring(s));
    }	

    /*
     * This is not the last path element - therefore this must exist and
     * must be a script
     *
     * FIXME Error Handling
     */
    assert(s && (data_type(s) == Script));
    script = s -> ptrval;
    c = ptr + 1;
    ptr = strchr(c, '.');
  }
  free(n);
  return script;
}

/*
 * closure_t - public functions
 */

void closure_free(closure_t *closure) {
  if (closure) {
    closure -> refs--;
    if (closure <= 0) {
      script_free(closure -> script);
      datastack_free(closure -> stack);
      dict_free(closure -> variables);
      free(closure);
    }
  }
}

data_t * closure_pop(closure_t *closure) {
  return datastack_pop(closure -> stack);
}

closure_t * closure_push(closure_t *closure, data_t *entry) {
  datastack_push(closure -> stack, entry);
  return closure;
}

closure_t * closure_set(closure_t *closure, char *varname, data_t *value) {
  script_t *s;
  char     *ptr;

  s = _closure_get_script(closure, varname);
  if (!s) {
    if (script_debug) {
      debug("  Setting local '%s' = %s", varname, data_debugstr(value));
    }
    dict_put(closure -> variables, strdup(varname), data_copy(value));
  } else {
    ptr = strrchr(varname, '.');
    ptr = (ptr) ? ptr + 1 : varname;
    if (script_debug) {
      debug("  Setting '%s' -> '%s' = %s", script_get_fullname(s), ptr, data_debugstr(value));
    }
    script_set(s, ptr, value);
  }
  return closure;
}

data_t * closure_get(closure_t *closure, char *varname) {
  data_t *ret;

  ret = (data_t *) dict_get(closure -> variables, varname);
  return ret;
}

/* FIXME: error handling */
data_t * closure_resolve(closure_t *closure, char *name) {
  script_t *s;
  char     *ptr;

  s = _closure_get_script(closure, name);
  if (!s) {
    return closure_get(closure, name);
  } else {
    ptr = strrchr(name, '.');
    ptr = (ptr) ? ptr + 1 : name;
    return script_get(s, ptr);
  }
}

data_t * closure_execute(closure_t *closure) {
  data_t *ret;

  datastack_clear(closure -> stack);
  list_process(closure -> script -> instructions, (reduce_t) _closure_execute_instruction, closure);
  ret = (datastack_notempty(closure -> stack)) ? closure_pop(closure) : data_null();
  if (script_debug) {
    debug("    Execution of %s done: %s", closure -> script -> name, data_tostring(ret));
  }
  return ret;
}

/*
 * scriptloader_t -
 */

#define GRAMMAR_PATH    "/etc/grammar.txt"

scriptloader_t * scriptloader_create(char *obl_path, char *grammarpath) {
  scriptloader_t   *ret;
  file_t           *grammarfile;
  grammar_parser_t *gp;

  assert(!_loader);
  ret = NEW(scriptloader_t);
  ret -> path = strdup(obl_path);
  if (!grammarpath) {
    grammarpath = (char *) new(strlen(ret -> path) + strlen(GRAMMAR_PATH) + 1);
    strcpy(grammarpath, ret -> path);
    strcat(grammarpath, GRAMMAR_PATH);
  }
  debug("obelix path: %s", obl_path);
  debug("grammar file: %s", grammarpath);
  grammarfile = file_open(grammarpath);
  assert(grammarfile);
  gp = grammar_parser_create(grammarfile);
  ret -> grammar = grammar_parser_parse(gp);
  assert(ret -> grammar);
  grammar_parser_free(gp);
  file_free(grammarfile);
  ret -> root = scriptloader_load(ret, "/__root__.obl");
  _loader = ret;
  return ret;
}

scriptloader_t * scriptloader_get(void) {
  return _loader;
}

void scriptloader_free(scriptloader_t *loader) {
  if (loader) {
    free(loader -> path);
    grammar_free(loader -> grammar);
    object_free(loader -> root);
    free(loader);
  }
}

object_t * scriptloader_load_fromreader(scriptloader_t *loader, char *name, reader_t *reader) {
  parser_t       *parser;
  script_t       *script;
  script_t       *modscript;
  char           *curname;
  char           *modname;
  char           *classname;
  char           *sepptr;
  data_t         *data;
  str_t          *debugstr;

  if (script_debug) {
    debug("scriptloader_load_fromreader(%s)", name);
  }
  script = loader -> root;
  if (name && name[0]) {
    curname = strdup(name);
    modname = curname;
    sepptr = strchr(modname, '.');
    while (sepptr != NULL) {
      *sepptr = 0;
      data = NULL; // (data_t *) dict_get(script -> variables,  modname);
      assert(!data || data_type(data) == Script);
      modscript = (data) ? (script_t *) data -> ptrval : NULL;
      if (!modscript) {
        if (script_debug) {
          debug("script %s does not have class %s yet", script -> name, curname);
        }
	modscript = scriptloader_load(loader, curname);
      } else {
        if (script_debug) {
          debug("script %s already has class %s", script -> name, curname);
        }
      }
      script = modscript;
      *sepptr = '.';
      modname = sepptr + 1;
      sepptr = strchr(modname, '.');
    }
    classname = modname;
  } else {
    classname = NULL;
  }

  parser = parser_create(loader -> grammar);
  parser -> data = script_create(script, classname);
  parser_parse(parser, reader);
  script = (script_t *) parser -> data;
  if (script -> label) {
    script_parse_emit_nop(parser);
  }
  if (script_debug) {
    script_list(script);
  }
  parser_free(parser);
  free(curname);
  return script;
}

object_t * scriptloader_load(scriptloader_t *loader, char *name) {
  file_t    *text;
  char      *fname;
  fsentry_t *e;
  fsentry_t *init;
  str_t     *dummy;
  char      *ptr;
  object_t  *ret;

  if (script_debug) {
    debug("scriptloader_load(%s)", name);
  }
  fname = new(strlen(loader -> path) + strlen(name) + 6);
  sprintf(fname, "%s/%s", loader -> path, name);
  for (ptr = strchr(fname, '.'); ptr; ptr = strchr(ptr, '.')) {
    *ptr = '/';
  }
  e = fsentry_create(fname);
  if (fsentry_isdir(e)) {
    init = fsentry_getentry(e, "__init__.obl");
    e = (fsentry_exists(init)) ? init : NULL;
  } else {
    strcat(fname, ".obl");
    e = fsentry_create(fname);
  }
  ret = NULL;
  if (e != NULL) {
    if (fsentry_isfile(e) && fsentry_canread(e)) {
      text = fsentry_open(e);
      assert(text -> fh > 0);
      ret = scriptloader_load_fromreader(loader, name, (reader_t *) text);
      file_free(text);
    }
  } else {
    dummy = str_wrap("");
    ret = scriptloader_load_fromreader(loader, name, (reader_t *) dummy);
    str_free(dummy);
  }
  free(fname);
  return ret;
}


data_t * scriptloader_execute(scriptloader_t *loader, char *name) {
  script_t     *script;
  data_t       *ret;
  
  script = scriptloader_load(loader, name);
  if (script) {
    ret = script_execute(script, NULL);
    if (script_debug) {
      debug("Return value: %s", data_tostring(ret));
    }
    script_free(script);
  }
  return ret;
}



