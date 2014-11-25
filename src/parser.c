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

#include <lexer.h>
#include <parser.h>

typedef enum {
  PSETypeRule,
  PSETypeOption,
  PSETypeItem,
  PSETypeFunction
} parser_stack_entry_type_t;

typedef struct _parser_stack_entry {
  parser_stack_entry_type_t type;
  union {
    rule_t        *rule;
    rule_option_t *option;
    rule_item_t   *item;
    parser_fnc_t   fnc;
  };
} parser_stack_entry_t;

static parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *);
static parser_stack_entry_t * _parser_stack_entry_for_option(rule_option_t *);
static parser_stack_entry_t * _parser_stack_entry_for_item(rule_item_t *);
static parser_stack_entry_t * _parser_stack_entry_for_function(voidptr_t);
static void                   _parser_stack_entry_free(parser_stack_entry_t *);
static char *                 _parser_stack_entry_tostring(parser_stack_entry_t *, char *, int);

static void *                 _parser_ll1_token_handler(token_t *, parser_t *);

/*
 * parser_stack_entry_t static functions
 */

parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *rule) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeRule;
  ret -> rule = rule;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_option(rule_option_t *option) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeOption;
  ret -> option = option;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_item(rule_item_t *item) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeItem;
  ret -> item = item;
  return ret;
}

parser_stack_entry_t * _parser_stack_entry_for_function(voidptr_t function) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeFunction;
  ret -> fnc = (parser_fnc_t) function;
  return ret;
}

void _parser_stack_entry_free(parser_stack_entry_t *entry) {
  free(entry);
}

char * _parser_stack_entry_tostring(parser_stack_entry_t *entry, char *buf, int maxlen) {
  rule_t      *rule;
  rule_item_t *item;
  static char  localbuf[128];

  if (!buf) {
    buf = localbuf;
    maxlen = 128;
  }
  switch (entry -> type) {
    case PSETypeRule:
      snprintf(buf, maxlen, "R {%s}", entry -> rule -> name);
      break;
    case PSETypeItem:
      item = entry -> item;
      if (item -> terminal) {
        strcpy(buf, "I {");
        token_tostring(item -> token, buf + 3, maxlen - 4);
        strcat(buf, "}");
      } else {
        snprintf(buf, maxlen, "I {%s}", item -> rule);
      }
      break;
    case PSETypeFunction:
      strncpy(buf, "Function", maxlen - 1);
      buf[maxlen - 1] = 0;
      break;
  }
  return buf;
}

/*
 * parser_t static functions
 */


parser_t * _parser_lr1_token_handler(token_t *token, parser_t *parser) {
  return parser;
}

void * _parser_ll1_token_handler(token_t *token, parser_t *parser) {
  rule_t               *rule;
  rule_t               *new_rule;
  rule_option_t        *option;
  rule_item_t          *item;
  parser_stack_entry_t *entry;
  parser_stack_entry_t *new_entry;
  int                   code;
  int                   i;
  listiterator_t       *iter;
  static int            count = 0;

  if (count > 20) {
    assert(0);
  }
  debug("_parser_ll1_token_handler - Parser Stack");
  for(iter = li_create(parser -> prod_stack); li_has_next(iter); ) {
    entry = li_next(iter);
    debug("[ %s ]", _parser_stack_entry_tostring(entry, NULL, 0));
  }
  li_free(iter);
  code = token_code(token);
  entry = list_pop(parser -> prod_stack);
  if (entry == NULL) {
    debug("Parser stack exhausted");
    assert(code == TokenCodeEnd);
    // TODO: Error Handling.
    // If assert trips we've reached what should have been the
    // end of the input but we're getting more stuff.
  } else {
    debug("_parser_ll1_token_handler: Token: %s Stack Entry: %s",
          token_tostring(token, NULL, 0),
          _parser_stack_entry_tostring(entry, NULL, 0));
    if (parser -> last_token) {
      token_free(parser -> last_token);
    }
    parser -> last_token = token_copy(token);
    switch (entry -> type) {
      case PSETypeRule:
        rule = entry -> rule;
        option = dict_get_int(rule -> parse_table, code);
        // TODO: Error Handling.
        // If assert trips we've received a terminal that
        // doesn't match any production for the rule.
        // This is what normal people call a syntax error :-)
        for (i = array_size(option -> items) - 1; i >= 0; i--) {
          item = (rule_item_t *) array_get(option -> items, i);
          if (item -> terminal) {
            debug("Pushing terminal '%s' onto parser stack",
                  token_tostring(item -> token, NULL, 0));
            list_push(parser -> prod_stack,
                      _parser_stack_entry_for_item(item));
          } else {
	    debug("Pushing non-terminal '%s' onto parser stack", item -> rule);
	    if (rule_item_get_finalizer(item)) {
	      debug("Pushing non-terminal item finalizer onto parser stack");
	      list_push(parser -> prod_stack,
			_parser_stack_entry_for_function(rule_item_get_finalizer(item)));
	    }
	    new_rule = grammar_get_rule(parser -> grammar, item -> rule);
	    if (rule_get_finalizer(new_rule)) {
	      debug("Pushing rule finalizer onto parser stack");
	      list_push(parser -> prod_stack,
			_parser_stack_entry_for_function(rule_get_finalizer(new_rule)));
	    }
	    new_entry = _parser_stack_entry_for_rule(new_rule);
	    list_push(parser -> prod_stack, new_entry);
	    if (rule_get_initializer(new_rule)) {
	      debug("Pushing rule initializer onto parser stack");
	      list_push(parser -> prod_stack,
			_parser_stack_entry_for_function(rule_get_initializer(new_rule)));
	    }
	    if (rule_item_get_initializer(item)) {
	      debug("Pushing non-terminal item initializer onto parser stack");
	      list_push(parser -> prod_stack,
			_parser_stack_entry_for_function(rule_item_get_initializer(item)));
	    }
          }
        }
        _parser_ll1_token_handler(token, parser);
        break;
      case PSETypeItem:
        item = entry -> item;
        assert(item -> terminal);
	assert(!token_cmp(item -> token, token));
	debug("Encountered terminal '%s'", token_code_name(code));
	// TODO: Error Handling.
	// If assert trips we've received a terminal that has a different
	// code than the one we're expecting. Syntax error.
	
	if (rule_item_get_finalizer(item)) {
	  debug("Executing terminal finalizer");
	  rule_item_get_finalizer(item)(parser);
	}
        break;
      case PSETypeFunction:
	debug("Executing function");
        assert(entry -> fnc);
        entry -> fnc(parser);
        _parser_ll1_token_handler(token, parser);
        break;
    }
    _parser_stack_entry_free(entry);
  }
  return parser;
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
  return ret;
}

parser_t * _parser_read_grammar(reader_t *reader) {
  parser_t  *ret;
  grammar_t *grammar;

  grammar = grammar_read(reader);
  if (grammar) {
    ret = parser_create(grammar);
    if (!ret) {
      grammar_free(grammar);
    }
  }
  return ret;
}

lexer_t * _parser_set_keywords(token_t *token, lexer_t *lexer) {
  lexer_add_keyword(lexer, token_code(token), token_token(token));
  return lexer;
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
    grammar_get_initializer(parser -> grammar)(parser);
  }
  switch (grammar_get_parsing_strategy(parser -> grammar)) {
    case ParsingStrategyTopDown:
      list_append(parser -> prod_stack,
                  _parser_stack_entry_for_rule(parser -> grammar -> entrypoint));
      lexer_tokenize(lexer, _parser_ll1_token_handler, parser);
      break;
    case ParsingStrategyBottomUp:
      lexer_tokenize(lexer, _parser_lr1_token_handler, parser);
      break;
  }
  if (grammar_get_finalizer(parser -> grammar)) {
    grammar_get_finalizer(parser -> grammar)(parser);
  }
  lexer_free(lexer);
}

void parser_free(parser_t *parser) {
  if (parser) {
    list_free(parser -> prod_stack);
    grammar_free(parser -> grammar);
    free(parser);
  }
}
