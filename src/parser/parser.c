/*
 * /obelix/src/parser/parser.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include "libparser.h"
#include <data.h>
#include <exception.h>
#include <lexer.h>
#include <logging.h>
#include <nvp.h>

typedef struct _parser_stack_entry {
  data_t  _d;
  data_t *subject;
  int     filter;
} parser_stack_entry_t;


typedef int (*pse_execute_t)(parser_stack_entry_t *, parser_t *, token_t *);

static inline void            _parser_init(void);

static parser_stack_entry_t * _parser_stack_entry_create(int, data_t *, int);
static parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *);
static parser_stack_entry_t * _parser_stack_entry_for_nonterminal(nonterminal_t *);
static parser_stack_entry_t * _parser_stack_entry_for_entry(rule_entry_t *);
static parser_stack_entry_t * _parser_stack_entry_for_action(grammar_action_t *);

static int                    _parser_stack_entry_call(parser_stack_entry_t *, parser_t *, token_t *);
static parser_stack_entry_t * _parser_stack_entry_new(parser_stack_entry_t *, va_list);
static void                   _parser_stack_entry_free(parser_stack_entry_t *);

static parser_t *             _parser_push_grammar_element(parser_t *, ge_t *);
static parser_t *             _parser_push_to_prodstack(parser_t *, parser_stack_entry_t *);
static parser_t *             _parser_set(entry_t *, parser_t *);
static int                    _parser_ll1_token_handler(token_t *, parser_t *, int);
static parser_t *             _parser_ll1(token_t *, parser_t *);
static parser_t *             _parser_lr1(token_t *, parser_t *);

static parser_t *             _parser_new(parser_t *, va_list);
static void                   _parser_free(parser_t *);
static char *                 _parser_allocstring(parser_t *);
static data_t *               _parser_call(parser_t *, array_t *, dict_t *);

int parser_debug = 0;
int Parser = -1;
int ParserStackEntry = -1;

static token_t *token_end = NULL;

static code_label_t _parser_states[] = {
  code_label(ParserStateNone),
  code_label(ParserStateNonTerminal),
  code_label(ParserStateTerminal),
  code_label(ParserStateRule),
  code_label(ParserStateDone),
  code_label(ParserStateError)
};

/* ------------------------------------------------------------------------ */

static vtable_t _vtable_Parser[] = {
  { .id = FunctionNew,         .fnc = (void_t) _parser_new },
  { .id = FunctionFree,        .fnc = (void_t) _parser_free },
  { .id = FunctionAllocString, .fnc = (void_t) _parser_allocstring },
  { .id = FunctionCall,        .fnc = (void_t) _parser_call },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ParserStackEntry[] = {
  { .id = FunctionNew,   .fnc = (void_t) _parser_stack_entry_new },
  { .id = FunctionFree,  .fnc = (void_t) _parser_stack_entry_free },
  { .id = FunctionNone,  .fnc = NULL }
};

#define data_is_parser_stack_entry(d)  ((d) && data_hastype((d), ParserStackEntry))
#define data_as_parser_stack_entry(d)  (data_is_parser_stack_entry((d)) ? ((parser_stack_entry_t *) (d)) : NULL)
#define parser_stack_entry_copy(o)     ((parser_stack_entry_t *) data_copy((data_t *) (o)))
#define parser_stack_entry_tostring(o) (data_tostring((data_t *) (o)))
#define parser_stack_entry_free(o)     (data_free((data_t *) (o)))

#define PSEType(t)                                                           \
int             PSEType ## t = -1;                                           \
static int      _pse_execute_ ## t(parser_stack_entry_t *, parser_t *, token_t *); \
static char *   _pse_allocstring_ ## t(parser_stack_entry_t *);              \
static void     _register_ ## t(void);                                       \
static vtable_t _vtable_PSEType ## t [] = {                                  \
  { .id = FunctionUsr1,        .fnc = (void_t) _pse_execute_ ## t },         \
  { .id = FunctionAllocString, .fnc = (void_t) _pse_allocstring_ ## t },     \
  { .id = FunctionNone,        .fnc = NULL }                                 \
};                                                                           \
void _register_ ## t(void) {                                                 \
  typedescr_register(PSEType ## t, parser_t);                                \
  typedescr_assign_inheritance(PSEType ## t, ParserStackEntry);              \
}

PSEType(NonTerminal);
PSEType(Rule);
PSEType(Entry);
PSEType(Action);

/* ------------------------------------------------------------------------ */

void _parser_init(void) {
  if (Parser < 1) {
    logging_register_category("parser", &parser_debug);
    dictionary_init();
    typedescr_register(Parser, parser_t);
    typedescr_assign_inheritance(Parser, Dictionary);
    typedescr_register(ParserStackEntry, parser_stack_entry_t);
    _register_NonTerminal();
    _register_Rule();
    _register_Entry();
    _register_Action();
  }
}

/* -- P A R S E R _ S T A C K _ E N T R Y --------------------------------- */

parser_stack_entry_t * _parser_stack_entry_new(parser_stack_entry_t *pse, va_list args) {
  pse -> subject = data_copy(va_arg(args, data_t *));
  pse -> filter = va_arg(args, int);
  return pse;
}

void _parser_stack_entry_free(parser_stack_entry_t *pse) {
  if (pse) {
    data_free(pse -> subject);
  }
}

int _parser_stack_entry_call(parser_stack_entry_t *pse,
    parser_t *parser, token_t *token) {
  pse_execute_t  execute;

  if (parser -> state & pse -> filter) {
    debug(parser, "PSE Call %s", parser_stack_entry_tostring(pse));
    execute = (pse_execute_t) data_get_function((data_t *) pse, FunctionUsr1);
    assert(execute);
    return execute(pse, parser, token);
  } else {
    debug(parser, "PSE Blocked %s", parser_stack_entry_tostring(pse));
    return parser -> state | ParserStateDone;
  }
}

parser_stack_entry_t * _parser_stack_entry_create(int type, data_t *subject, int filter) {
  _parser_init();
  return (parser_stack_entry_t *) data_create(type, subject, filter);
}

parser_stack_entry_t * _parser_stack_entry_for_nonterminal(nonterminal_t *nonterminal) {
  return _parser_stack_entry_create(PSETypeNonTerminal, (data_t *) nonterminal,
    ParserStateNone | ParserStateNonTerminal);
}

parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *rule) {
  return _parser_stack_entry_create(PSETypeRule, (data_t *) rule, ParserStateAll);
}

parser_stack_entry_t * _parser_stack_entry_for_entry(rule_entry_t *entry) {
  return _parser_stack_entry_create(PSETypeEntry, (data_t *) entry, ParserStateAll);
}

parser_stack_entry_t * _parser_stack_entry_for_action(grammar_action_t *action) {
  return _parser_stack_entry_create(PSETypeAction, (data_t *) action, ParserStateAll);
}

/* -- P S E T Y P E  N O N T E R M I N A L -------------------------------- */

char * _pse_allocstring_NonTerminal(parser_stack_entry_t *e) {
  char *buf;

  asprintf(&buf, " N {%s}", data_tostring(e -> subject));
  return buf;
}

int _pse_execute_NonTerminal(parser_stack_entry_t *e, parser_t *parser, token_t *token) {
  int            code = token_code(token);
  nonterminal_t *nonterminal = (nonterminal_t *) e -> subject;
  rule_t        *rule;
  int            i;
  rule_entry_t  *entry;
  nonterminal_t *new_nt;

  if (code == TokenCodeEOF) {
    /*
     * End of the stream. Bail and retry with the next token:
     */
    return parser -> state | ParserStateDone;
  } else if (!(rule = dict_get_int(nonterminal -> parse_table, code))) {
    if (code != TokenCodeEnd) {
      parser -> error = data_exception(ErrorSyntax,
        "Unexpected token '%s'", token_tostring(token));
    } else {
      parser -> error = data_exception(ErrorSyntax,
        "Unexpected end of program text");
    }
    return ParserStateError;
  } else {
    debug(parser, "Selected rule: %s", rule_tostring(rule));
    // _parser_push_to_prodstack(parser,
    //                           _parser_stack_entry_for_rule(rule));
    for (i = array_size(rule -> entries) - 1; i >= 0; i--) {
      entry = rule_get_entry(rule, i);
      if (entry -> terminal) {
        _parser_push_grammar_element(parser, (ge_t *) entry);
        _parser_push_to_prodstack(
            parser, _parser_stack_entry_for_entry(entry));
      } else {
        _parser_push_grammar_element(parser, (ge_t *) entry);
        new_nt = grammar_get_nonterminal(parser -> grammar,
                                         entry -> nonterminal);
        _parser_push_to_prodstack(parser,
                                  _parser_stack_entry_for_nonterminal(new_nt));
        _parser_push_grammar_element(parser, (ge_t *) new_nt);
      }
    }
    _parser_push_grammar_element(parser, (ge_t *) rule);
  }
  return parser -> state | ParserStateNonTerminal;
}

/* -- P S E T Y P E  R U L E ---------------------------------------------- */

char * _pse_allocstring_Rule(parser_stack_entry_t *e) {
  char *buf;

  asprintf(&buf, " R {%s}", data_tostring(e -> subject));
  return buf;
}

int _pse_execute_Rule(parser_stack_entry_t *e, parser_t *parser, token_t *token) {
  return parser -> state | ParserStateRule;
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

int _pse_execute_Entry(parser_stack_entry_t *e, parser_t *parser, token_t *token) {
  rule_entry_t *entry = (rule_entry_t *) e -> subject;

  assert(entry -> terminal);
  if (parser -> state & ParserStateTerminal) {
    return ParserStateDone;
  } else if (token_cmp(entry -> token, token)) {
    parser -> error = data_exception(
            ErrorSyntax, "Expected '%s' but got '%s' instead",
            token_tostring(entry -> token), token_tostring(token));
  }
  return (parser -> state | ParserStateTerminal) & ~ParserStateNonTerminal;
}

/* -- P S E T Y P E  A C T I O N ------------------------------------------ */

char * _pse_allocstring_Action(parser_stack_entry_t *e) {
  char *buf;

  asprintf(&buf, " A {%s}", data_tostring(e -> subject));
  return buf;
}

int _pse_execute_Action(parser_stack_entry_t *e, parser_t *parser, token_t *token) {
  parser_t         *ret;
  grammar_action_t *action = (grammar_action_t *) e -> subject;

  assert(action && action -> fnc && action -> fnc -> fnc);
  debug(parser, "Action '%s'", data_tostring(e -> subject));
  if (action -> data) {
    ret = ((parser_data_fnc_t) action -> fnc -> fnc)(parser, action -> data);
  } else {
    ret = ((parser_fnc_t) action -> fnc -> fnc)(parser);
  }
  if (!ret) {
    parser -> error = data_exception(
            ErrorSyntax, "Error executing grammar action %s",
            grammar_action_tostring(action));
  }
  return parser -> state;
}

/* -- P A R S E R  D A T A  F U N C T I O N S ----------------------------- */

parser_t * _parser_new(parser_t *parser, va_list args) {
  parser -> grammar = grammar_copy(va_arg(args, grammar_t *));
  parser -> lexer = NULL;
  parser -> prod_stack = list_create();
  parser -> last_token = NULL;
  parser -> error = NULL;
  parser -> stack = datastack_create("__parser__");
  parser -> variables = strdata_dict_create();
  datastack_set_debug(parser -> stack, parser_debug);
  return parser;
}

void _parser_free(parser_t *parser) {
  if (parser) {
    token_free(parser -> last_token);
    data_free(parser -> error);
    list_free(parser -> prod_stack);
    datastack_free(parser -> stack);
    dict_free(parser -> variables);
  }
}

char * _parser_allocstring(parser_t *parser) {
  char *buf;

  if (parser -> lexer && parser -> lexer -> reader) {
    asprintf(&buf, "Parser for '%s'", data_tostring(parser -> lexer -> reader));
  } else {
    asprintf(&buf, "Parser for '%s'", grammar_tostring(parser -> grammar));
  }
  return buf;
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
    return (data_t *) lexer_copy(parser -> lexer);
  } else if (!strcmp(name, "grammar")) {
    return (data_t *) grammar_copy(parser -> grammar);
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

parser_t * _parser_dump_prod_stack(parser_t *parser) {
  parser_stack_entry_t *entry;

  if (parser -> last_token) {
      _debug("== Last Token: %-35.35sLine %d Column %d",
          token_tostring(parser -> last_token),
          parser -> last_token -> line,
          parser -> last_token -> column);
  }
  _debug("== Production Stack ==========================================================");
  for (list_start(parser -> prod_stack); list_has_next(parser -> prod_stack); ) {
    entry = (parser_stack_entry_t *) list_next(parser -> prod_stack);
    _debug("[ %-32.32s ]", data_tostring((data_t *) entry));
  }
  return parser;
}

parser_t * _parser_ll1(token_t *token, parser_t *parser) {
  int  attempts = 0;
  char buf[128];

  if (parser -> last_token) {
    token_free(parser -> last_token);
    parser -> last_token = NULL;
  }
  parser -> last_token = token_copy(token);
  parser -> state = ParserStateNone;
  do {
    if (parser_debug) {
      debug(parser, "Processing token '%s'. state = %s",
        token_tostring(token),
        labels_for_bitmap(_parser_states, parser -> state, buf, 127));
      _parser_dump_prod_stack(parser);
    }
    _parser_ll1_token_handler(token, parser, attempts++);
  } while (!parser -> error && (parser -> state < ParserStateDone));
  if (parser_debug) {
    debug(parser, "Processed token '%s'. state = %s, error = '%s'",
      token_tostring(token),
      labels_for_bitmap(_parser_states, parser -> state, buf, 127),
      data_tostring(parser -> error));
  }
  return (parser -> error || (parser -> state & ParserStateError)) ? NULL : parser;
}

parser_t * _parser_push_to_prodstack(parser_t *parser, parser_stack_entry_t *entry) {
  list_push(parser -> prod_stack, entry);
  debug(parser, "      Pushed  %s", parser_stack_entry_tostring(entry));
  return parser;
}

parser_t * _parser_push_grammar_element(parser_t *parser, ge_t *element) {
  grammar_action_t *action;

  for (list_end(element -> actions); list_has_prev(element -> actions); ) {
    action = list_prev(element -> actions);
    _parser_push_to_prodstack(parser, _parser_stack_entry_for_action(action));
  }
  return parser;
}

int _parser_ll1_token_handler(token_t *token, parser_t *parser, int attempts) {
  parser_stack_entry_t *e;
  int                   code;

  code = token_code(token);
  e = list_pop(parser -> prod_stack);
  if (e == NULL) {
    debug(parser, "Parser stack exhausted");
    /*
     * FIXME (or maybe not :-) If the parse ends with an Action, there is nothing
     * to pop afterwards. Therefore: if the stack is empty and we're trying to
     * handle the same action more than once, just bail.
     */
    if ((code != TokenCodeEnd) && !attempts) {
      parser -> error = data_exception(ErrorSyntax,
				       "Expected end of text, read unexpected token '%s'",
				       token_tostring(token));
    }
    parser -> state |= ParserStateError;
  } else {
    debug(parser, "    Popped  %s", data_tostring((data_t *) e));
    parser -> state = _parser_stack_entry_call(e, parser, token);
    parser -> state &= ~(ParserStateNone); /* Clear None bit */
    if (parser -> state & ParserStateDone) {
      debug(parser, "  Re-pushing  %s", data_tostring((data_t *) e));
      list_push(parser -> prod_stack, e);
    } else {
      data_free((data_t *) e);
    }
  }
  return parser -> state;
}

/* -- parser_t public functions ------------------------------------------- */

parser_t * parser_create(grammar_t *grammar) {
  _parser_init();
  return (parser_t *) data_create(Parser, grammar);
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
  parser -> last_token = NULL;
  data_free(parser -> error);
  parser -> error = NULL;
  lexer_free(parser -> lexer);
  parser -> lexer = NULL;
  return parser;
}

parser_t * parser_start(parser_t *parser) {
  parser_clear(parser);
  list_append(parser -> prod_stack,
              _parser_stack_entry_for_nonterminal(parser -> grammar -> entrypoint));
  return parser;
}

data_t * parser_parse(parser_t *parser, data_t *reader) {
  data_t  *ret = NULL;

  parser_start(parser);
  ret = parser_parse_reader(parser, reader);
  if (!ret) {
    ret = parser_end(parser);
  }
  parser_clear(parser);
  return ret;
}

data_t * parser_parse_reader(parser_t *parser, data_t *reader) {
  data_t *ret = NULL;

  debug(parser, "Parsing reader '%s'.", data_tostring(reader))
  parser -> lexer = lexer_create(parser -> grammar -> lexer, reader);
  parser -> lexer -> data = parser;
  lexer_tokenize(parser -> lexer, _parser_ll1, parser);
  ret = parser -> error;
  parser -> error = NULL;
  debug(parser, "Parsed reader '%s'. Result: '%s'",
    data_tostring(reader), data_tostring(ret))
  if (list_notempty(parser -> prod_stack) && parser_debug) {
    _parser_dump_prod_stack(parser);
  }
  if (parser_debug && datastack_notempty(parser -> stack)) {
    datastack_list(parser -> stack);
  }
  return ret;
}

data_t * parser_send_token(parser_t *parser, token_t *token) {
  data_t *ret;

  _parser_ll1(token, parser);
  ret = parser -> error;
  parser -> error = NULL;
  debug(parser, "Parsed token '%s'. Result: '%s'",
    token_tostring(token), data_tostring(ret))
  if (list_notempty(parser -> prod_stack) && parser_debug) {
    _parser_dump_prod_stack(parser);
  }
  if (parser_debug && datastack_notempty(parser -> stack)) {
    datastack_list(parser -> stack);
  }
  return ret;
}

data_t * parser_end(parser_t *parser) {
  data_t *ret;

  if (!token_end) {
    token_end = token_create(TokenCodeEnd, "$$");
  }
  ret = parser_send_token(parser, token_end);
  return ret;
}
