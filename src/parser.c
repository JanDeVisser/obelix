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
#include <lexer.h>
#include <parser.h>

int parser_debug = 0;

typedef enum {
  PSETypeNonTerminal,
  PSETypeRule,
  PSETypeEntry,
  PSETypeFunction,
  PSETypePush,
  PSETypeSet,
  PSETypeIncr
} parser_stack_entry_type_t;

typedef struct _parser_stack_entry {
  parser_stack_entry_type_t type;
  char        *name;
  union {
    nonterminal_t *nonterminal;
    rule_t        *rule;
    rule_entry_t  *entry;
    function_t    *fnc;
    data_t        *value;
  };
} parser_stack_entry_t;

static parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *);
static parser_stack_entry_t * _parser_stack_entry_for_nonterminal(nonterminal_t *);
static parser_stack_entry_t * _parser_stack_entry_for_entry(rule_entry_t *);
static parser_stack_entry_t * _parser_stack_entry_for_function(function_t *);
static parser_stack_entry_t * _parser_stack_entry_for_pushvalue(token_t *);
static parser_stack_entry_t * _parser_stack_entry_for_setvalue(char *, token_t *);
static parser_stack_entry_t * _parser_stack_entry_for_incrvalue(char *);
static void                   _parser_stack_entry_free(parser_stack_entry_t *);
static char *                 _parser_stack_entry_tostring(parser_stack_entry_t *);

static parser_t *             _parser_push_setvalues(entry_t *, parser_t *);
static int                    _parser_ll1_token_handler(token_t *, parser_t *, int);
static parser_t *             _parser_ll1(token_t *, parser_t *);
static parser_t *             _parser_lr1(token_t *, parser_t *);
static lexer_t *              _parser_set_keywords(token_t *, lexer_t *);

/*
 * parser_stack_entry_t static functions
 */

parser_stack_entry_t * _parser_stack_entry_for_nonterminal(nonterminal_t *nonterminal) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeNonTerminal;
  ret -> nonterminal = nonterminal;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *rule) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeRule;
  ret -> rule = rule;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_entry(rule_entry_t *entry) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeEntry;
  ret -> entry = entry;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_function(function_t *function) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeFunction;
  ret -> fnc = function_copy(function);
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_pushvalue(token_t *token) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypePush;
  ret -> value = (token_code(token) != TokenCodeDollar) ? token_todata(token) : NULL;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_setvalue(char *name, token_t *token) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeSet;
  ret -> name = strdup(name);
  ret -> value = (token_code(token) != TokenCodeDollar) ? token_todata(token) : NULL;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_incrvalue(char *name) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeIncr;
  ret -> name = strdup(name);
  return ret;
}

void _parser_stack_entry_free(parser_stack_entry_t *entry) {
  if (entry) {
    switch (entry -> type) {
      case PSETypeIncr:
        free(entry -> name);
        break;
      case PSETypeSet:
        free(entry -> name);
        /* no break */
      case PSETypePush:
        data_free(entry -> value);
        break;
      case PSETypeFunction:
        function_free(entry -> fnc);
        break;
    }
    free(entry);
  }
}

char * _parser_stack_entry_tostring(parser_stack_entry_t *e) {
  nonterminal_t *nonterminal;
  rule_entry_t  *entry;
  int            maxlen;
  static char    buf[256];

  maxlen = 256;
  switch (e -> type) {
    case PSETypeNonTerminal:
      snprintf(buf, maxlen, "N {%s}", e -> nonterminal -> name);
      break;
    case PSETypeEntry:
      entry = e -> entry;
      if (entry -> terminal) {
        strcpy(buf, "T ");
        token_tostring(entry -> token);
      } else {
        snprintf(buf, maxlen, "N {%s}", entry -> nonterminal);
      }
      break;
    case PSETypeFunction:
      snprintf(buf, maxlen, "F %s", function_tostring(e -> fnc));
      buf[maxlen - 1] = 0;
      break;
    case PSETypePush:
      snprintf(buf, maxlen, "P %s", data_tostring(e -> value));
      buf[maxlen - 1] = 0;
      break;
    case PSETypeSet:
      snprintf(buf, maxlen, "S %s = %s",
               e -> name, data_tostring(e -> value));
      buf[maxlen - 1] = 0;
      break;
    case PSETypeIncr:
      snprintf(buf, maxlen, "I %s", e -> name);
      buf[maxlen - 1] = 0;
      break;
  }
  return buf;
}

/*
 * parser_t static functions
 */


parser_t * _parser_lr1(token_t *token, parser_t *parser) {
  return parser;
}

parser_t * _parser_ll1(token_t *token, parser_t *parser) {
  parser_stack_entry_t *entry;
  int                   consuming;
  listiterator_t       *iter;

  if (parser -> last_token) {
    token_free(parser -> last_token);
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
  } while (consuming);
  return parser;
}

parser_t * _parser_push_to_prodstack(parser_t *parser, parser_stack_entry_t *entry) {
  list_push(parser -> prod_stack, entry);
  if (parser_debug) {
    debug("    Pushed   %s", _parser_stack_entry_tostring(entry));
  }
  return parser;
}

parser_t * _parser_push_setvalues(entry_t *entry, parser_t *parser) {
  return _parser_push_to_prodstack(
      parser,
      _parser_stack_entry_for_setvalue(entry -> key, entry -> value));
}

parser_t * _parser_push_incrvalues(char *name, parser_t *parser) {
  return _parser_push_to_prodstack(
      parser,
      _parser_stack_entry_for_incrvalue(name));
}

parser_t * _parser_push_pushvalues(token_t *value, parser_t *parser) {
  return _parser_push_to_prodstack(
      parser,
      _parser_stack_entry_for_pushvalue(value));
}

parser_t * _parser_push_grammar_element(parser_t *parser, ge_t *element, int init) {
  parser_stack_entry_t *new_entry;

  if (init) {
    if (ge_get_initializer(element)) {
      _parser_push_to_prodstack(
          parser,
          _parser_stack_entry_for_function(ge_get_initializer(element)));
    }
  } else {
    if (ge_get_finalizer(element)) {
      _parser_push_to_prodstack(
          parser,
          _parser_stack_entry_for_function(ge_get_finalizer(element)));
    }
    dict_reduce(element -> variables, (reduce_t) _parser_push_setvalues, parser);
    set_reduce(element -> incrs, (reduce_t) _parser_push_incrvalues, parser);
    set_reduce(element -> pushvalues, (reduce_t) _parser_push_pushvalues, parser);
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

  code = token_code(token);
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
  } else if ((consuming == 1) &&
      (e -> type != PSETypeFunction) &&
      (e -> type != PSETypeSet) &&
      (e -> type != PSETypeIncr) &&
      (e -> type != PSETypePush)) {
    list_push(parser -> prod_stack, e);
    consuming = 0;
  } else {
    if (parser_debug) {
      debug("    Popped  %s", _parser_stack_entry_tostring(e));
    }
    switch (e -> type) {
      case PSETypeNonTerminal:
        rule = e -> rule;
        rule = dict_get_int(nonterminal -> parse_table, code);
        assert(rule);
        // TODO: Error Handling.
        // If assert trips we've received a terminal that
        // doesn't match any production for the rule.
        // This is what normal people call a syntax error :-)
        if (array_size(rule -> entries)) {
          for (i = array_size(rule -> entries) - 1; i >= 0; i--) {
            entry = (rule_entry_t *) array_get(rule -> entries, i);
            if (entry -> terminal) {
              _parser_push_grammar_element(parser, entry -> ge, 0);
              _parser_push_to_prodstack(
                  parser,
                  _parser_stack_entry_for_entry(entry));
              _parser_push_grammar_element(parser, entry -> ge, 1);
            } else {
              new_nt = grammar_get_nonterminal(parser -> grammar, entry -> nonterminal);
              _parser_push_grammar_element(parser, entry -> ge, 0);
              _parser_push_grammar_element(parser, new_nt -> ge, 0);
              _parser_push_to_prodstack(
                  parser,
                  _parser_stack_entry_for_nonterminal(new_nt));
              _parser_push_grammar_element(parser, new_nt -> ge, 1);
              _parser_push_grammar_element(parser, entry -> ge, 1);
            }
          }
        }
        consuming = 2;
        break;
      case PSETypeEntry:
        entry = e -> entry;
        assert(entry -> terminal);
	assert(!token_cmp(entry -> token, token));
	// TODO: Error Handling.
	// If assert trips we've received a terminal that has a different
	// code than the one we're expecting. Syntax error.

#if 0
        if (rule_item_get_pushvalue(item)) {
          ne = _parser_stack_entry_for_pushvalue(rule_item_get_pushvalue(item));
          list_push(parser -> prod_stack, ne);
          if (parser_debug) {
            debug("    Pushed   %s", _parser_stack_entry_tostring(ne, NULL, 0));
          }
        }
        dict_reduce(entry -> grammar_element -> variables, (reduce_t) _parser_push_setvalues, parser);
	if (grammar_element_get_finalizer(entry -> grammar_element)) {
	  if (parser_debug) {
            debug("    Executing terminal entry finalizer %s",
                  function_tostring(rule_item_get_finalizer(entry)));
	  }
	  rule_item_get_finalizer(entry) -> fnc(parser);
	}
#endif /* 0 */
	consuming = 1;
        break;
      case PSETypeFunction:
        assert(e -> fnc -> fnc);
        if (parser_debug) {
          debug("    Executing non-terminal function %s",
                function_tostring(e -> fnc));
        }
        e -> fnc -> fnc(parser);
        break;
      case PSETypeSet:
        data = e -> value;
        if (!data) {
          data = token_todata(parser -> last_token);
        }
        if (parser_debug) {
          debug("    Setting variable %s to %s", e -> name, data_tostring(data));
        }
        dict_put(parser -> variables, strdup(e -> name), data_copy(data));
        if (!e -> value) {
          data_free(data);
        }
        break;
      case PSETypeIncr:
        data = (dict_t *) dict_get(parser -> variables, e -> name);
        if (data) {
          if (data -> type == Int) {
            data -> intval++;
            if (parser_debug) {
              debug("    Incremented variable %s to %s", e -> name, data_tostring(data));
            }
          } else {
            error("Could not increment parser variable '%s' because it is not an integer", e -> name);
          }
        } else {
          dict_put(parser -> variables, strdup(e -> name), data_create_int(1));
          if (parser_debug) {
            debug("    Initialized variable %s to %s", e -> name, data_tostring(data));
          }
        }
        break;
      case PSETypePush:
        data = e -> value;
        if (!data) {
          data = token_todata(parser -> last_token);
        }
        if (parser_debug) {
          debug("    Pushing value %s", data_tostring(data));
        }
        datastack_push(parser -> stack, data_copy(data));
        if (!e -> value) {
          data_free(data);
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
data_t * parser_pop(parser_t *, char *) {
  return (data_t *) dict_pop(parser -> variables, name);
}


void _parser_parse(parser_t *parser, reader_t *reader) {
  lexer_t        *lexer;
  lexer_option_t  ix;

  lexer = lexer_create(reader);
  dict_reduce_values(parser -> grammar -> keywords, (reduce_t) _parser_set_keywords, lexer);
  for (ix = 0; ix < LexerOptionLAST; ix++) {
    lexer_set_option(lexer, ix, grammar_get_option(parser -> grammar, ix));
  }
  if (grammar_get_initializer(parser -> grammar)) {
    grammar_get_initializer(parser -> grammar) -> fnc(parser);
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
  if (grammar_get_finalizer(parser -> grammar)) {
    grammar_get_finalizer(parser -> grammar) -> fnc(parser);
  }
  if (datastack_notempty(parser -> stack)) {
    error("Parser stack not empty after parse!");
    datastack_list(parser -> stack);
    datastack_clear(parser -> stack);
  }
  lexer_free(lexer);
}

void parser_free(parser_t *parser) {
  if (parser) {
    token_free(parser -> last_token);
    list_free(parser -> prod_stack);
    datastack_free(parser -> stack);
    dict_free(parser -> variables);
    free(parser);
  }
}
