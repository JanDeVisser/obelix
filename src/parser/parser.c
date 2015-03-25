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
#include <parser.h>

int parser_debug = 0;

typedef enum {
  PSETypeNonTerminal,
  PSETypeRule,
  PSETypeEntry,
  PSETypeAction
} parser_stack_entry_type_t;

typedef struct _parser_stack_entry {
  parser_stack_entry_type_t type;
  union {
    nonterminal_t    *nonterminal;
    rule_t           *rule;
    rule_entry_t     *entry;
    grammar_action_t *action;
  };
  char               *str;
} parser_stack_entry_t;

static void                   _parser_init(void) __attribute__((constructor(102)));

static parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *);
static parser_stack_entry_t * _parser_stack_entry_for_nonterminal(nonterminal_t *);
static parser_stack_entry_t * _parser_stack_entry_for_entry(rule_entry_t *);
static parser_stack_entry_t * _parser_stack_entry_for_action(grammar_action_t *);
static void                   _parser_stack_entry_free(parser_stack_entry_t *);
static char *                 _parser_stack_entry_tostring(parser_stack_entry_t *);

static parser_t *             _parser_push_setvalues(entry_t *, parser_t *);
static int                    _parser_ll1_token_handler(token_t *, parser_t *, int);
static parser_t *             _parser_ll1(token_t *, parser_t *);
static parser_t *             _parser_lr1(token_t *, parser_t *);
static lexer_t *              _parser_set_keywords(token_t *, lexer_t *);

void _parser_init(void) {
  logging_register_category("parser", &parser_debug);
}

/*
 * parser_stack_entry_t static functions
 */

parser_stack_entry_t * _parser_stack_entry_for_nonterminal(nonterminal_t *nonterminal) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeNonTerminal;
  ret -> nonterminal = nonterminal;
  ret -> str = NULL;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *rule) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeRule;
  ret -> rule = rule;
  ret -> str = NULL;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_entry(rule_entry_t *entry) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeEntry;
  ret -> entry = entry;
  ret -> str = NULL;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_action(grammar_action_t *action) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeAction;
  ret -> action = action;
  ret -> str = NULL;
  return ret;
}

void _parser_stack_entry_free(parser_stack_entry_t *entry) {
  if (entry) {
    free(entry -> str);
    free(entry);
  }
}

char * _parser_stack_entry_tostring(parser_stack_entry_t *e) {
  nonterminal_t *nonterminal;
  rule_entry_t  *entry;

  if (!e -> str) {
    switch (e -> type) {
      case PSETypeNonTerminal:
        asprintf(&e -> str, "N {%s}", e -> nonterminal -> name);
        break;
      case PSETypeEntry:
        entry = e -> entry;
        if (entry -> terminal) {
          asprintf(&e -> str, "T {%s}", token_tostring(entry -> token));
        } else {
          asprintf(&e -> str, "N {%s}", entry -> nonterminal);
        }
        break;
      case PSETypeAction:
        asprintf(&e -> str, "A %s", grammar_action_tostring(e -> action));
        break;
    }
  }
  return e -> str;
}

/*
 * parser_t static functions
 */


parser_t * _parser_lr1(token_t *token, parser_t *parser) {
  error("Bottom-up parsing is not yet implemented");
  assert(0);
  return parser;
}

parser_t * _parser_ll1(token_t *token, parser_t *parser) {
  parser_stack_entry_t *entry;
  int                   consuming;
  listiterator_t       *iter;

  if (parser -> last_token) {
    token_free(parser -> last_token);
    parser -> last_token = NULL;
  }
  parser -> last_token = token_copy(token);
  if (parser_debug) {
    debug("== Last Token: %s    Line %d Column %d",
          token_tostring(parser -> last_token),
          parser -> last_token -> line,
          parser -> last_token -> column);
    debug("Production Stack =================================================");
    for(iter = li_create(parser -> prod_stack); li_has_next(iter); ) {
      entry = li_next(iter);
      debug("[ %-32.32s ]", _parser_stack_entry_tostring(entry));
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
    debug("      Pushed  %s", _parser_stack_entry_tostring(entry));
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
  nonterminal_t        *nonterminal;
  nonterminal_t        *new_nt;
  rule_t               *rule;
  rule_entry_t         *entry;
  parser_stack_entry_t *e;
  parser_stack_entry_t *ne;
  int                   code;
  int                   i;
  data_t               *data;
  data_t               *counter;
  token_t              *pushtoken;
  parser_t             *ret;

  code = token_code(token);
  parser -> line = token -> line;
  parser -> column = token -> column;
  e = list_pop(parser -> prod_stack);
  if (e == NULL) {
    if (parser_debug) {
      debug("Parser stack exhausted");
    }
    assert(code == TokenCodeEnd);
    consuming = 0;
    // TODO: Error Handling.
    // If assert trips we've reached what should have been the
    // end of the input but we're getting more stuff.
  } else if ((consuming == 1) && (e -> type != PSETypeAction)) {
    list_push(parser -> prod_stack, e);
    consuming = 0;
  } else {
    if (parser_debug) {
      debug("    Popped  %s", _parser_stack_entry_tostring(e));
    }
    
    switch (e -> type) {
      case PSETypeNonTerminal:
        nonterminal = e -> nonterminal;
        rule = dict_get_int(nonterminal -> parse_table, code);
	if (!rule) {
	  parser -> error = data_error(ErrorSyntax, 
				       "Unexpected token '%s'", 
				       token_token(token));
	  break;
	}
        if (array_size(rule -> entries)) {
          for (i = array_size(rule -> entries) - 1; i >= 0; i--) {
            entry = rule_get_entry(rule, i);
            if (entry -> terminal) {
              _parser_push_grammar_element(parser, entry -> ge);
              _parser_push_to_prodstack(
                  parser, _parser_stack_entry_for_entry(entry));
            } else {
              _parser_push_grammar_element(parser, entry -> ge);
              new_nt = grammar_get_nonterminal(parser -> grammar, entry -> nonterminal);
              _parser_push_to_prodstack(
                  parser,
                  _parser_stack_entry_for_nonterminal(new_nt));
              _parser_push_grammar_element(parser, new_nt -> ge);
            }
          }
        }
        _parser_push_grammar_element(parser, rule -> ge);
        consuming = 2;
        break;
        
      case PSETypeEntry:
        entry = e -> entry;
        assert(entry -> terminal);
	if (token_cmp(entry -> token, token)) {
	  parser -> error = data_error(ErrorSyntax,
				       "Expected '%s' but got '%s' instead",
				       token_token(entry -> token),
				       token_token(token));
	}
	consuming = 1;
        break;
        
      case PSETypeAction:
        assert(e -> action && e -> action -> fnc && e -> action -> fnc -> fnc);
        if (parser_debug) {
          debug("    Executing action %s", grammar_action_tostring(e -> action));
        }
        if (e -> action -> data) {
          ret = ((parser_data_fnc_t) e -> action -> fnc -> fnc)(parser, e -> action -> data);
        } else {
          ret = ((parser_fnc_t) e -> action -> fnc -> fnc)(parser);
        }
        if (!ret) {
	  parser -> error = data_error(ErrorSyntax,
				       "Error executing grammar action %s",
				       grammar_action_tostring(e -> action));
        }
        break;
    }        
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

  ret = NEW(parser_t);
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


data_t * _parser_parse(parser_t *parser, reader_t *reader) {
  lexer_t        *lexer;
  lexer_option_t  ix;

  token_free(parser -> last_token);
  parser -> last_token = NULL;
  lexer = lexer_create(reader);
  dict_reduce_values(parser -> grammar -> keywords, (reduce_t) _parser_set_keywords, lexer);
  for (ix = 0; ix < LexerOptionLAST; ix++) {
    lexer_set_option(lexer, ix, grammar_get_lexer_option(parser -> grammar, ix));
  }
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
  lexer_free(lexer);
  return data_copy(parser -> error);
}

void parser_free(parser_t *parser) {
  if (parser) {
    token_free(parser -> last_token);
    data_free(parser -> error);
    list_free(parser -> prod_stack);
    datastack_free(parser -> stack);
    dict_free(parser -> variables);
    free(parser);
  }
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

parser_t * parser_pushval(parser_t *parser, data_t *data) {
  if (parser_debug) {
    debug("    Pushing value %s", data_debugstr(data));
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
    debug("    Discarding value %s", data_debugstr(data));
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
