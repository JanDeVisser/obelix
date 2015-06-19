/*
 * obelix/src/scriptparse.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <bytecode.h>
#include <exception.h>
#include <function.h>
#include <loader.h>
#include <namespace.h>
#include <nvp.h>
#include <script.h>
#include <scriptparse.h>

static void       _script_parse_create_statics(void) __attribute__((constructor));
static data_t *   _script_parse_gen_label(void);
static name_t *   _script_parse_pop_operation(parser_t *);
static parser_t * _script_parse_prolog(parser_t *);
static parser_t * _script_parse_epilog(parser_t *);
static long       _script_parse_get_option(parser_t *, obelix_option_t);

static data_t    *data_error = NULL;
static data_t    *data_end = NULL;

static name_t    *name_end = NULL;
static name_t    *name_error = NULL;
static name_t    *name_query = NULL;
static name_t    *name_hasattr = NULL;
static name_t    *name_self = NULL;
static name_t    *name_reduce = NULL;
static name_t    *name_equals = NULL;
static name_t    *name_or = NULL;
static name_t    *name_and = NULL;

/* ----------------------------------------------------------------------- */

void _script_parse_create_statics(void) {
  data_error   = data_create(String, "ERROR");
  data_end     = data_create(String, "END");

  name_end     = name_create(1, data_tostring(data_end));
  name_error   = name_create(1, data_tostring(data_error));
  name_query   = name_create(1, "query");
  name_hasattr = name_create(1, "hasattr");
  name_self    = name_create(1, "self");
  name_reduce  = name_create(1, "reduce");
  name_equals  = name_create(1, "==");
  name_or      = name_create(1, "or");
  name_and     = name_create(1, "and");
}


data_t * _script_parse_gen_label(void) {
  char label[9];

  do {
    strrand(label, 8);
  } while (isupper(label[0]));
  return data_create(String, label);
}

name_t * _script_parse_pop_operation(parser_t *parser) {
  data_t *data;
  name_t *ret;

  data = datastack_pop(parser -> stack);
  ret = name_create(1, data_tostring(data));
  data_free(data);
  return ret;
}

parser_t * _script_parse_prolog(parser_t *parser) {
  bytecode_t    *bytecode;
  data_t        *data;
  instruction_t *instr;

  bytecode = (bytecode_t *) parser -> data;
  bytecode_push_instruction(bytecode,
                          instruction_create_enter_context(NULL, data_error));
  return parser;
}

parser_t * _script_parse_epilog(parser_t *parser) {
  bytecode_t *bytecode;
  data_t     *data;
  data_t     *instr;

  bytecode = (bytecode_t *) parser -> data;
  instr = list_peek(bytecode -> instructions);
  if (instr) {
    if ((data_type(instr) != ITJump) && !datastack_empty(bytecode -> pending_labels)) {
      /*
       * If the previous instruction was a Jump, and there is no label set for the
       * next statement, we can never get here. No point in emitting a push and
       * jump in that case.
       */
      while (datastack_depth(bytecode -> pending_labels) > 1) {
        bytecode_push_instruction(bytecode, instruction_create_nop());
      }
      data = data_create(Int, 0);
      bytecode_push_instruction(bytecode, instruction_create_pushval(data));
      data_free(data);
    }

    datastack_push(bytecode -> pending_labels, data_copy(data_error));
    bytecode_push_instruction(bytecode,
                              instruction_create_leave_context(name_error));

    datastack_push(bytecode -> pending_labels, data_copy(data_end));
    script_parse_nop(parser);
  }
  if (script_debug || _script_parse_get_option(parser, ObelixOptionList)) {
    bytecode_list(bytecode);
  }
  return parser;
}

long _script_parse_get_option(parser_t *parser, obelix_option_t option) {
  data_t  *options = parser_get(parser, "options");
  array_t *list = data_as_array(options);
  data_t  *opt = data_array_get(list, option);

  return data_intval(opt);
}

/* ----------------------------------------------------------------------- */

parser_t * script_parse_init(parser_t *parser) {
  char       *name;
  module_t   *mod;
  data_t     *data;
  bytecode_t *bytecode;
  script_t   *script;

  if (parser_debug) {
    debug("script_parse_init");
  }
  data = parser_get(parser, "name");
  name = data_tostring(data);
  data = parser_get(parser, "module");
  assert(data);
  mod = data_as_module(data);
  if (script_debug) {
    debug("Parsing module '%s'", name_tostring(mod -> name));
  }
  script = script_create(mod, NULL, name);
  parser_set(parser, "script", data_create(Script, script));
  parser -> data = script -> bytecode;
  _script_parse_prolog(parser);
  return parser;
}

parser_t * script_parse_done(parser_t *parser) {
  if (parser_debug) {
    debug("script_parse_done");
  }
  return _script_parse_epilog(parser);
}

parser_t * script_parse_mark_line(parser_t *parser, data_t *line) {
  bytecode_t *bytecode;

  if (parser -> data) {
    bytecode = (bytecode_t *) parser -> data;
    bytecode -> current_line = data_intval(line);
  }
  return parser;
}

parser_t * script_make_nvp(parser_t *parser) {
  bytecode_t *bytecode;
  data_t     *name;
  data_t     *data;

  data = token_todata(parser -> last_token);
  assert(data);
  name = datastack_pop(parser -> stack);
  assert(name);
  if (parser_debug) {
    debug(" -- %s = %s", data_tostring(name), data_tostring(data));
  }
  datastack_push(parser -> stack, (data_t *) nvp_create(name, data));
  return parser;
}

/* ----------------------------------------------------------------------- */

/*
 * Stack frame for function call:
 *
 *   | kwarg           |
 *   +-----------------+
 *   | kwarg           |
 *   +-----------------+    <- Bookmark for kwarg names
 *   | func_name       |Name
 *   +-----------------+
 *   | . . .           |
 */
parser_t * script_parse_init_function(parser_t *parser) {
  datastack_new_counter(parser -> stack);
  datastack_bookmark(parser -> stack);
  parser_set(parser, "constructor", data_create(Bool, FALSE));
  return parser;
}

parser_t * script_parse_setup_constructor(parser_t *parser, data_t *func) {
  datastack_new_counter(parser -> stack);
  datastack_bookmark(parser -> stack);
  parser_set(parser, "constructor", data_create(Bool, TRUE));
  return parser;
}

parser_t * script_parse_setup_function(parser_t *parser, data_t *func) {
  name_t *name;

  name = name_create(1, data_tostring(func));
  datastack_push(parser -> stack, data_create(Name, name));
  script_parse_init_function(parser);
  return parser;
}

parser_t * script_parse_start_deferred_block(parser_t *parser) {
  bytecode_start_deferred_block((bytecode_t *) parser -> data);
  return parser;
}

parser_t * script_parse_end_deferred_block(parser_t *parser) {
  bytecode_end_deferred_block((bytecode_t *) parser -> data);
  return parser;
}

parser_t * script_parse_pop_deferred_block(parser_t *parser) {
  bytecode_pop_deferred_block((bytecode_t *) parser -> data);
  return parser;
}

parser_t * script_parse_instruction_bookmark(parser_t *parser) {
  bytecode_bookmark((bytecode_t *) parser -> data);
  return parser;
}

parser_t * script_parse_discard_instruction_bookmark(parser_t *parser) {
  bytecode_discard_bookmark((bytecode_t *) parser -> data);
  return parser;
}

parser_t * script_parse_defer_bookmarked_block(parser_t *parser) {
  bytecode_defer_bookmarked_block((bytecode_t *) parser -> data);
  return parser;
}

parser_t * script_parse_instruction(parser_t *parser, data_t *type) {
  typedescr_t *td;

  td = typedescr_get_byname(data_tostring(type));
  if (!type) {
    return NULL;
  } else {
    bytecode_push_instruction((bytecode_t *) parser -> data,
                              data_create(td -> type, NULL, NULL));
    return parser;
  }
}

/* ----------------------------------------------------------------------- */

parser_t * script_parse_assign(parser_t *parser) {
  data_t *varname;

  varname = datastack_pop(parser -> stack);
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_assign(data_as_name(varname)));
  data_free(varname);
  return parser;
}

parser_t * script_parse_pushvar(parser_t *parser) {
  data_t *varname;

  varname = datastack_pop(parser -> stack);
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_pushvar(data_as_name(varname)));
  data_free(varname);
  return parser;
}

parser_t * script_parse_push_token(parser_t *parser) {
  data_t *data;

  data = token_todata(parser -> last_token);
  assert(data);
  if (parser_debug) {
    debug(" -- val: %s", data_tostring(data));
  }
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_pushval(data));
  data_free(data);
  return parser;
}

parser_t * script_parse_pushval_from_stack(parser_t *parser) {
  data_t *data;

  data = datastack_pop(parser -> stack);
  assert(data);
  if (parser_debug) {
    debug(" -- val: %s", data_tostring(data));
  }
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_pushval(data));
  data_free(data);
  return parser;
}

parser_t * script_parse_dupval(parser_t *parser) {
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_dup());
  return parser;
}

parser_t * script_parse_pushconst(parser_t *parser, data_t *constval) {
  data_t *data;

  data = data_decode(data_tostring(constval));
  assert(data);
  if (parser_debug) {
    debug(" -- val: %s", data_tostring(data));
  }
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_pushval(data));
  data_free(data);
  return parser;
}

parser_t *script_parse_push_signed_val(parser_t *parser) {
  data_t *data;
  data_t *signed_val;
  name_t *op;

  data = token_todata(parser -> last_token);
  assert(data);
  op = _script_parse_pop_operation(parser);
  if (parser_debug) {
    debug(" -- val: %s %s", name_tostring(op), data_tostring(data));
  }
  signed_val = data_invoke(data, op, NULL, NULL);
  name_free(op);
  assert(data_type(signed_val) == data_type(data));
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_pushval(signed_val));
  data_free(data);
  data_free(signed_val);
  return parser;
}

parser_t * script_parse_unary_op(parser_t *parser) {
  data_t *op = datastack_pop(parser -> stack);
  name_t *name = name_create(1, data_tostring(op));

  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_function(name, CFInfix, 1, NULL));
  name_free(name);
  data_free(op);
  return parser;
}

parser_t * script_parse_infix_op(parser_t *parser) {
  data_t *op = datastack_pop(parser -> stack);
  name_t *name = name_create(0);

  name_extend_data(name, op);
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_function(name, CFInfix, 2, NULL));
  name_free(name);
  data_free(op);
  return parser;
}

parser_t * script_parse_op(parser_t *parser, name_t *op) {
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_function(op, CFInfix, 2, NULL));
  return parser;
}

parser_t * script_parse_jump(parser_t *parser, data_t *label) {
  if (parser_debug) {
    debug(" -- label: %s", data_tostring(label));
  }
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_jump(data_copy(label)));
  return parser;
}

parser_t * script_parse_stash(parser_t *parser, data_t *stash) {
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_stash(data_intval(stash)));
  return parser;
}

parser_t * script_parse_unstash(parser_t *parser, data_t *stash) {
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_unstash(data_intval(stash)));
  return parser;
}

parser_t* script_parse_reduce(parser_t *parser, data_t *initial) {
  int            init = data_intval(initial);

  if (init) {
    /*
    * reduce() expects the function first and then the initial value. Here we
    * have the function on the top of the stack so we need to swap the
    * function and the initial value.
    */
    bytecode_push_instruction((bytecode_t *) parser -> data,
                              instruction_create_swap());
    bytecode_push_instruction((bytecode_t *) parser -> data,
                              instruction_create_function(name_reduce, CFInfix, 3, NULL));
  } else {
    bytecode_push_instruction((bytecode_t *) parser -> data,
                              instruction_create_function(name_reduce, CFInfix, 2, NULL));
  }
  return parser;
}

parser_t * script_parse_comprehension(parser_t *parser) {
  bytecode_t *bytecode = (bytecode_t *) parser -> data;

  if (parser_debug) {
    debug(" -- Comprehension");
  }
  /*
   * Paste in the deferred generator expression:
   */
  bytecode_pop_deferred_block(bytecode);

  /*
   * Deconstruct the stack:
   *
   * Stash 0: Last generated value
   * Stash 1: Iterator
   * Stash 2: #values
   */
  bytecode_push_instruction(bytecode, instruction_create_stash(0));
  bytecode_push_instruction(bytecode, instruction_create_stash(1));
  bytecode_push_instruction(bytecode, instruction_create_stash(2));

  /*
   * Rebuild stack. Also increment #values.
   *
   * Iterator
   * #values
   * ... values ...
   */
  bytecode_push_instruction(bytecode, instruction_create_unstash(0));

  /* Get #values, increment. Put iterator back on top */
  bytecode_push_instruction(bytecode, instruction_create_unstash(2));
  bytecode_push_instruction(bytecode, instruction_create_incr());
  bytecode_push_instruction(bytecode, instruction_create_unstash(1));

  return parser;
}

parser_t * script_parse_where(parser_t *parser) {
  data_t *label;

  if (parser_debug) {
    debug(" -- Comprehension Where");
  }
  label = data_copy(datastack_peek_deep(parser -> stack, 1));
  if (parser_debug) {
    debug(" -- 'next' label: %s", data_tostring(label));
  }
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_test(label));
  return parser;
}

parser_t * script_parse_func_call(parser_t *parser) {
  data_t     *func_name;
  int         arg_count = 0;
  array_t    *kwargs;
  data_t     *is_constr = parser_get(parser, "constructor");
  data_t     *varargs = parser_get(parser, "varargs");
  callflag_t  flags = 0;

  kwargs = datastack_rollup(parser -> stack);
  if (varargs && data_intval(varargs)) {
    flags |= CFVarargs;
  } else {
    arg_count = datastack_count(parser -> stack);
    if (parser_debug) {
      debug(" -- arg_count: %d", arg_count);
    }
  }
  func_name = datastack_pop(parser -> stack);
  if (is_constr && data_intval(is_constr)) {
    flags |= CFConstructor;
  }
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_function(data_as_name(func_name),
                                                        flags,
                                                        arg_count,
                                                        kwargs));
  data_free(func_name);
  parser_set(parser, "varargs", data_false());
  parser_set(parser, "constructor", data_false());
  return parser;
}

parser_t * script_parse_pop(parser_t *parser) {
  bytecode_push_instruction((bytecode_t *) parser -> data, instruction_create_pop());
  return parser;
}

parser_t * script_parse_nop(parser_t *parser) {
  bytecode_push_instruction((bytecode_t *) parser -> data, instruction_create_nop());
  return parser;
}

parser_t * script_parse_for(parser_t *parser) {
  bytecode_t *bytecode;
  data_t     *next_label = _script_parse_gen_label();
  data_t     *end_label = _script_parse_gen_label();
  data_t     *varname;

  bytecode = (bytecode_t *) parser -> data;
  varname = datastack_pop(parser -> stack);
  datastack_push(parser -> stack, data_copy(next_label));
  datastack_push(parser -> stack, data_copy(end_label));
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_iter());
  datastack_push(bytecode -> pending_labels, data_copy(next_label));
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_next(end_label));
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_assign(data_as_name(varname)));
  data_free(varname);
  data_free(next_label);
  data_free(end_label);
  return parser;
}

parser_t * script_parse_start_loop(parser_t *parser) {
  bytecode_t *bytecode;
  data_t     *label = _script_parse_gen_label();

  bytecode = (bytecode_t *) parser -> data;
  if (parser_debug) {
    debug(" -- loop   jumpback label %s--", data_tostring(label));
  }
  datastack_push(bytecode -> pending_labels, data_copy(label));
  datastack_push(parser -> stack, data_copy(label));
  return parser;
}

parser_t * script_parse_end_loop(parser_t *parser) {
  bytecode_t *bytecode;
  data_t     *label;
  data_t     *block_label;

  bytecode = (bytecode_t *) parser -> data;

  /*
   * First label: The one pushed at the end of the expression. This is the
   * label to be set at the end of the loop:
   */
  block_label = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- end loop label: %s", data_tostring(block_label));
  }

  /*
   * Second label: The one pushed after the while/for statement. This is the one
   * we have to jump back to:
   */
  label = datastack_pop(parser -> stack);
  if (parser_debug) {
    debug(" -- end loop jump back label: %s", data_tostring(label));
  }
  bytecode_push_instruction(bytecode, instruction_create_jump(label));
  data_free(label);

  datastack_push(bytecode -> pending_labels, block_label);

  return parser;
}

parser_t * script_parse_if(parser_t *parser) {
  data_t *endlabel = _script_parse_gen_label();

  if (parser_debug) {
    debug(" -- if     endlabel %s--", data_tostring(endlabel));
  }
  datastack_push(parser -> stack, data_copy(endlabel));
  data_free(endlabel);
  return parser;
}

parser_t * script_parse_test(parser_t *parser) {
  data_t *elselabel = _script_parse_gen_label();

  if (parser_debug) {
    debug(" -- test   elselabel %s--", data_tostring(elselabel));
  }
  datastack_push(parser -> stack, data_copy(elselabel));
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_test(elselabel));
  data_free(elselabel);
  return parser;
}

parser_t * script_parse_elif(parser_t *parser) {
  bytecode_t *bytecode = (bytecode_t *) parser -> data;
  data_t     *elselabel = datastack_pop(parser -> stack);
  data_t     *endlabel = data_copy(datastack_peek(parser -> stack));
  
  if (parser_debug) {
    debug(" -- elif   elselabel: '%s' endlabel '%s'",
          data_tostring(elselabel), data_tostring(endlabel));
  }
  bytecode_push_instruction(bytecode, instruction_create_jump(endlabel));
  datastack_push(bytecode -> pending_labels, data_copy(elselabel));
  data_free(elselabel);
  data_free(endlabel);
  return parser;
}

parser_t * script_parse_else(parser_t *parser) {
  bytecode_t *bytecode = (bytecode_t *) parser -> data;
  data_t     *elselabel = datastack_pop(parser -> stack);
  data_t     *endlabel = data_copy(datastack_peek(parser -> stack));
  
  if (parser_debug) {
    debug(" -- else   elselabel: '%s' endlabel: '%s'", 
          data_tostring(elselabel), 
          data_tostring(endlabel));
  }
  bytecode_push_instruction(bytecode, instruction_create_jump(endlabel));
  datastack_push(bytecode -> pending_labels, data_copy(elselabel));
  datastack_push(parser -> stack, data_copy(endlabel));
  data_free(elselabel);
  data_free(endlabel);
  return parser;
}

parser_t * script_parse_end_conditional(parser_t *parser) {
  bytecode_t *bytecode = (bytecode_t *) parser -> data;
  data_t     *elselabel = datastack_pop(parser -> stack);
  data_t     *endlabel = datastack_pop(parser -> stack);

  if (parser_debug) {
    debug(" -- end    elselabel: '%s' endlabel: '%s'", 
          data_tostring(elselabel), 
          data_tostring(endlabel));
  }
  datastack_push(bytecode -> pending_labels, data_copy(elselabel));
  if (data_cmp(endlabel, elselabel)) {
    datastack_push(bytecode -> pending_labels, data_copy(endlabel));
  }
  data_free(elselabel);
  data_free(endlabel);
  return parser;
}

/* -- S W I T C H  S T A T E M E N T ---------------------------------------*/

parser_t * script_parse_case_prolog(parser_t *parser) {
  bytecode_t *bytecode;
  data_t   *endlabel;
  data_t   *elselabel;
  int       count;

  bytecode = (bytecode_t *) parser -> data;

  /*
   * Get number of case sequences we've had up to now. We only need to
   * emit a Jump if this is not the first case sequence.
   */
  count = datastack_current_count(parser -> stack);

  /*
   * Increment the case sequences counter.
   */
  datastack_increment(parser -> stack);

  /*
   * Initialize counter for the number of cases in this sequence:
   */
  datastack_new_counter(parser -> stack);
  if (parser_debug) {
    debug(" -- elif   elselabel: '%s' endlabel '%s'",
          data_tostring(elselabel), data_tostring(endlabel));
  }
  if (count) {
    elselabel = datastack_pop(parser -> stack);
    endlabel = data_copy(datastack_peek(parser -> stack));
    bytecode_push_instruction(bytecode, instruction_create_jump(endlabel));
    datastack_push(bytecode -> pending_labels, data_copy(elselabel));
    data_free(elselabel);
    data_free(endlabel);
  }
  return parser;
}

parser_t * script_parse_case(parser_t *parser) {
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_unstash(0));
  script_parse_op(parser, name_equals);
  return parser;
}

parser_t * script_parse_rollup_cases(parser_t *parser) {
  int count = datastack_count(parser -> stack);

  if (count > 1) {
    bytecode_push_instruction((bytecode_t *) parser -> data, 
                              instruction_create_function(name_or, CFInfix, count, NULL));
  }
  return parser;
}

/* -- F U N C T I O N  D E F I N I T I O N S -------------------------------*/

parser_t * script_parse_start_function(parser_t *parser) {
  bytecode_t  *up;
  script_t    *func;
  data_t      *data;
  char        *fname;
  data_t      *params;
  int          ix;
  int          async;

  up = (bytecode_t *) parser -> data;

  /* Top of stack: Parameter names as List */
  params = datastack_pop(parser -> stack);

  /* Next on stack: function name */
  data = datastack_pop(parser -> stack);
  fname = strdup(data_tostring(data));
  data_free(data);

  /* sync/async flag */
  data = datastack_pop(parser -> stack);
  async = data_intval(data);
  data_free(data);

  func = script_create(NULL, data_as_script(up -> owner), fname);
  func -> async = async;
  func -> params = str_array_create(array_size(data_as_array(params)));
  array_reduce(data_as_array(params),
               (reduce_t) data_add_strings_reducer,
               func -> params);
  free(fname);
  data_free(params);
  if (parser_debug) {
    debug(" -- defining function %s", name_tostring(func -> name));
  }
  parser -> data = func -> bytecode;
  _script_parse_prolog(parser);
  return parser;
}

parser_t * script_parse_baseclass_constructors(parser_t *parser) {
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_pushvar(name_self));
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_function(name_hasattr, CFNone, 1, NULL));
  script_parse_test(parser);
  return parser;
}

parser_t * script_parse_end_constructors(parser_t *parser) {
  datastack_push(((bytecode_t *) parser -> data) -> pending_labels, 
                 datastack_pop(parser -> stack));
  return parser;
}

parser_t * script_parse_end_function(parser_t *parser) {
  bytecode_t *bytecode = (bytecode_t *) parser -> data;
  script_t   *func = data_as_script(bytecode -> owner);
  
  _script_parse_epilog(parser);
  parser -> data = func -> up -> bytecode;
  return parser;
}

parser_t * script_parse_native_function(parser_t *parser) {
  bytecode_t   *bytecode;
  script_t     *script;
  function_t   *func;
  data_t       *data;
  char         *fname;
  data_t       *params;
  parser_t     *ret = parser;
  int           async;

  bytecode = (bytecode_t *) parser -> data;
  script = data_as_script(bytecode -> owner);

  /* Top of stack: Parameter names as List */
  params = datastack_pop(parser -> stack);

  /* Next on stack: function name */
  data = datastack_pop(parser -> stack);
  fname = strdup(data_tostring(data));
  data_free(data);

  /* sync/async flag */
  data = datastack_pop(parser -> stack);
  async = data_intval(data);
  data_free(data);

  func = function_create(token_token(parser -> last_token), NULL);
  func -> params = str_array_create(array_size(data_as_array(params)));
  func -> async = async;
  array_reduce(data_as_array(params),
               (reduce_t) data_add_strings_reducer,
               func -> params);
  dict_put(script -> functions, fname, func);
  if (parser_debug) {
    debug(" -- defined native function %s", function_tostring(func));
  }
  data_free(params);
  return ret;
}

/* -- E X C E P T I O N  H A N D L I N G -----------------------------------*/

parser_t * script_parse_begin_context_block(parser_t *parser) {
  name_t *varname;
  data_t *data;
  data_t *label = _script_parse_gen_label();

  data = datastack_peek(parser -> stack);
  varname = data_as_name(data);
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_enter_context(varname, label));
  datastack_push(parser -> stack, data_copy(label));
  data_free(label);
  return parser;
}

parser_t * script_parse_throw_exception(parser_t *parser) {
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_throw());
}

parser_t * script_parse_leave(parser_t *parser) {
  bytecode_push_instruction((bytecode_t *) parser -> data,
                          instruction_create_pushval(data_exception(ErrorLeave, "Leave")));
  bytecode_push_instruction((bytecode_t *) parser -> data, 
                            instruction_create_throw());
}

parser_t * script_parse_end_context_block(parser_t *parser) {
  name_t     *varname;
  data_t     *data_varname;
  data_t     *label;
  bytecode_t *bytecode;

  bytecode = (bytecode_t *) parser -> data;
  label = datastack_pop(parser -> stack);
  data_varname = datastack_pop(parser -> stack);
  varname = data_as_name(data_varname);
  bytecode_push_instruction(bytecode,
                            instruction_create_pushval(data_create(Int, 0)));
  datastack_push(bytecode -> pending_labels, data_copy(label));
  bytecode_push_instruction(bytecode,
                            instruction_create_leave_context(varname));
  data_free(data_varname);
  data_free(label);
  return parser;
}

/* -- Q U E R Y ----------------------------------------------------------- */

parser_t * script_parse_query(parser_t *parser) {
  data_t *query;

  query = token_todata(parser -> last_token);
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_pushctx());
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_pushval(query));
  bytecode_push_instruction((bytecode_t *) parser -> data,
                            instruction_create_function(name_query, CFInfix, 2, NULL));
  return parser;
}
