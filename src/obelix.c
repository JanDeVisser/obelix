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

#include <stdio.h>
#include <unistd.h>

#include <expr.h>
#include <file.h>
#include <lexer.h>
#include <parser.h>
#include <resolve.h>
#include <str.h>

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

typedef enum _instruction_type {
  ITAssign,
  ITPushVar,
  ITPushVal,
  ITFunction,
  ITRead,
  ITWrite
} instruction_type_t;

typedef struct _script {
  dict_t   *variables;
  list_t   *stack;
  list_t   *instructions;
} script_t;

typedef data_t * (*obelix_fnc_t)(script_t *, array_t *);

typedef struct _instruction {
  instruction_type_t  type;
  union {
    char             *name;
    obelix_fnc_t      function;
    data_t           *value;
  };
  int                 num;
} instruction_t;

extern instruction_t * instruction_create_assign(void);

extern script_t *      script_create();
extern void            script_free(script_t *);

/* Parsing functions */
extern parser_t *      script_parse_init(parser_t *parser);

/* Execution functions */
extern data_t *        script_pop(script_t *);
extern script_t *      script_push(script_t *, data_t *);
extern script_t *      script_set(script_t *, char *, data_t *);
extern data_t *        script_get(script_t *, char *);

extern parser_t *      script_parse_init(parser_t *);


static instruction_t * _instruction_create(instruction_type_t);
static script_t *      _instruction_execute_assign(instruction_t *, script_t *);
static script_t *      _instruction_execute_pushvar(instruction_t *, script_t *);
static script_t *      _instruction_execute_pushval(instruction_t *, script_t *);
static script_t *      _instruction_execute_function(instruction_t *, script_t *);


typedef script_t * (*script_fnc_t)(instruction_t *, script_t *);

typedef struct _instruction_type_execute {
  instruction_type_t  type;
  script_fnc_t        function;
  char               *name;
} instruction_type_descr_t;


static instruction_type_descr_t instruction_descr_map[] = {
    { type: ITAssign,   function: _instruction_execute_assign,   name: "ITAssign" },
    { type: ITPushVar,  function: _instruction_execute_pushvar,  name: "ITPushVar" },
    { type: ITPushVal,  function: _instruction_execute_pushval,  name: "ITPushVal" },
    { type: ITFunction, function: _instruction_execute_function, name: "ITFunction" }
};


/*
 * instruction_t static functions
 */

instruction_t * _instruction_create(instruction_type_t type) {
  instruction_t *ret;

  ret = NEW(instruction_t);
  ret -> type = type;
  return ret;
}

script_t * _instruction_execute_assign(instruction_t *instr, script_t *script) {
  char          *varname;
  data_t        *value;

  value = script_pop(script);
  assert(value);
  assert(instr -> name);

  script_set(script, instr -> name, value);
  return script;
}

script_t * _instruction_execute_pushvar(instruction_t *instr, script_t *script) {
  data_t        *value;

  assert(instr -> name);
  value = script_get(script, instr -> name);
  assert(value);
  script_push(script, value);
  return script;
}

script_t * _instruction_execute_pushval(instruction_t *instr, script_t *script) {
  data_t        *value;

  assert(instr -> value);
  script_push(script, instr -> value);
  return script;
}

script_t * _instruction_execute_function(instruction_t *instr, script_t *script) {
  data_t   *value;
  array_t  *params;
  int       ix;

  assert(instr -> function);
  params = array_create();
  array_set_free(data_free);
  for (ix = 0; ix < instr -> num; ix++) {
    value = script_pop(script);
    assert(value);
    array_set(params, instr -> num - ix, value);
  }
  value = instr -> function(script, params);
  script_push(script, value);
  array_free(params);
  return script;
}

/*
 * instruction_t public functions
 */

instruction_t * instruction_create_assign(char *varname) {
  instruction_t *ret;

  ret = _instruction_create(ITWrite);
  ret -> name = varname;
  return ret;
}

instruction_t * instruction_create_pushvar(char *varname) {
  instruction_t *ret;

  ret = _instruction_create(ITPushVar);
  ret -> name = varname;
  return ret;
}

instruction_t * instruction_create_pushval(data_t *value) {
  instruction_t *ret;

  ret = _instruction_create(ITPushVal);
  ret -> value = value;
  return ret;
}

instruction_t * instruction_create_function(script_fnc_t fnc, int num_params) {
  instruction_t *ret;

  ret = _instruction_create(ITFunction);
  ret -> function = fnc;
  ret -> num = num_params;
  return ret;
}

script_t * instruction_execute(instruction_t *instr, script_t *script) {
  script_fnc_t fnc;

  fnc = instruction_descr_map[instr -> type].function;
  return fnc(instr, script);
}

void instruction_free(instruction_t *instruction) {
  if (instruction) {
    switch (instruction -> type) {
      case ITPushVar:
      case ITAssign:
        free(instruction -> name);
        break;
    }
    free(instruction);
  }
}

/*
 * script_t static functions
 */

typedef long (*mathop_t)(long, long);

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

script_t * _script_function_mathop(array_t *params, mathop_t mathop) {
  char          *varname;
  data_t        *value;
  long           l1, l2, result;

  assert(array_size(params) == 2);
  value = array_get(params, 0);
  assert(value);
  data_get(value, &l1);
  value = array_get(params, 1);
  assert(value);
  data_get(value, &l2);
  result = mathop(l1, l2);
  return data_create(Int, &result);
}

data_t * _script_function_plus(script_t *script, array_t *params) {
  return _script_function_mathop(params, _plus);
}

data_t * _script_function_minus(script_t *script, array_t *params) {
  return _script_function_mathop(params, _minus);
}

data_t * _script_function_times(script_t *script, array_t *params) {
  return _script_function_mathop(params, _times);
}

data_t * _script_function_div(script_t *script, array_t *params) {
  return _script_function_mathop(params, _div);
}

data_t * _script_function_modulo(script_t *script, array_t *params) {
  return _script_function_mathop(params, _modulo);
}

script_t * _script_function_print(instruction_t *instr, script_t *script) {
  char          *varname;
  data_t        *value;

  value = script_pop(script);
  assert(value);
  printf("%s\n", data_tostring(value));
  free(varname);
  return script;
}

script_t * _script_function_read(instruction_t *instr, script_t *script) {
  data_t        *value;
  char          *line;
  double         d;
  long           l;
  int            ix;
  datatype_t     dt;
  char           ch;
  void           *ptr;

  getline(&line, 0, stdin);
  while (strlen(line) && isspace(line[strlen(line)])) {
    line[strlen(line)] = 0;
  }
  dt = Int;
  for (ix = 0; ((dt == Int) || (dt == Float)) && (ix < strlen(line)); ix++) {
    if (isdigit(ch) || (ch == '.')) {
      if (ch == '.') {
        dt = (dt == Int) ? Float : String;
      }
    } else {
      dt = String;
    }
  }
  switch (dt) {
    case Int:
      l = atol(line);
      ptr = (void *) l;
      break;
    case Float:
      d = atof(line);
      ptr = (void *) d;
      break;
    case String:
      ptr = line;
      break;
  }
  value = data_create(dt, ptr);
  script_push(value);
  free(line);
  return script;
}

/*
 * script_t public functions
 */

script_t * script_create() {
  script_t *ret;

  ret = NEW(script_t);
  ret -> variables = strvoid_dict_create();
  ret -> stack = list_create();
  list_set_free(ret -> stack, data_free);
  ret -> instructions = list_create();
  list_set_free(ret -> instructions, instruction_free);
  return ret;
}

void script_free(script_t *script) {
  if (script) {
    dict_free(script -> variables);
    list_free(script -> stack);
    list_free(script -> instructions);
  }
}

/* ----------------------------------------------------------------------- */

parser_t * script_parse_init(parser_t *parser) {
  script_t *script = script_create();
  parser -> data = script;
  return parser;
}

parser_t * script_parse_push_last_token(parser_t *parser) {
  token_t  *token;
  script_t *script;

  script = parser -> data;
  token = parser -> last_token;
  script_push(script, data_create(String, token_token(token)));
  return parser;
}

parser_t * script_parse_emit_assign(parser_t *parser) {
  char     *varname;
  data_t   *data;
  script_t *script;

  script = parser -> data;
  data = script_pop(script);
  list_push(script -> instructions,
            instruction_create_assign(data -> value));
  data_free(data);
  return parser;
}

parser_t * script_parse_emit_pushvar(parser_t *parser) {
  token_t  *token;
  script_t *script;

  script = parser -> data;
  token = parser -> last_token;
  list_push(script -> instructions,
            instruction_create_pushvar(token_token(token)));
  return parser;
}

parser_t * script_parse_emit_pushval(parser_t *parser) {
  token_t  *token;
  script_t *script;
  long      val;

  script = parser -> data;
  token = parser -> last_token;
  val = strtol(token_token(token), NULL, 10);

  list_push(script -> instructions,
            instruction_create_pushval(data_create(Int, (void *) val)));
  return parser;
}

parser_t * script_parse_emit_mathop(parser_t *parser) {
  char     *op;
  script_fnc_t func;
  data_t   *data;
  script_t *script;

  script = parser -> data;
  data = script_pop(script);
  op = data -> value;
  if (!strcmp(op, "+")) {
    func = resolve_function(_script_function_plus);
  } else if (!strcmp(op, "-")) {
      func = resolve_function(_script_function_minus);
  } else if (!strcmp(op, "*")) {
      func = resolve_function(_script_function_times);
  } else if (!strcmp(op, "/")) {
      func = resolve_function(_script_function_div);
  } else if (!strcmp(op, "%")) {
      func = resolve_function(_script_function_div);
  }
  assert(func);
  list_push(script -> instructions,
            instruction_create_function(func, 2));
  data_free(data);
  return parser;
}


/* ----------------------------------------------------------------------- */

data_t * script_pop(script_t *script) {
  return (data_t *) list_pop(script -> stack);
}

script_t * script_push(script_t *script, data_t *entry) {
  list_push(script -> stack, entry);
  return script;
}

script_t * script_set(script_t *script, char *varname, data_t *value) {
  dict_put(script -> variables, strdup(varname), value);
  return script;
}

data_t * script_get(script_t *script, char *varname) {
  return (data_t *) dict_get(script -> variables, varname);
}

void test_parser(char *gname, char *tname) {
  file_t *     *file_grammar;
  reader_t     *greader;
  reader_t     *treader;
  parser_t     *parser;
  free_t        gfree, tfree;

  if (!gname) {
    //greader = (reader_t *) str_wrap(bnf_grammar);
    greader = (reader_t *) file_open("/home/jan/Projects/obelix/etc/grammar.txt");
    gfree = (free_t) str_free;
  } else {
    greader = (reader_t *) file_open(gname);
    gfree = (free_t) file_free;
  }
  if (!tname) {
    treader = (reader_t *) file_create(fileno(stdin));
  } else {
    treader = (reader_t *) file_open(tname);
  }
  if (greader) {
    parser = parser_read_grammar(greader);
    if (parser) {
      grammar_dump(parser -> grammar);
      parser_parse(parser, treader);
      parser_free(parser);
    }
    file_free((file_t *) treader);
    gfree(greader);
  }
}

extern void foo(void) {
  puts("foo!");
}

int main(int argc, char **argv) {
  char *grammar;
  int   opt;

  grammar = NULL;
  while ((opt = getopt(argc, argv, "g:")) != -1) {
    switch (opt) {
      case 'g':
        grammar = optarg;
        break;
    }
  }
  test_parser(grammar, (argc > optind) ? argv[optind] : NULL);
}
