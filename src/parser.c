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
    function_t    *fnc;
  };
} parser_stack_entry_t;

static parser_stack_entry_t * _parser_stack_entry_for_rule(rule_t *);
static parser_stack_entry_t * _parser_stack_entry_for_option(rule_option_t *);
static parser_stack_entry_t * _parser_stack_entry_for_item(rule_item_t *);
static parser_stack_entry_t * _parser_stack_entry_for_function(function_t *fnc);
static void                   _parser_stack_entry_free(parser_stack_entry_t *);
static char *                 _parser_stack_entry_tostring(parser_stack_entry_t *, char *, int);

static int                    _parser_ll1_token_handler(token_t *, parser_t *, int);
static parser_t *             _parser_ll1(token_t *, parser_t *);
static parser_t *             _parser_lr1(token_t *, parser_t *);
static lexer_t *              _parser_set_keywords(token_t *, lexer_t *);

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

parser_stack_entry_t * _parser_stack_entry_for_function(function_t *function) {
  parser_stack_entry_t *ret;

  ret = NEW(parser_stack_entry_t);
  ret -> type = PSETypeFunction;
  ret -> fnc = function;
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
        strcpy(buf, "I ");
        token_tostring(item -> token, buf + 2, maxlen - 2);
      } else {
        snprintf(buf, maxlen, "T {%s}", item -> rule);
      }
      break;
    case PSETypeFunction:
      snprintf(buf, maxlen, "F %s", function_tostring(entry -> fnc));
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
          token_tostring(parser -> last_token, NULL, 0),
          parser -> last_token -> line,
          parser -> last_token -> column);
    debug("Parser Stack =====================================================");
    for(iter = li_create(parser -> prod_stack); li_has_next(iter); ) {
      entry = li_next(iter);
      debug("[ %-32.32s ]", _parser_stack_entry_tostring(entry, NULL, 0));
    }
    li_free(iter);
  }
  consuming = 2;
  do {
    consuming = _parser_ll1_token_handler(token, parser, consuming);
  } while (consuming);
  return parser;
}

int _parser_ll1_token_handler(token_t *token, parser_t *parser, int consuming) {
  rule_t               *rule;
  rule_t               *new_rule;
  rule_option_t        *option;
  rule_item_t          *item;
  parser_stack_entry_t *entry;
  parser_stack_entry_t *new_entry;
  int                   code;
  int                   i;

  code = token_code(token);
  entry = list_pop(parser -> prod_stack);
  if (entry == NULL) {
    if (parser_debug) {
      debug("Parser stack exhausted");
    }
    assert(code == TokenCodeEnd);
    consuming = 0;
    // TODO: Error Handling.
    // If assert trips we've reached what should have been the
    // end of the input but we're getting more stuff.
  } else if ((consuming == 1) && (entry -> type != PSETypeFunction)) {
      list_push(parser -> prod_stack, entry);
      consuming = 0;
  } else {
    if (parser_debug) {
      debug("    Popped  %s", _parser_stack_entry_tostring(entry, NULL, 0));
    }
    switch (entry -> type) {
      case PSETypeRule:
        rule = entry -> rule;
        option = dict_get_int(rule -> parse_table, code);
        assert(option);
        // TODO: Error Handling.
        // If assert trips we've received a terminal that
        // doesn't match any production for the rule.
        // This is what normal people call a syntax error :-)
        if (array_size(option -> items)) {
          for (i = array_size(option -> items) - 1; i >= 0; i--) {
            item = (rule_item_t *) array_get(option -> items, i);
            if (item -> terminal) {
              new_entry = _parser_stack_entry_for_item(item);
              list_push(parser -> prod_stack, new_entry);
              if (parser_debug) {
                debug("    Pushed   %s", _parser_stack_entry_tostring(new_entry, NULL, 0));
              }
            } else {
              new_rule = grammar_get_rule(parser -> grammar, item -> rule);
              if (rule_item_get_finalizer(item)) {
                new_entry = _parser_stack_entry_for_function(rule_item_get_finalizer(item));
                list_push(parser -> prod_stack, new_entry);
                if (parser_debug) {
                  debug("    Pushed   %s", _parser_stack_entry_tostring(new_entry, NULL, 0));
                }
              }
              if (rule_get_finalizer(new_rule)) {
                new_entry = _parser_stack_entry_for_function(rule_get_finalizer(new_rule));
                list_push(parser -> prod_stack, new_entry);
                if (parser_debug) {
                  debug("    Pushed   %s", _parser_stack_entry_tostring(new_entry, NULL, 0));
                }
              }
              new_entry = _parser_stack_entry_for_rule(new_rule);
              list_push(parser -> prod_stack, new_entry);
              if (parser_debug) {
                debug("    Pushed   %s", _parser_stack_entry_tostring(new_entry, NULL, 0));
              }
              if (rule_get_initializer(new_rule)) {
                new_entry = _parser_stack_entry_for_function(rule_get_initializer(new_rule));
                list_push(parser -> prod_stack, new_entry);
                if (parser_debug) {
                  debug("    Pushed   %s", _parser_stack_entry_tostring(new_entry, NULL, 0));
                }
              }
              if (rule_item_get_initializer(item)) {
                new_entry = _parser_stack_entry_for_function(rule_item_get_initializer(item));
                list_push(parser -> prod_stack, new_entry);
                if (parser_debug) {
                  debug("    Pushed   %s", _parser_stack_entry_tostring(new_entry, NULL, 0));
                }
              }
            }
          }
        }
        consuming = 2;
        break;
      case PSETypeItem:
        item = entry -> item;
        assert(item -> terminal);
	assert(!token_cmp(item -> token, token));
	// TODO: Error Handling.
	// If assert trips we've received a terminal that has a different
	// code than the one we're expecting. Syntax error.
	
	if (rule_item_get_finalizer(item)) {
	  if (parser_debug) {
            debug("    Executing terminal item finalizer %s",
                  function_tostring(rule_item_get_finalizer(item)));
	  }
	  rule_item_get_finalizer(item) -> fnc(parser);
	}
	consuming = 1;
        break;
      case PSETypeFunction:
        assert(entry -> fnc -> fnc);
        if (parser_debug) {
          debug("    Executing non-terminal function %s",
                function_tostring(entry -> fnc));
        }
        entry -> fnc -> fnc(parser);
        break;
    }
    _parser_stack_entry_free(entry);
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
  ret -> stack = list_create();
  list_set_free(ret -> stack, (free_t) data_free);
  list_set_tostring(ret -> stack, (tostring_t) data_tostring);
  return ret;
}

void _parser_parse(parser_t *parser, reader_t *reader) {
  lexer_t        *lexer;
  lexer_option_t  ix;
  str_t          *debugstr;

  lexer = lexer_create(reader);
  dict_reduce_values(parser -> grammar -> keywords, (reduce_t) _parser_set_keywords, lexer);
  for (ix = 0; ix < LexerOptionLAST; ix++) {
    lexer_set_option(lexer, ix, grammar_get_option(parser -> grammar, ix));
  }
  list_clear(parser -> stack);
  if (grammar_get_initializer(parser -> grammar)) {
    grammar_get_initializer(parser -> grammar) -> fnc(parser);
  }
  switch (grammar_get_parsing_strategy(parser -> grammar)) {
    case ParsingStrategyTopDown:
      list_append(parser -> prod_stack,
                  _parser_stack_entry_for_rule(parser -> grammar -> entrypoint));
      lexer_tokenize(lexer, _parser_ll1, parser);
      break;
    case ParsingStrategyBottomUp:
      lexer_tokenize(lexer, _parser_lr1, parser);
      break;
  }
  if (grammar_get_finalizer(parser -> grammar)) {
    grammar_get_finalizer(parser -> grammar) -> fnc(parser);
  }
  if (list_notempty(parser -> stack)) {
    debugstr = list_tostr(parser -> stack);
    error("Parser stack not empty after parse!\n%s", str_chars(debugstr));
    str_free(debugstr);
    list_clear(parser -> stack);
  }
  lexer_free(lexer);
}

void parser_free(parser_t *parser) {
  if (parser) {
    token_free(parser -> last_token);
    list_free(parser -> prod_stack);
    list_free(parser -> stack);
    free(parser);
  }
}
