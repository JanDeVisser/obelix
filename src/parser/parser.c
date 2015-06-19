/*
 * /obelix/src/parser.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
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
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <data.h>
#include <exception.h>
#include <lexer.h>
#include <logging.h>
#include <nvp.h>
#include <parser.h>

typedef struct _parser_stack_entry {
  data_t  _d;
  data_t *subject;
} parser_stack_entry_t;

static void                   _parser_init(void) __attribute__((constructor(102)));

static parser_stack_entry_t * _parser_stack_create(data_t *);
static int                    _parser_stack_entry_register(char *, vtable_t *);
static parser_stack_entry_t * _parser_stack_entry_create(int type, data_t *);
static parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *);
static parser_stack_entry_t * _parser_stack_entry_for_nonterminal(nonterminal_t *);
static parser_stack_entry_t * _parser_stack_entry_for_entry(rule_entry_t *);
static parser_stack_entry_t * _parser_stack_entry_for_action(grammar_action_t *);
static void                   _parser_stack_entry_free(parser_stack_entry_t *);

static parser_t *             _parser_push_grammar_element(parser_t *, ge_t *);
static parser_t *             _parser_push_to_prodstack(parser_t *, parser_stack_entry_t *);
static parser_t *             _parser_set(entry_t *, parser_t *);
static parser_t *             _parser_push_setvalues(entry_t *, parser_t *);
static int                    _parser_ll1_token_handler(token_t *, parser_t *, int);
static parser_t *             _parser_ll1(token_t *, parser_t *);
static parser_t *             _parser_lr1(token_t *, parser_t *);
static lexer_t *              _parser_set_keywords(token_t *, lexer_t *);

extern lexer_t *              parser_newline(lexer_t *, int);

static data_t *               _parser_create(int, va_list);
static void                   _parser_free(parser_t *);
static char *                 _parser_tostring(parser_t *);
static data_t *               _parser_call(parser_t *, array_t *, dict_t *);

       int parser_debug = 0;
extern int script_debug;
       int Parser = -1;
       int ParserStackEntry = -1;

/* ------------------------------------------------------------------------ */

static vtable_t _vtable_parser[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _parser_create },
  { .id = FunctionFree,     .fnc = (void_t) _parser_free },
  { .id = FunctionToString, .fnc = (void_t) _parser_tostring },
  { .id = FunctionCall,     .fnc = (void_t) _parser_call },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_parser = {
  .type = -1,
  .type_name = "parser",
  .vtable = _vtable_parser
};

static vtable_t _vtable_parser_stack_entry[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionFree,     .fnc = (void_t) _parser_stack_entry_free },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_parser_stack_entry = {
  .type = -1,
  .type_name = "parserstackentry",
  .vtable = _vtable_parser_stack_entry
};

#define PSEType(t)                                                           \
int             PSEType ## t = -1;                                           \
static data_t * _pse_call_ ## t(data_t *, array_t *, dict_t *);            \
static data_t * _pse_execute_ ## t(parser_stack_entry_t *, parser_t *, token_t *, int); \
static char *   _pse_allocstring_ ## t(parser_stack_entry_t *);            \
static void     _register_ ## t(void) __attribute__((constructor(200)));   \
static vtable_t _vtable_ ## t [] = {                                         \
  { .id = FunctionCall,        .fnc = (void_t) _pse_call_ ## t },            \
  { .id = FunctionAllocString, .fnc = (void_t) _pse_allocstring_ ## t },     \
  { .id = FunctionNone,        .fnc = NULL }                                 \
};                                                                           \
void _register_ ## t(void) {                                                 \
  PSEType ## t = _parser_stack_entry_register(#t, _vtable_ ## t);            \
}                                                                            \
data_t * _pse_call_ ## t(data_t *data, array_t *p, dict_t *kw) {             \
  parser_stack_entry_t *pse = (parser_stack_entry_t *) data;                 \
  parser_t             *parser = (parser_t *) array_get(p, 0);               \
  token_t              *token = (token_t *) array_get(p, 1);                 \
  data_t               *consuming = data_array_get(p, 2);                    \
  if (parser_debug) {                                                        \
    info("%s", data_tostring(data));                                         \
  }                                                                          \
  return _pse_execute_ ## t(pse, parser, token, data_intval(consuming));     \
}

PSEType(NonTerminal);
PSEType(Rule);
PSEType(Entry);
PSEType(Action)

/* ------------------------------------------------------------------------ */

void _parser_init(void) {
  logging_register_category("parser", &parser_debug);
  Parser = typedescr_register(&_typedescr_parser);
  ParserStackEntry = typedescr_register(&_typedescr_parser_stack_entry);
}

/* -- P A R S E R _ S T A C K _ E N T R Y --------------------------------- */

int _parser_stack_entry_register(char *name, vtable_t *vtable) {
  return typedescr_create_and_register(-1, name, vtable, NULL);
}

parser_stack_entry_t * _parser_stack_entry_create(int type, data_t *subject) {
  parser_stack_entry_t *ret;

  ret = data_new(type, parser_stack_entry_t);
  ret -> subject = subject;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_nonterminal(nonterminal_t *nonterminal) {
  return _parser_stack_entry_create(PSETypeNonTerminal, (data_t *) nonterminal);
}

parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *rule) {
  return _parser_stack_entry_create(PSETypeRule, (data_t *) rule);
}

parser_stack_entry_t * _parser_stack_entry_for_entry(rule_entry_t *entry) {
  return _parser_stack_entry_create(PSETypeEntry, (data_t *) entry);
}

parser_stack_entry_t * _parser_stack_entry_for_action(grammar_action_t *action) {
  return _parser_stack_entry_create(PSETypeAction, (data_t *) action);
}

void _parser_stack_entry_free(parser_stack_entry_t *entry) {
  if (entry) {
    free(entry);
  }
}

/* -- P S E T Y P E  N O N T E R M I N A L -------------------------------- */

char * _pse_allocstring_NonTerminal(parser_stack_entry_t *e) {
  char *buf;

  asprintf(&buf, " N {%s}", data_tostring(e -> subject));
  return buf;
}

data_t * _pse_execute_NonTerminal(parser_stack_entry_t *e, parser_t *parser, token_t *token, int consuming) {
  int            code = token_code(token);
  nonterminal_t *nonterminal = (nonterminal_t *) e -> subject;
  rule_t        *rule;
  int            i;
  rule_entry_t  *entry;
  nonterminal_t *new_nt;

  rule = dict_get_int(nonterminal -> parse_table, code);
  if (!rule) {
    parser -> error = data_exception(ErrorSyntax,
                                 "Unexpected token '%s'",
                                 token_tostring(token));
  } else if (array_size(rule -> entries)) {
    for (i = array_size(rule -> entries) - 1; i >= 0; i--) {
      entry = rule_get_entry(rule, i);
      if (entry -> terminal) {
        _parser_push_grammar_element(parser, (ge_t *) entry);
        _parser_push_to_prodstack(
            parser, _parser_stack_entry_for_entry(entry));
      } else {
        _parser_push_grammar_element(parser, (ge_t *) entry);
        new_nt = grammar_get_nonterminal(parser -> grammar, entry -> nonterminal);
        _parser_push_to_prodstack(
            parser,
            _parser_stack_entry_for_nonterminal(new_nt));
        _parser_push_grammar_element(parser, (ge_t *) new_nt);
      }
    }
  }
  _parser_push_grammar_element(parser, (ge_t *) rule);
  return data_create(Int, 2);
}

/* -- P S E T Y P E  R U L E ---------------------------------------------- */

char * _pse_allocstring_Rule(parser_stack_entry_t *e) {
  char *buf;

  asprintf(&buf, " R {%s}", data_tostring(e -> subject));
  return buf;
}

data_t * _pse_execute_Rule(parser_stack_entry_t *e, parser_t *parser, token_t *token, int consuming) {
  return data_create(Int, consuming);
}

/* -- P S E T Y P E  E N T R Y -------------------------------------------- */

char * _pse_allocstring_Entry(parser_stack_entry_t *e) {
  rule_entry_t *entry = (rule_entry_t *) e -> subject;
  char         *buf;

  if (entry -> terminal) {
    asprintf(&buf, "ET {%s}", token_tostring(entry -> token));
  } else {
    asprintf(&buf, "EN {%s}", entry -> nonterminal);
  }
  return buf;
}

data_t * _pse_execute_Entry(parser_stack_entry_t *e, parser_t *parser, token_t *token, int consuming) {
  rule_entry_t *entry = (rule_entry_t *) e -> subject;

  assert(entry -> terminal);
  if (token_cmp(entry -> token, token)) {
    parser -> error = data_exception(ErrorSyntax,
                                     "Expected '%s' but got '%s' instead",
                                     token_tostring(entry -> token),
                                     token_tostring(token));
  }
  return data_create(Int, 1);
}

/* -- P S E T Y P E  A C T I O N ------------------------------------------ */

char * _pse_allocstring_Action(parser_stack_entry_t *e) {
  char *buf;

  asprintf(&buf, " A {%s}", data_tostring(e -> subject));
  return buf;
}

data_t * _pse_execute_Action(parser_stack_entry_t *e, parser_t *parser, token_t *token, int consuming) {
  parser_t         *ret;
  grammar_action_t *action = (grammar_action_t *) e -> subject;

  assert(action && action -> fnc && action -> fnc -> fnc);
  if (action -> data) {
    ret = ((parser_data_fnc_t) action -> fnc -> fnc)(parser, action -> data);
  } else {
    ret = ((parser_fnc_t) action -> fnc -> fnc)(parser);
  }
  if (!ret) {
    parser -> error = data_exception(ErrorSyntax,
                                     "Error executing grammar action %s",
                                     grammar_action_tostring(action));
  }
  return data_create(Int, consuming);
}

/* -- P A R S E R  D A T A  F U N C T I O N S ----------------------------- */

data_t * _parser_create(int type, va_list args) {
  parser_t *parser = va_arg(args, parser_t *);

  return data_copy(&parser -> _d);
}

void _parser_free(parser_t *parser) {
  if (parser) {
    token_free(parser -> last_token);
    data_free(parser -> error);
    list_free(parser -> prod_stack);
    datastack_free(parser -> stack);
    dict_free(parser -> variables);
    function_free(parser -> on_newline);
    free(parser);
  }
}

char * _parser_tostring(parser_t *parser) {
  if (!parser -> _d.str) {
    asprintf(&parser -> _d.str, "Parser for '%s'",
             data_tostring(parser -> lexer -> reader));
  }
  return NULL;
}

parser_t * _parser_set(entry_t *e, parser_t *parser) {
  parser_set(parser, (char *) e -> key, data_copy((data_t *) e -> value));
  return parser;
}

data_t * _parser_call(parser_t *parser, array_t *args, dict_t *kwargs) {
  data_t *reader = data_array_get(args, 0);

  parser_clear(parser);
  if (kwargs) {
    dict_reduce(kwargs, (reduce_t) _parser_set, parser);
  }
  return parser_parse(parser, reader);
}

data_t * _parser_resolve(parser_t *parser, char *name) {
  if (!strcmp(name, "lexer")) {
    return data_create(Lexer, parser -> lexer);
  } else if (!strcmp(name, "grammar")) {
    return data_create(Grammar, parser -> grammar);
  } else if (!strcmp(name, "line")) {
    return data_create(Int, parser -> line);
  } else if (!strcmp(name, "column")) {
    return data_create(Int, parser -> column);
  } else {
    return data_copy(parser_get(parser, name));
  }
}

/* -- P A R S E R  S T A T I C  F U N C T I O N S ------------------------- */

parser_t * _parser_lr1(token_t *token, parser_t *parser) {
  error("Bottom-up parsing is not yet implemented");
  assert(0);
  return parser;
}

parser_t * _parser_ll1(token_t *token, parser_t *parser) {
  parser_stack_entry_t *entry;
  int                   consuming;
  listiterator_t       *iter;
  void                 *p1, *p2;
  
  if (parser -> last_token) {
    token_free(parser -> last_token);
    parser -> last_token = NULL;
  }
  parser -> last_token = token_copy(token);
  if (parser_debug) {
    warning("== Last Token: %-35.35sLine %d Column %d",
          token_tostring(parser -> last_token),
          parser -> last_token -> line,
          parser -> last_token -> column);
    debug("Production Stack =================================================");
    for(iter = li_create(parser -> prod_stack); li_has_next(iter); ) {
      entry = li_next(iter);
      debug("[ %-32.32s ]",data_tostring((data_t *) entry));
    }
    li_free(iter);
  }
  consuming = 2;
  do {
    consuming = _parser_ll1_token_handler(token, parser, consuming);
  } while (!parser -> error && consuming);
  return parser -> error ? NULL : parser;
}

parser_t * _parser_push_to_prodstack(parser_t *parser, parser_stack_entry_t *entry) {
  list_push(parser -> prod_stack, entry);
  if (parser_debug) {
    debug("      Pushed  %s", data_tostring((data_t *) entry));
  }
  return parser;
}

parser_t * _parser_push_setvalues(entry_t *entry, parser_t *parser) {
  return parser;
  /*
  return _parser_push_to_prodstack(
      parser,
      _parser_stack_entry_for_setvalue(entry -> key, entry -> value));
  */
}

parser_t * _parser_push_grammar_element(parser_t *parser, ge_t *element) {
  grammar_action_t *action;

  dict_reduce(element -> variables, (reduce_t) _parser_push_setvalues, parser);
  for (list_end(element -> actions); list_has_prev(element -> actions); ) {
    action = list_prev(element -> actions);
    _parser_push_to_prodstack(parser, _parser_stack_entry_for_action(action));
  }

  return parser;
}

int _parser_ll1_token_handler(token_t *token, parser_t *parser, int consuming) {
  parser_stack_entry_t *e;
  int                   code;
  data_t               *ret;
  array_t              *args;

  code = token_code(token);
  parser -> line = token -> line;
  parser -> column = token -> column;
  e = list_pop(parser -> prod_stack);
  if (e == NULL) {
    if (parser_debug) {
      debug("Parser stack exhausted");
    }
    if (code != TokenCodeEnd) {
      parser -> error = data_exception(ErrorSyntax,
				       "Expected end of file, read unexpected token '%s'",
				       token_tostring(token));
    }
    consuming = 0;
  } else if ((consuming == 1) && (e -> _d.type != PSETypeAction)) {
    list_push(parser -> prod_stack, e);
    consuming = 0;
  } else {
    if (parser_debug) {
      debug("    Popped  %s", data_tostring((data_t *) e));
    }
    args = data_array_create(3);
    array_push(args, parser_copy(parser));
    array_push(args, token_copy(token));
    array_push(args, data_create(Int, consuming));
    ret = data_call((data_t *) e, args, NULL);
    consuming = data_intval(ret);
    data_free(ret);
    array_free(args);
    _parser_stack_entry_free(e);
  }
  return consuming;
}

lexer_t * _parser_set_keywords(token_t *token, lexer_t *lexer) {
  lexer_add_keyword(lexer, token_code(token), token_token(token));
  return lexer;
}

/*
 * parser_t public functions
 */

parser_t * parser_create(grammar_t *grammar) {
  parser_t *ret;

  ret = data_new(Parser, parser_t);
  ret -> grammar = grammar;
  ret -> prod_stack = list_create();
  ret -> last_token = NULL;
  ret -> error = NULL;
  ret -> stack = datastack_create("__parser__");
  ret -> variables = strdata_dict_create();
  datastack_set_debug(ret -> stack, parser_debug);
  return ret;
}

/**
 * Clears the parser state, i.e. the stack and variables used during the
 * parsing process. This function will also clear the internal stack
 * with production rules, and therefore this function should not be used
 * in the middle of a parsing sequence.
 */
parser_t * parser_clear(parser_t *parser) {
  datastack_clear(parser -> stack);
  list_clear(parser -> prod_stack);
  dict_clear(parser -> variables);
  token_free(parser -> last_token);
  data_free(parser -> error);
  return parser;
}

/**
 * Sets a parser variable. Note the the data object is not copied but added to
 * the parser as-is, therefore the caller should not free it. The key string
 * is copied however.
 */
parser_t * parser_set(parser_t *parser, char *name, data_t *data) {
  dict_put(parser -> variables, strdup(name), data);
  return parser;
}

/**
 * Retrieves a parser variable. The variable remains part of the parser and
 * therefore the caller should not free the returned value.
 *
 * @return The parser variable with the given name, or NULL if the parser has
 * no variable with that name.
 */
data_t * parser_get(parser_t *parser, char *name) {
  return (data_t *) dict_get(parser -> variables, name);
}

/**
 * Retrieves a parser variable and removes it from the parser and therefore
 * the caller is responsible for freeing the return value.
 *
 * @return The parser variable with the given name, or NULL if the parser has
 * no variable with that name.
 *
 * @see
 */
data_t * parser_pop(parser_t *parser, char *name) {
  return (data_t *) dict_pop(parser -> variables, name);
}


data_t * parser_parse(parser_t *parser, data_t *reader) {
  lexer_t        *lexer;
  lexer_option_t  ix;
  function_t     *fnc;
  token_t        *fnc_name;

  token_free(parser -> last_token);
  parser -> last_token = NULL;
  lexer = lexer_create(reader);
  dict_reduce_values(parser -> grammar -> keywords, (reduce_t) _parser_set_keywords, lexer);
  for (ix = 0; ix < LexerOptionLAST; ix++) {
    lexer_set_option(lexer, ix, grammar_get_lexer_option(parser -> grammar, ix));
  }

  parser -> on_newline = NULL;
  fnc_name = (token_t *) dict_get(parser -> grammar -> ge.variables,
                                  "on_newline");
  if (fnc_name) {
    if (parser_debug) {
      debug("Setting on_newline function '%s'", token_token(fnc_name));
    }
    fnc = grammar_resolve_function(parser -> grammar, token_token(fnc_name));
    if (fnc) {
      lexer_set_option(lexer, LexerOptionOnNewLine,
                       (long) function_create("parser_newline",
                                              (void_t) parser_newline));
      parser -> on_newline = fnc;
    } else {
      error("Could not resolve parser on_newline function '%s'", token_token(fnc_name));
    }
  }

  lexer -> data = parser;
  parser -> lexer = lexer;
  switch (grammar_get_parsing_strategy(parser -> grammar)) {
    case ParsingStrategyTopDown:
      list_append(parser -> prod_stack,
                  _parser_stack_entry_for_nonterminal(parser -> grammar -> entrypoint));
      lexer_tokenize(lexer, _parser_ll1, parser);
      break;
    case ParsingStrategyBottomUp:
      lexer_tokenize(lexer, _parser_lr1, parser);
      break;
  }
  if (!parser -> error) {
    if (datastack_notempty(parser -> stack)) {
      error("Parser stack not empty after parse!");
      datastack_list(parser -> stack);
      datastack_clear(parser -> stack);
    }
  }
  token_free(parser -> last_token);
  parser -> last_token = NULL;
  parser -> lexer = NULL;
  lexer_free(lexer);
  return data_copy(parser -> error);
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

lexer_t * parser_newline(lexer_t *lexer, int line) {
  parser_t          *parser = (parser_t *) lexer -> data;
  parser_data_fnc_t  f;
  data_t            *l;

  if (parser && parser -> on_newline) {
    if (parser_debug) {
      debug("Marking line %d", line);
    }
    f = (parser_data_fnc_t) parser -> on_newline -> fnc;
    l = data_create(Int, line);
    parser = f(parser, l);
    data_free(l);
    return (parser) ? lexer : NULL;
  } else {
    return lexer;
  }
}

parser_t * parser_log(parser_t *parser, data_t *msg) {
  info("parser_log: %s", data_tostring(msg));
  return parser;
}

parser_t * parser_set_variable(parser_t *parser, data_t *keyval) {
  nvp_t *nvp = nvp_parse(data_tostring(keyval));

  if (nvp) {
    parser_set(parser, data_tostring(nvp -> name), data_copy(nvp -> value));
    nvp_free(nvp);
  } else {
    parser = NULL;
  }
  return parser;
}

parser_t * parser_rollup_to(parser_t *parser, data_t *marker) {
  char    *m = data_tostring(marker);
  token_t *rolled_up;

  if (m && *m) {
    if (parser_debug) {
      debug("    Rolling up to %c", *m);
    }
    rolled_up = lexer_rollup_to(parser -> lexer, *m);
    if (token_code(rolled_up) == TokenCodeEnd) {
      // FIXME report decent error.
      return NULL;
    } else {
      datastack_push(parser -> stack, token_todata(rolled_up));
    }
    return parser;
  } else {
    return NULL;
  }
}

parser_t * parser_pushval(parser_t *parser, data_t *data) {
  if (parser_debug) {
    debug("    Pushing value %s", data_tostring(data));
  }
  datastack_push(parser -> stack, data_copy(data));
  return parser;
}

parser_t * parser_push(parser_t *parser) {
  data_t   *data = token_todata(parser -> last_token);
  parser_t *ret = parser_pushval(parser, data);

  data_free(data);
  return ret;
}

parser_t * parser_discard(parser_t *parser) {
  data_t   *data = datastack_pop(parser -> stack);

  if (parser_debug) {
    debug("    Discarding value %s", data_tostring(data));
  }
  data_free(data);
  return parser;
}

parser_t * parser_dup(parser_t *parser) {
  return parser_pushval(parser, datastack_peek(parser -> stack));
}

parser_t * parser_push_tokenstring(parser_t *parser) {
  data_t   *data = data_create(String, token_token(parser -> last_token));
  parser_t *ret = parser_pushval(parser, data);

  data_free(data);
  return ret;
}

parser_t * parser_bookmark(parser_t *parser) {
  if (parser_debug) {
    debug("    Setting bookmark at depth %d", datastack_depth(parser -> stack));
  }
  datastack_bookmark(parser -> stack);
  return parser;
}

parser_t * parser_rollup_list(parser_t *parser) {
  array_t *arr;
  data_t  *list;

  arr = datastack_rollup(parser -> stack);
  list = data_create_list(arr);
  if (parser_debug) {
    debug("    Rolled up list '%s' from bookmark", data_tostring(list));
  }
  datastack_push(parser -> stack, list);
  array_free(arr);
  return parser;
}

parser_t * parser_rollup_name(parser_t *parser) {
  name_t  *name;

  name = datastack_rollup_name(parser -> stack);
  if (parser_debug) {
    debug("    Rolled up name '%s' from bookmark", name_tostring(name));
  }
  datastack_push(parser -> stack, data_create(Name, name));
  name_free(name);
  return parser;
}

parser_t * parser_new_counter(parser_t *parser) {
  if (parser_debug) {
    debug("    Setting new counter");
  }
  datastack_new_counter(parser -> stack);
  return parser;
}

parser_t * parser_incr(parser_t *parser) {
  if (parser_debug) {
    debug("    Incrementing counter");
  }
  datastack_increment(parser -> stack);
  return parser;
}

parser_t * parser_count(parser_t *parser) {
  if (parser_debug) {
    debug("    Pushing count to stack");
  }
  datastack_push(parser -> stack,
                 data_create(Int, datastack_count(parser -> stack)));
  return parser;
}

parser_t * parser_discard_counter(parser_t *parser) {
  if (parser_debug) {
    debug("    Discarding counter");
  }
  datastack_count(parser -> stack);
  return parser;
}

