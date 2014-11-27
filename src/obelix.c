/*
 * obelix.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
 * 
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <expr.h>
#include <file.h>
#include <lexer.h>
#include <parser.h>
#include <resolve.h>
#include <str.h>

/*
static char *bnf_grammar =
    "%\n"
    "name: simple_expr\n"
    "init: dummy_init \n"
    "ignore_ws: true\n"
    "%\n"
    "program                   := statements ;\n"
    "statements                := statement statements | ;\n"
    "statement                 := 'i' \":=\" expr | \"read\" 'i' | \"write\" expr ;\n"
    "expr [ init: start_expr ] := term termtail ;\n"
    "termtail                  := add_op term termtail | ;\n"
    "term                      := factor factortail ; \n"
    "factortail                := mult_op factor factortail | ;\n"
    "factor                    := ( expr ) | 'i' | 'd' ;\n"
    "add_op                    := + | - ;\n"
    "mult_op                   := * | / ;\n";
*/

typedef enum _instruction_type {
  ITAssign,
  ITPushVar,
  ITPushVal,
  ITFunction,
  ITTest,
  ITPop,
  ITJump,
  ITNop
} instruction_type_t;

struct _script;

typedef data_t * (*obelix_fnc_t)(struct _script *, char *, array_t *);
typedef long (*mathop_t)(long, long);


typedef struct _function_def {
  char         *name;
  obelix_fnc_t  fnc;
  int           min_params;
  int           max_params;
} function_def_t;

typedef struct _script {
  dict_t   *variables;
  list_t   *stack;
  list_t   *instructions;
  dict_t   *functions;
  dict_t   *labels;
  char     *label;
} script_t;

typedef struct _instruction {
  instruction_type_t  type;
  char               *label;
  char               *name;
  int                 num;
  union {
    obelix_fnc_t      function;
    data_t           *value;
  };
} instruction_t;

typedef char * (*script_fnc_t)(instruction_t *, script_t *);

extern instruction_t *  instruction_create_assign(char *);
extern instruction_t *  instruction_create_pushvar(char *);
extern instruction_t *  instruction_create_pushval(data_t *);
extern instruction_t *  instruction_create_function(char *, obelix_fnc_t, int);
extern instruction_t *  instruction_create_test(char *);
extern instruction_t *  instruction_create_jump(char *);
extern instruction_t *  instruction_create_pop(void);
extern instruction_t *  instruction_create_nop(void);
extern char *           instruction_assign_label(instruction_t *);
extern instruction_t *  instruction_set_label(instruction_t *, char *);
extern char *           instruction_execute(instruction_t *, script_t *);
extern char *           instruction_tostring(instruction_t *);
extern void             instruction_free(instruction_t *);

extern script_t *       script_create();
extern void             script_free(script_t *);

/* Parsing functions */
extern parser_t *       script_parse_init(parser_t *);
extern parser_t *       script_parse_push_last_token(parser_t *);
extern parser_t *       script_parse_init_param_count(parser_t *);
extern parser_t *       script_parse_param_count(parser_t *);
extern parser_t *       script_parse_emit_assign(parser_t *);
extern parser_t *       script_parse_emit_pushvar(parser_t *);
extern parser_t *       script_parse_emit_pushval(parser_t *);
extern parser_t *       script_parse_emit_mathop(parser_t *);
extern parser_t *       script_parse_emit_func_call(parser_t *);
extern parser_t *       script_parse_emit_jump(parser_t *);
extern parser_t *       script_parse_emit_pop(parser_t *);
extern parser_t *       script_parse_emit_nop(parser_t *);
extern parser_t *       script_parse_emit_test(parser_t *);
extern parser_t *       script_parse_push_label(parser_t *);
extern parser_t *       script_parse_emit_else(parser_t *);
extern parser_t *       script_parse_emit_end(parser_t *);
extern script_t *       script_push_instruction(script_t *, instruction_t *);

/* Execution functions */
extern data_t *         script_pop(script_t *);
extern script_t *       script_push(script_t *, data_t *);
extern script_t *       script_set(script_t *, char *, data_t *);
extern data_t *         script_get(script_t *, char *);
extern script_t *       script_register(script_t *, function_def_t *);
extern function_def_t * script_get_function(script_t *, char *);
extern void             script_list(script_t *script);
extern data_t *         script_execute(script_t *);
extern int              script_parse_execute(reader_t *, reader_t *);

extern function_def_t * function_def_create(char *, obelix_fnc_t, int, int);
extern function_def_t * function_def_copy(function_def_t *);
extern void             function_def_free(function_def_t *);

/*
 * S T A T I C  F U N C T I O N S
 */

static instruction_t *  _instruction_create(instruction_type_t);
static void             _instruction_free_name(instruction_t *);
static void             _instruction_free_value(instruction_t *);
static char *           _instruction_tostring_name(instruction_t *);
static char *           _instruction_tostring_value(instruction_t *);
static char *           _instruction_execute_assign(instruction_t *, script_t *);
static char *           _instruction_execute_pushvar(instruction_t *, script_t *);
static char *           _instruction_execute_pushval(instruction_t *, script_t *);
static char *           _instruction_execute_function(instruction_t *, script_t *);
static char *           _instruction_execute_test(instruction_t *, script_t *);
static char *           _instruction_execute_jump(instruction_t *, script_t *);
static char *           _instruction_execute_pop(instruction_t *, script_t *);
static char *           _instruction_execute_nop(instruction_t *, script_t *);

static long             _plus(long, long);
static long             _minus(long, long);
static long             _times(long, long);
static long             _div(long, long);
static long             _modulo(long, long);
static long             _pow(long, long);
static long             _gt(long, long);
static long             _geq(long, long);
static long             _lt(long, long);
static long             _leq(long, long);
static long             _eq(long, long);
static long             _neq(long, long);

static data_t *         _script_function_print(script_t *, char *, array_t *);
static data_t *         _script_function_mathop(script_t *, char *, array_t *);

static listnode_t *     _script_execute_instruction(instruction_t *, script_t *);
static void             _script_list_visitor(instruction_t *);

typedef struct _instruction_type_descr {
  instruction_type_t  type;
  script_fnc_t        function;
  char               *name;
  free_t              free;
  tostring_t          tostring;
} instruction_type_descr_t;

static instruction_type_descr_t instruction_descr_map[] = {
    { type: ITAssign,   function: _instruction_execute_assign,
      name: "ITAssign",   free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITPushVar,  function: _instruction_execute_pushvar,
      name: "ITPushVar",  free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITPushVal,  function: _instruction_execute_pushval,
      name: "ITPushVal",  free: (free_t) _instruction_free_value,
      tostring: (tostring_t) _instruction_tostring_value },
    { type: ITFunction, function: _instruction_execute_function,
      name: "ITFunction", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITTest, function: _instruction_execute_test,
      name: "ITTest", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITPop, function: _instruction_execute_pop,
      name: "ITPop", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITJump, function: _instruction_execute_jump,
      name: "ITJump", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITNop, function: _instruction_execute_nop,
      name: "ITNop", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
};

static function_def_t _builtins[] = {
    { name: "print", fnc: _script_function_print, min_params: 1, max_params: -1 },
    { name: NULL,    fnc: NULL,                   min_params: 0, max_params:  0 }
};

/*
 * function_def_t public functions
 */

function_def_t * function_def_create(char *name, obelix_fnc_t fnc, int min_params, int max_params) {
  function_def_t *def;

  def = NEW(function_def_t);
  def -> name = strdup(name);
  def -> fnc = fnc;
  def -> min_params = min_params;
  def -> max_params = max_params;
  return def;
}

function_def_t * function_def_copy(function_def_t *src) {
  return function_def_create(src -> name, src -> fnc, src -> min_params, src -> max_params);
}

void function_def_free(function_def_t *def) {
  if (def) {
    free(def -> name);
    free(def);
  }
}

/*
 * instruction_t static functions
 */

instruction_t * _instruction_create(instruction_type_t type) {
  instruction_t *ret;

  ret = NEW(instruction_t);
  ret -> type = type;
  return ret;
}

void _instruction_free_name(instruction_t *instruction) {
  free(instruction -> name);
}

void _instruction_free_value(instruction_t *instruction) {
  data_free(instruction -> value);
}

char * _instruction_tostring_name(instruction_t *instruction) {
  return instruction -> name;
}

char * _instruction_tostring_value(instruction_t *instruction) {
  return data_tostring(instruction -> value);
}

char * _instruction_execute_assign(instruction_t *instr, script_t *script) {
  char          *varname;
  data_t        *value;

  value = script_pop(script);
  assert(value);
  assert(instr -> name);
  debug(" -- value '%s'", data_tostring(value));

  script_set(script, instr -> name, value);
  data_free(value);
  return NULL;
}

char * _instruction_execute_pushvar(instruction_t *instr, script_t *script) {
  data_t        *value;

  assert(instr -> name);
  value = script_get(script, instr -> name);
  assert(value);
  debug(" -- value '%s'", data_tostring(value));
  script_push(script, data_copy(value));
  return NULL;
}

char * _instruction_execute_pushval(instruction_t *instr, script_t *script) {
  data_t        *value;

  assert(instr -> value);
  script_push(script, data_copy(instr -> value));
  return NULL;
}

char * _instruction_execute_function(instruction_t *instr, script_t *script) {
  data_t   *value;
  array_t  *params;
  int       ix;
  str_t    *debugstr;
  
  debug(" -- #parameters: %d", instr -> num);
  assert(instr -> function);
  params = array_create(instr -> num);
  array_set_free(params, (free_t) data_free);
  array_set_tostring(params, (tostring_t) data_tostring);
  for (ix = 0; ix < instr -> num; ix++) {
    value = script_pop(script);
    assert(value);
    array_set(params, instr -> num - ix - 1, value);
  }
  debugstr = array_tostr(params);
  debug(" -- parameters: %s", str_chars(debugstr));
  str_free(debugstr);
  value = instr -> function(script, instr -> name, params);
  value = value ? value : data_create_int(0);
  debug(" -- return: '%s'", data_tostring(value));
  script_push(script, value);
  array_free(params);
  return NULL;
}

char * _instruction_execute_test(instruction_t *instr, script_t *script) {
  char          *ret;
  data_t        *value;

  value = script_pop(script);
  assert(value);
  assert(instr -> name);

  /* FIXME - Convert other objects to boolean */
  ret = (!value -> intval) ? instr -> name : NULL;
  data_free(value);
  return ret;
}

char * _instruction_execute_jump(instruction_t *instr, script_t *script) {
  assert(instr -> name);
  return instr -> name;
}

char * _instruction_execute_pop(instruction_t *instr, script_t *script) {
  char          *ret;
  data_t        *value;

  value = script_pop(script);
  data_free(value);
  return NULL;
}

char * _instruction_execute_nop(instruction_t *instr, script_t *script) {
  return NULL;
}

/*
 * instruction_t public functions
 */

instruction_t * instruction_create_assign(char *varname) {
  instruction_t *ret;

  ret = _instruction_create(ITAssign);
  ret -> name = strdup(varname);
  return ret;
}

instruction_t * instruction_create_pushvar(char *varname) {
  instruction_t *ret;

  ret = _instruction_create(ITPushVar);
  ret -> name = strdup(varname);
  return ret;
}

instruction_t * instruction_create_pushval(data_t *value) {
  instruction_t *ret;

  ret = _instruction_create(ITPushVal);
  ret -> value = value;
  return ret;
}

instruction_t * instruction_create_function(char *name, obelix_fnc_t fnc, int num_params) {
  instruction_t *ret;

  ret = _instruction_create(ITFunction);
  ret -> name = strdup((name) ? name : "<<anon>>");
  ret -> function = fnc;
  ret -> num = num_params;
  return ret;
}

instruction_t * instruction_create_test(char *label) {
  instruction_t *ret;

  ret = _instruction_create(ITTest);
  ret -> name = strdup(label);
  return ret;
}

instruction_t * instruction_create_jump(char *label) {
  instruction_t *ret;

  ret = _instruction_create(ITJump);
  ret -> name = strdup(label);
  return ret;
}


instruction_t * instruction_create_pop(void) {
  instruction_t *ret;

  ret = _instruction_create(ITPop);
  ret -> name = strdup("Discard top-of-stack");
  return ret;
}

instruction_t * instruction_create_nop(void) {
  instruction_t *ret;

  ret = _instruction_create(ITNop);
  ret -> name = strdup("Nothing");
  return ret;
}

char * instruction_assign_label(instruction_t *instruction) {
  char buf[10];

  strrand(buf, 9);
  instruction_set_label(instruction, buf);
  return instruction -> label;
}

instruction_t * instruction_set_label(instruction_t *instruction, char *label) {
  instruction -> label = strdup(label);
  return instruction;
}

char * instruction_execute(instruction_t *instr, script_t *script) {
  script_fnc_t fnc;

  debug("Executing %s", instruction_tostring(instr));
  fnc = instruction_descr_map[instr -> type].function;
  return fnc(instr, script);
}

char * instruction_tostring(instruction_t *instruction) {
  tostring_t   tostring;
  char        *s;
  static char  buf[100];
  char        *lbl;

  tostring = instruction_descr_map[instruction -> type].tostring;
  if (tostring) {
    s = tostring(instruction);
  } else {
    s = "";
  }
  lbl = (instruction -> label) ? instruction -> label : "";
  snprintf(buf, 100, "%-11.11s%-15.15s%s", lbl, instruction_descr_map[instruction -> type].name, s);
  return buf;
}

void instruction_free(instruction_t *instruction) {
  free_t freefnc;

  if (instruction) {
    freefnc = instruction_descr_map[instruction -> type].free;
    if (freefnc) {
      freefnc(instruction);
    }
    if (instruction -> label) {
      free(instruction -> label);
    }
    free(instruction);
  }
}

/*
 * script_t static functions
 */

long _plus(long l1, long l2) {
  return l1 + l2;
}

long _minus(long l1, long l2) {
  return l1 - l2;
}

long _times(long l1, long l2) {
  return l1 * l2;
}

long _div(long l1, long l2) {
  return l1 / l2;
}

long _modulo(long l1, long l2) {
  return l1 % l2;
}

long _pow(long l1, long l2) {
  return (long) pow(l1, l2);
}

long _gt(long l1, long l2) {
  return (long) (l1 > l2);
}

long _geq(long l1, long l2) {
  return (long) (l1 >= l2);
}

long _lt(long l1, long l2) {
  return (long) (l1 < l2);
}

long _leq(long l1, long l2) {
  return (long) (l1 <= l2);
}

long _eq(long l1, long l2) {
  return (long) (l1 == l2);
}

long _neq(long l1, long l2) {
  return (long) (l1 != l2);
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

data_t * _script_function_mathop(script_t *script, char *op, array_t *params) {
  char     *varname;
  data_t   *value;
  long      l1, l2, result;
  int       ix;
  mathop_t  fnc;

  debug("_script_function_mathop: num params: %d", array_size(params));
  assert(array_size(params) == 2);
  value = array_get(params, 0);
  assert(value);
  l1 = value -> intval;
  value = array_get(params, 1);
  assert(value);
  l2 = value -> intval;
  fnc = NULL;
  for (ix = 0; mathops[ix].token; ix++) {
    if (!strcmp(mathops[ix].token, op)) {
      fnc = mathops[ix].fnc;
      break;
    }
  }
  assert(fnc);
  result = fnc(l1, l2);
  return data_create_int(result);
}

data_t * _script_function_print(script_t *script, char *name, array_t *params) {
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
  debug(data_tostring(value));
  return data_create_int(1);
}

/*
 * script_t public functions
 */

script_t * script_create() {
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

  ret -> functions = strvoid_dict_create();
  dict_set_free_data(ret -> functions, (free_t) function_def_free);
  for (def = _builtins; def -> name; def++) {
    script_register(ret, def);
  }
  return ret;
}

void script_free(script_t *script) {
  if (script) {
    dict_free(script -> variables);
    list_free(script -> stack);
    list_free(script -> instructions);
    dict_free(script -> functions);
    dict_free(script -> labels);
    free(script);
   }
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

parser_t * script_parse_init(parser_t *parser) {
  script_t *script;

  debug("script_parse_init");
  script = script_create();
  parser -> data = script;
  return parser;
}

parser_t * script_parse_push_last_token(parser_t *parser) {
  token_t  *token;
  script_t *script;

  script = parser -> data;
  token = parser -> last_token;
  debug("last_token: %s",
	(token) ? token_tostring(token, NULL, 0) : "--NULL--");
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
  debug("  -- count: %d", data -> intval);
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
  debug("  -- varname: %s", varname);
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
  debug("  -- varname: %s", varname);
  script_push_instruction(script, instruction_create_pushvar(varname));
  return parser;
}

parser_t * script_parse_emit_pushval(parser_t *parser) {
  token_t  *token;
  script_t *script;
  long      val;

  script = parser -> data;
  token = parser -> last_token;
  val = strtol(token_token(token), NULL, 10);
  debug(" -- val: %ld", val);

  script_push_instruction(script,
    instruction_create_pushval(data_create(Int, (void *) val)));
  return parser;
}

parser_t * script_parse_emit_mathop(parser_t *parser) {
  char     *op;
  obelix_fnc_t func;
  data_t   *data;
  script_t *script;
  
  script = parser -> data;
  data = script_pop(script);
  op = (char *) data -> ptrval;
  debug(" -- op: %s", op);
  script_push_instruction(script,
    instruction_create_function(op,
      (obelix_fnc_t) _script_function_mathop, 2));
  data_free(data);
  return parser;
}

parser_t * script_parse_emit_func_call(parser_t *parser) {
  obelix_fnc_t    func;
  data_t         *func_name;
  data_t         *param_count;
  function_def_t *def;
  script_t       *script;
  
  script = parser -> data;
  param_count = script_pop(script);
  debug(" -- param_count: %d", param_count -> intval);
  func_name = script_pop(script);
  debug(" -- func_name: %s", (char *) func_name -> ptrval);
  def = script_get_function(script, (char *) func_name -> ptrval);
  assert(def);
  script_push_instruction(script,
    instruction_create_function(def -> name, def -> fnc, param_count -> intval));
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
  debug(" -- label: %s", (char *) label -> ptrval);
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
  debug(" -- label: %s", (char *) label -> ptrval);
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
  debug(" -- end block label: %s", (char *) label -> ptrval);
  block_label = strdup(label -> ptrval);
  data_free(label);

  /*
   * Second label: The one pushed after the while statement. This is the one
   * we have to jump back to:
   */
  label = script_pop(script);
  debug(" -- jump back label: %s", (char *) label -> ptrval);
  jump = instruction_create_jump((char *) label -> ptrval);
  script_push_instruction(script, jump);
  data_free(label);

  script -> label = block_label;

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

data_t * script_pop(script_t *script) {
  data_t *ret;
  ret = (data_t *) list_pop(script -> stack);
  assert(ret);
  debug("  Popped %s", data_tostring(ret));
  return ret;
}

script_t * script_push(script_t *script, data_t *entry) {
  list_push(script -> stack, entry);
  script_stack(script);
  return script;
}

script_t * script_set(script_t *script, char *varname, data_t *value) {
  dict_put(script -> variables, strdup(varname), data_copy(value));
  return script;
}

data_t * script_get(script_t *script, char *varname) {
  return (data_t *) dict_get(script -> variables, varname);
}

script_t * script_register(script_t *script, function_def_t *def) {
  dict_put(script -> functions, strdup(def -> name), function_def_copy(def));
  return script;
}

function_def_t * script_get_function(script_t *script, char *name) {
  return (function_def_t *) dict_get(script -> functions, name);
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
  debug("Execution done: %s", data_tostring(ret));
  return ret;
}

void _script_list_visitor(instruction_t *instruction) {
  debug(instruction_tostring(instruction));
}

void script_list(script_t *script) {
  list_visit(script -> instructions, (visit_t) _script_list_visitor);
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
    grammar_dump(parser -> grammar);
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
    script_list(script);
    ret = script_execute(script);
    debug("Return value: %s", data_tostring(ret));
    debugstr = dict_tostr(script -> variables);
    debug("Script variables on exit:\n%s", str_chars(debugstr));
    str_free(debugstr);
    script_free(script);
    parser_free(parser);
    retval = (ret && (ret -> type == Int)) ? (int) ret -> intval : 0;
    data_free(ret);
  }
  return retval;
}

int parse(char *gname, char *tname) {
  file_t *     *file_grammar;
  reader_t     *greader;
  reader_t     *treader;
  parser_t     *parser;
  script_t     *script;
  free_t        gfree, tfree;
  int           ret;

  if (!gname) {
    //greader = (reader_t *) str_wrap(bnf_grammar);
    greader = (reader_t *) file_open("/home/jan/Projects/obelix/etc/grammar.txt");
    gfree = (free_t) file_free;
  } else {
    greader = (reader_t *) file_open(gname);
    gfree = (free_t) file_free;
  }
  if (!tname) {
    treader = (reader_t *) file_create(fileno(stdin));
  } else {
    treader = (reader_t *) file_open(tname);
  }
  ret = -1;
  if (greader && treader) {
    ret = script_parse_execute(greader, treader);
    file_free((file_t *) treader);
    gfree(greader);
  }
  return ret;
}

int main(int argc, char **argv) {
  char *grammar;
  int   opt;
  int   ret;

  grammar = NULL;
  while ((opt = getopt(argc, argv, "g:")) != -1) {
    switch (opt) {
      case 'g':
        grammar = optarg;
        break;
    }
  }
  ret = parse(grammar, (argc > optind) ? argv[optind] : NULL);
  debug("Exiting with exit code %d", ret);
  return ret;
}
