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

static listnode_t *     _closure_execute_instruction(instruction_t *, closure_t *);
static void             _closure_stack(closure_t *);
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
  { token: "==",  fnc: _lt },
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
  new:      (new_t)      _data_new_script,
  copy:     (copydata_t) _data_copy_script,
  cmp:      (cmp_t)      _data_cmp_script,
  free:     (free_t)     script_free,
  tostring: (tostring_t) _data_tostring_script,
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

data_t * _script_function_mathop(closure_t *script, char *op, array_t *params) {
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

  ret = NEW(script_t);
  ret -> variables = strvoid_dict_create();
  dict_set_free_data(ret -> variables, (free_t) data_free);
  dict_set_tostring_data(ret -> variables, (tostring_t) data_tostring);
  ret -> instructions = list_create();
  list_set_free(ret -> instructions, (free_t) instruction_free);
  list_set_tostring(ret -> instructions, (tostring_t) instruction_tostring);

  ret -> labels = strvoid_dict_create();
  ret -> label = NULL;
  ret -> params = NULL;

  ret -> up = up;
  if (!up) {
    ret -> name = "<<root>>";
    for (builtin = _builtins; builtin -> name; builtin++) {
      script_set(ret, builtin -> name, data_create_function(builtin));
    }
  } else {
    if (!name || !name[0]) {
      snprintf(buf, sizeof(buf), "__anon__%d__", hashptr(ret));
      name = buf;
    }
    ret -> name = strdup(name);
    data = data_create_script(ret);
    script_set(up, ret -> name, data);
  }
  ret -> refs++;
  return ret;
}

char * script_get_fullname(script_t *script) {
  char *upname;
  int   sz;

  if (!script -> up) {
    return NULL;
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
  return (script -> up) ? script_get_fullname(script -> up) : NULL;
}

char * script_get_classname(script_t *script) {
  return script -> name;
}

void script_free(script_t *script) {
  if (script) {
    script -> refs--;
    if (script -> refs <= 0) {
      dict_free(script -> variables);
      list_free(script -> stack);
      list_free(script -> instructions);
      dict_free(script -> labels);
      array_free(script -> params);
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

parser_t * script_parse_push_last_token(parser_t *parser) {
  token_t  *token;

  token = parser -> last_token;
  if (parser_debug) {
    debug("      last_token: %s",
          (token) ? token_tostring(token, NULL, 0) : "--NULL--");
  }
  assert(token);
  datastack_push_string(parser -> stack, token_token(token));
  return parser;
}

parser_t * script_parse_init_param_count(parser_t *parser) {
  datastack_push_int(parser -> stack, 0);
  return parser;
}

parser_t * script_parse_param_count(parser_t *parser) {
  data_t   *data;

  data = datastack_pop(parser -> stack);
  data -> intval++;
  if (parser_debug) {
    debug("  -- count: %d", data -> intval);
  }
  datastack_push(parser -> stack, data);
  return parser;
}

parser_t * script_parse_push_param(parser_t *parser) {
  data_t   *data;

  data = datastack_pop(parser -> stack);
  data -> intval++;
  if (parser_debug) {
    debug("  -- count: %d", data -> intval);
  }
  script_parse_push_last_token(parser);
  datastack_push(parser -> stack, data);
  return parser;
}

parser_t * script_parse_init_identifier(parser_t *parser) {
  data_t   *data;

  script_parse_push_last_token(parser);
  datastack_push_int(parser -> stack, 1);
  return parser;
}

parser_t * script_parse_push_identifier_comp(parser_t *parser) {
  data_t   *count;

  count = datastack_pop(parser -> stack);
  count -> intval++;
  if (parser_debug) {
    debug("  -- count: %d", count -> intval);
  }
  script_parse_push_last_token(parser);
  datastack_push(parser -> stack, count);
  return parser;
}

char * _script_pop_and_build_varname(parser_t *parser) {
  char           *varname;
  data_t         *data;
  data_t         *count;
  int             ix;
  int             len;
  list_t         *components;
  listiterator_t *iter;

  count = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug("  -- components: %d", count -> intval);
  }
  assert(count -> intval);
  components = str_list_create();
  len = 0;
  for (ix = 0; ix < count -> intval; ix++) {
    data = datastack_pop(parser -> stack);
    list_push(components, strdup((char *) data -> ptrval));
    len += strlen((char *) data -> ptrval);
    data_free(data);
  }
  data_free(count);
  varname = (char *) new(len + list_size(components));
  iter = li_create(components);
  for (li_tail(iter); li_has_prev(iter); ) {
    strcat(varname, (char *) li_prev(iter));
    if (li_has_prev(iter)) {
      strcat(varname, ".");
    }
  }
  li_free(iter);
  list_free(components);
  if (parser_debug) {
    debug("  -- varname: %s", varname);
  }
  return varname;
}

parser_t * script_parse_emit_assign(parser_t *parser) {
  char     *varname;
  script_t *script;

  script = parser -> data;
  varname = _script_pop_and_build_varname(parser);
  script_push_instruction(script, instruction_create_assign(varname));
  free(varname);
  return parser;
}

parser_t * script_parse_emit_pushvar(parser_t *parser) {
  token_t  *token;
  script_t *script;
  char     *varname;

  script = parser -> data;
  varname = _script_pop_and_build_varname(parser);
  script_push_instruction(script, instruction_create_pushvar(varname));
  free(varname);
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
  char     *func_name;
  data_t   *param_count;

  script = parser -> data;
  datastack_list(parser -> stack);
  param_count = datastack_pop(parser -> stack);
  func_name = _script_pop_and_build_varname(parser);
  if (parser_debug) {
    debug(" -- param_count: %d", param_count -> intval);
    debug(" -- func_name: %s", func_name);
  }
  script_push_instruction(script,
    instruction_create_function(func_name, param_count -> intval));
  data_free(param_count);
  free(func_name);
  return parser;
}

parser_t * script_parse_emit_test(parser_t *parser) {
  script_t      *script;
  instruction_t *test;
  char           label[10];

  script = parser -> data;
  strrand(label, 9);
  test = instruction_create_test(label);
  datastack_push_string(parser -> stack, label);
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
  script -> label = new(10);
  strrand(script -> label, 9);
  datastack_push_string(parser -> stack, script -> label);
  return parser;
}

parser_t * script_parse_emit_else(parser_t *parser) {
  script_t      *script;
  instruction_t *jump;
  data_t        *label;
  char           newlabel[10];

  script = parser -> data;
  label = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- label: %s", (char *) label -> ptrval);
  }
  script -> label = strdup(label -> ptrval);
  data_free(label);
  strrand(newlabel, 9);
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
  params = array_create(count);
  array_set_free(params, (free_t) free);
  array_set_tostring(params, (tostring_t) chars);
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

data_t * script_execute(script_t *script, array_t *params) {
  data_t    *ret;
  closure_t *closure;

  closure = script_create_closure(script, params);
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

script_t * script_set(script_t *script, char *varname, data_t *value) {
  dict_put(script -> variables, strdup(varname), data_copy(value));
  return script;
}

data_t * script_get(script_t *script, char *varname) {
  data_t *ret;

  ret = (data_t *) dict_get(script -> variables, varname);
  return ret;
}

closure_t * script_create_closure(script_t *script, array_t *params) {
  closure_t *ret;
  int        ix;

  if (params || (script -> params &&array_size(script -> params))) {
    assert(params && (array_size(script -> params) == array_size(params)));
  }
  ret = NEW(closure_t);
  ret -> script = script;
  script -> refs++;

  ret -> variables = strvoid_dict_create();
  dict_set_free_data(ret -> variables, (free_t) data_free);
  dict_set_tostring_data(ret -> variables, (tostring_t) data_tostring);
  ret -> stack = datastack_create(script_get_fullname(script));
  datastack_set_debug(ret -> stack, script_debug);

  if (params) {
    for (ix = 0; ix < array_size(params); ix++) {
      closure_set(ret, array_get(script -> params, ix),
                       array_get(params, ix));
    }
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
script_t * _closure_get_script(closure_t *closure, char *name) {
  script_t *script;
  data_t   *s;
  char     *n;
  char     *c;
  char     *ptr;

  if (script_debug) {
    debug("  Getting script for %s", name);
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
    if (closure_get(closure, name) || !script_get(script, name)) {
      if (script_debug) {
	debug("   is closure-local");
      }
      return NULL; /* NULL = closure-local */
    } else {
      if (script_debug) {
	debug("   is root-level");
      }
      return script;
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
  script_t *s = _closure_get_script(closure, varname);
  char     *ptr;

  if (!s) {
    dict_put(closure -> variables, strdup(varname), data_copy(value));
  } else {
    ptr = strrchr(varname, '.');
    ptr = (ptr) ? ptr + 1 : varname;
    script_set(s, varname, value);
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
  scriptloader_t *ret;
  file_t         *grammarfile;

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
  ret -> grammar = grammar_read(grammarfile);
  assert(ret -> grammar);
  file_free(grammarfile);
  ret -> rootscript = script_create(NULL, NULL);
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
    script_free(loader -> rootscript);
    free(loader);
  }
}

script_t * scriptloader_load_fromreader(scriptloader_t *loader, char *name, reader_t *reader) {
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
  script = loader -> rootscript;
  if (name && name[0]) {
    curname = strdup(name);
    modname = curname;
    sepptr = strchr(modname, '.');
    while (sepptr != NULL) {
      *sepptr = 0;
      data = (data_t *) dict_get(script -> variables,  modname);
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

script_t * scriptloader_load(scriptloader_t *loader, char *name) {
  file_t    *text;
  char      *fname;
  fsentry_t *e;
  fsentry_t *init;
  str_t     *dummy;
  char      *ptr;
  script_t  *ret;

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
  str_t        *debugstr;
  data_t       *ret;
  
  script = scriptloader_load(loader, name);
  if (script) {
    ret = script_execute(script, NULL);
    if (script_debug) {
      debug("Return value: %s", data_tostring(ret));
      debugstr = dict_tostr(script -> variables);
      debug("Script variables on exit:\n%s", str_chars(debugstr));
      str_free(debugstr);
    }
    script_free(script);
  }
  return ret;
}



