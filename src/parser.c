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

#include <array.h>
#include <dict.h>
#include <expr.h>
#include <lexer.h>
#include <list.h>
#include <parser.h>
#include <set.h>

typedef struct _grammar_parser {
  grammar_t     *grammar;
  rule_t        *rule;
  rule_option_t *option;
  rule_item_t   *item;
} grammar_parser_t;

static rule_item_t *   _rule_item_create(rule_option_t *, int);
static void            _grammar_dump_rule(entry_t *);
static void *          _parser_token_handler(token_t *, parser_t *);

/*
 * rule_t public functions
 */

rule_t * rule_create(grammar_t *grammar, char *name) {
  rule_t *ret;

  ret = NEW(rule_t);
  if (ret) {
    ret -> name = strdup(name);
    if (ret -> name) {
      ret -> options = array_create(2);
      if (!ret -> options) {
        free(ret -> name);
        ret -> name = NULL;
      } else {
        ret -> state = strhash(name);
        array_set_free(ret -> options, (free_t) rule_option_free);
        dict_put(grammar -> rules, ret -> name, ret);
        if (!grammar -> entrypoint) {
          grammar -> entrypoint = ret;
        }
        ret -> grammar = grammar;
      }
    }
    if (!ret -> name) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

void rule_free(rule_t *rule) {
  if (rule) {
    free(rule -> name);
    array_free(rule -> options);
    free(rule);
  }
}

void rule_dump(rule_t *rule) {
  fprintf(stderr, "%s%s :=", (rule -> grammar -> entrypoint == rule) ? "(*) " : "", rule -> name);
  array_visit(rule -> options, (visit_t) rule_option_dump);
  fprintf(stderr, "\b;\n\n");
}

/*
 * rule_option_t public functions
 */

rule_option_t * rule_option_create(rule_t *rule) {
  rule_option_t *ret;

  ret = NEW(rule_option_t);
  if (ret) {
    ret -> items = array_create(3);
    if (ret -> items) {
      ret -> rule = rule;
      array_set_free(ret -> items, (free_t) rule_item_free);
      if (!array_push(rule -> options, ret)) {
        array_free(ret -> items);
        ret -> items = NULL;
      }
    }
    if (!ret -> items) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

void rule_option_free(rule_option_t *option) {
  if (option) {
    array_free(option -> items);
    free(option);
  }
}

void rule_option_dump(rule_option_t *option) {
  array_visit(option -> items, (visit_t) rule_item_dump);
  fprintf(stderr, " |");
}

/*
 * rule_item_t static functions
 */

rule_item_t * _rule_item_create(rule_option_t *option, int terminal) {
  rule_item_t *ret;

  ret = NEW(rule_item_t);
  if (ret) {
    ret -> terminal = terminal;
    ret -> option = option;
    if (!array_push(option -> items, ret)) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

/*
 * rule_item_t public functions
 */

void rule_item_free(rule_item_t *item) {
  if (item) {
    if (item -> terminal) {
      token_free(item -> token);
    } else {
      free(item -> rule);
    }
    free(item);
  }
}

rule_item_t * rule_item_non_terminal(rule_option_t *option, char *rule) {
  rule_item_t *ret;

  ret = _rule_item_create(option, FALSE);
  if (ret) {
    ret -> rule = strdup(rule);
  }
  return ret;
}

rule_item_t * rule_item_terminal(rule_option_t *option, token_t *token) {
  rule_item_t *ret;
  int          code;
  char        *str;
  token_t     *terminal_token;

  ret = _rule_item_create(option, TRUE);
  if (ret) {
    code = token_code(token);
    str = token_token(token);
    if (code == TokenCodeDQuotedStr) {
      code = (int) strhash(str);
      ret -> token = token_create(code, str);
      set_add(option -> rule -> grammar -> keywords, ret -> token);
    } else {
      ret -> token = token_copy(token);
    }
  }
  return ret;
}

rule_item_t * rule_item_empty(rule_option_t *option) {
  rule_item_t *ret;

  ret = _rule_item_create(option, TRUE);
  if (ret) {
    ret -> token = token_create(TokenCodeEmpty, "$");
  }
  return ret;
}

parser_fnc_t rule_item_get_function(rule_item_t *item) {
  return item -> fnc;
}

rule_item_t * rule_item_set_function(rule_item_t *item, parser_fnc_t fnc) {
  item -> fnc = fnc;
  return item;
}

void rule_item_dump(rule_item_t *item) {
  fprintf(stderr, " ");
  if (item -> terminal) {
    fprintf(stderr, "'%s' (%d)", token_token(item -> token), token_code(item -> token));
  } else {
    fprintf(stderr, "%s", item -> rule);
  }
}

/*
 * grammar_t static functions
 */

grammar_parser_t * _grammar_token_handler(token_t *token, grammar_parser_t *grammar_parser) {
  int  code;
  char *str;
  char terminal_str[2];

  code = token_code(token);
  str = token_token(token);
  switch (token_code(token)) {
    case TokenCodeIdentifier:
      if (grammar_parser -> rule == NULL) {
        assert(code == TokenCodeIdentifier);
        grammar_parser -> rule = rule_create(grammar_parser -> grammar, str);
        grammar_parser -> option = NULL;
        grammar_parser -> item = NULL;
      } else {
        assert(grammar_parser -> option != NULL);
        grammar_parser -> item = rule_item_non_terminal(grammar_parser -> option, str);
      }
      break;
    case 200:
      assert(grammar_parser -> option == NULL);
      grammar_parser -> option = rule_option_create(grammar_parser -> rule);
      break;
    case TokenCodePipe:
      assert(grammar_parser -> option != NULL);
      grammar_parser -> option = rule_option_create(grammar_parser -> rule);
      break;
    case TokenCodeDQuotedStr:
      assert(grammar_parser -> option != NULL);
      grammar_parser -> item = rule_item_terminal(grammar_parser -> option, token);
      break;
    case TokenCodeSQuotedStr:
      if (strlen(str) == 1) {
        assert(grammar_parser -> option != NULL);
        code = str[0];
        strcpy(terminal_str, str);
        token_free(token);
        token = token_create(code, terminal_str);
        grammar_parser -> item = rule_item_terminal(grammar_parser -> option, token);
      } else {
        assert(0);
      }
      break;
    case TokenCodeDollar:
      assert(grammar_parser -> option != NULL);
      grammar_parser -> item = rule_item_empty(grammar_parser -> option);
      break;
    case TokenCodeSemiColon:
      assert(grammar_parser -> option != NULL);
      assert(grammar_parser -> rule != NULL);
      grammar_parser -> rule = NULL;
      grammar_parser -> option = NULL;
      grammar_parser -> item = NULL;
      break;
    default:
      assert(grammar_parser -> option != NULL);
      if ((code >= '!') && (code <= '~')) {
        grammar_parser -> item = rule_item_terminal(grammar_parser -> option, token);
      } else {
        assert(0);
      }
      break;
  }
  return grammar_parser;
}

void _grammar_dump_rule(entry_t *entry) {
  rule_dump((rule_t *) entry -> value);
}

/*
 * grammar_t public functions
 */

grammar_t * grammar_create() {
  grammar_t *ret;
  int ix;
  int ok = TRUE;

  ret = NEW(grammar_t);
  if (ret) {
    ret -> keywords = set_create((cmp_t) token_cmp);
    if (ok = (ret != NULL)) {
      set_set_hash(ret -> keywords, (hash_t) strhash);
      set_set_free(ret -> keywords, (free_t) token_free);
    }
    if (ok) {
      ret -> rules = dict_create((cmp_t) strcmp);
      if (ok = (ret -> rules != NULL)) {
        dict_set_hash(ret -> rules, (hash_t) token_hash);
        dict_set_free_data(ret -> rules, (free_t) rule_free);
      }
    }
    if (ok) {
      ret -> lexer_options = array_create((int) LexerOptionLAST);
      if (ok == (ret -> lexer_options != NULL)) {
        for (ix = 0; ix < (int) LexerOptionLAST; ix++) {
          grammar_set_option(ret, ix, 0L);
        }
      }
    }
    if (!ok) {
      grammar_free(ret);
      ret = NULL;
    }
  }
  return ret;
}

grammar_t * _grammar_read(reader_t *reader) {
  grammar_t *grammar;
  lexer_t *lexer;
  grammar_parser_t grammar_parser;

  grammar = grammar_create();
  if (grammar) {
    lexer = lexer_create(reader);
    if (lexer) {
      lexer_add_keyword(lexer, token_create(200, ":="));
      lexer_set_option(lexer, LexerOptionIgnoreWhitespace, TRUE);
      grammar_parser.grammar = grammar;
      grammar_parser.rule = NULL;
      grammar_parser.option = NULL;
      grammar_parser.item = NULL;
      lexer_tokenize(lexer, _grammar_token_handler, &grammar_parser);
      lexer_free(lexer);
    }
  }
  return grammar;
}

void grammar_free(grammar_t *grammar) {
  if (grammar) {
    dict_free(grammar -> rules);
    free(grammar);
  }
}

grammar_t * grammar_set_initializer(grammar_t *grammar, parser_fnc_t init) {
  grammar -> initializer = init;
  return grammar;
}

parser_fnc_t grammar_get_initializer(grammar_t *grammar) {
  return grammar -> initializer;
}

grammar_t * grammar_set_finalizer(grammar_t *grammar, parser_fnc_t finalizer) {
  grammar -> initializer = finalizer;
  return grammar;
}

parser_fnc_t grammar_get_finalizer(grammar_t *grammar) {
  return grammar -> finalizer;
}

grammar_t * grammar_set_option(grammar_t *grammar, lexer_option_t option, long value) {
  array_set(grammar -> lexer_options, (int) option, (void *) value);
  return grammar;
}

long grammar_get_option(grammar_t *grammar, lexer_option_t option) {
  return (long) array_get(grammar -> lexer_options, (int) option);
}

void grammar_dump(grammar_t *grammar) {
  if (set_size(grammar -> keywords)) {
    fprintf(stderr, "< ");
    set_visit(grammar -> keywords, (visit_t) token_dump);
    fprintf(stderr, " >\n");
  }
  dict_visit(grammar -> rules, (visit_t) _grammar_dump_rule);
}

/*
 * parser_t static functions
 */

void * _parser_token_handler(token_t *token, parser_t *parser) {
  list_push(parser -> tokens, token_copy(token));
  return parser;
}

/*
 * parser_t public functions
 */

parser_t * parser_create(grammar_t *grammar) {
  parser_t *ret;

  if (ret) {
    ret -> grammar = grammar;
    ret -> tokens = list_create();
    if (ret -> tokens) {
      list_set_free(ret -> tokens, (free_t) token_free);
    } else {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

parser_t * _parser_read_grammar(reader_t *reader) {
  parser_t  *ret;
  grammar_t *grammar;

  grammar = grammar_read(reader);
  if (grammar) {
    ret = NEW(parser_t);
    if (ret) {
      ret -> grammar = grammar;
    } else {
      grammar_free(grammar);
    }
  }
  return ret;
}

lexer_t * _parser_set_keywords(token_t *keyword, lexer_t *lexer) {
  lexer_add_keyword(lexer, keyword);
  return lexer;
}

void _parser_parse(parser_t *parser, reader_t *reader) {
  lexer_t        *lexer;
  lexer_option_t  ix;

  lexer = lexer_create(reader);
  set_reduce(parser -> grammar -> keywords, (reduce_t) _parser_set_keywords, lexer);
  for (ix = 0; ix < LexerOptionLAST; ix++) {
    lexer_set_option(lexer, ix, grammar_get_option(parser -> grammar, ix));
  }
  if (grammar_get_initializer(parser -> grammar)) {
    grammar_get_initializer(parser -> grammar)(parser);
  }
  lexer_tokenize(lexer, _parser_token_handler, parser);
  if (grammar_get_finalizer(parser -> grammar)) {
    grammar_get_finalizer(parser -> grammar)(parser);
  }
  lexer_free(lexer);
}

void parser_free(parser_t *parser) {
  if (parser) {
    list_free(parser -> tokens);
    grammar_free(parser -> grammar);
    free(parser);
  }
}
