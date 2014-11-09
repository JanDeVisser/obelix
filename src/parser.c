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

#include <stdio.h>

#include <array.h>
#include <expr.h>
#include <lexer.h>
#include <list.h>

typedef enum _parse_rule {
  ParserStateProgram,
  ParserStateError,
  ParserStateEmpty,
  ParserStateExpr,
  ParserStateTerminalExpr,
  ParserStateAtomExpr,
  ParserStateNumber,
  ParserStateMult,
  ParserStateParExpr
} parser_state_t;

typedef struct _grammar_rule {
  parser_state_t   state;
  array_t         *options;
} grammar_rule_t;

typedef struct _rule_option {
  array_t      *items;
} rule_option_t;

rule_option_t * rule_option_create(grammar_rule_t *rule) {
  rule_option_t *ret;

  ret = NEW(rule_option_t);
  if (ret) {
    ret -> items = array_create(3);
    if (!ret -> items) {
      free(ret);
      ret = NULL;
    } else {
      array_push(rule -> options, ret);
    }
  }
  return ret;
}


typedef struct _rule_item {
  int      *terminal;
  union {
    token_code_t   code;
    parser_state_t state;
  };
} rule_item_t;

rule_item_t _rule_item_create(rule_option_t *option, int terminal) {
  rule_item_t *ret;

  ret = NEW(rule_item_t);
  if (ret) {
    ret -> terminal = terminal;
    array_push(option -> items, ret);
  }
  return ret;
}

rule_item_t * rule_item_non_terminal(rule_option_t *option, parser_state_t state) {
  rule_item_t *ret;

  ret = _rule_item_create(option, FALSE);
  if (ret) {
    ret -> state = state;
  }
  return ret;
}

rule_item_t * rule_item_terminal(rule_option_t *option, token_code_t code) {
  rule_item_t *ret;

  ret = _rule_item_create(option, TRUE);
  if (ret) {
    ret -> code = code;
  }
  return ret;
}

rule_item_t * rule_item_empty(rule_option_t *option) {
  rule_item_t *ret;

  ret = _rule_item_create(option, TRUE);
  if (ret) {
    ret -> code = TokenCodeEmpty;
  }
  return ret;
}


extern grammar_rule_t * grammar_rule_create(parser_rule_t);

grammar_rule_t * grammar_rule_create(parser_state_t state) {
  grammar_rule_t *ret;

  ret = NEW(grammar_rule_t);
  if (ret) {
    ret -> state = state;
    ret -> options = array_create(2);
    if (!ret -> options) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}


static list_t *grammar = NULL;

void _init_grammar(char *fname) {
  grammar_rule_t *rule;
  rule_option_t *option;
  rule_item_t *item;



  grammar = list_create();

  rule = grammar_rule_create(ParserStateProgram);
  rule_item_non_terminal(rule_option_create(rule), ParserStateExpr);
  rule_item_empty(rule_option_create(rule));
  list_append(grammar, rule);

  rule = grammar_rule_create(ParserStateExpr);
  grammar_rule_add_item(rule, rule_item_non_terminal(ParserStateTerminalExpr));
  grammar_rule_add_item(rule, rule_item_non_terminal(ParserStateTerminalExpr));
  list_append(grammar, rule);


}


typedef struct _parser {
  lexer_t         *lexer;
  parser_state_t  *state;
  expr_t          *atom_expr;
  expr_t          *atom_expr_1;
  expr_t          *expr;
  int              factor;
  int              param_level;
} parser_t;

typedef parser_state_t * (*parser_state_handler_fnc_)(parser_t  *, token_t *);

typedef struct _parser_state_handler {
  parser_state_t            state;
  parser_state_handler_fnc_ handler;
} parser_state_handler_t;


static parser_state_t handle_expr(parser_t *, token_t *);
static parser_state_t handle_error(parser_t *, token_t *);
static parser_state_t handle_number(parser_t *, token_t *);
static parser_state_t handle_mult(parser_t *, token_t *);
static parser_state_t handle_atom(parser_t *, token_t *);

static parser_state_handler_t parser_state_handlers[] = {
    { state: ParserStateProgram, handler: handle_expr },
    { state: ParserStateExpr, handler: handle_expr },
    { state: ParserStateError, handler: handle_error },
    { state: ParserStateNumber, handler: handle_number },
    { state: ParserStateMult, handler: handle_mult },
    { state: ParserStateAtomExpr, handler: handle_expr }
};

/*
 * expression functions
 */

static data_t * _add(context_t *ctx, void *data, list_t *params, int factor) {
  int sum, val;
  listiterator_t *iter;
  data_t *cur, *ret;

  sum = 0;
  for(iter = li_create(params); li_has_next(iter); ) {
    cur = (data_t *) li_next(iter);
    data_get(cur, &val);
    sum += factor * val;
  }
  return data_create(Int, &sum);
}
static data_t * _plus(context_t *ctx, void *data, list_t *params) {
  return _add(ctx, data, 1);
}

static data_t * _minus(context_t *ctx, void *data, list_t *params) {
  return _add(ctx, data, -1);
}

static data_t * _mult(context_t *ctx, void *data, list_t *params) {
  int prod, val;
  listiterator_t *iter;
  data_t *cur, *ret;

  debug("*");
  prod = 1;
  for(iter = li_create(params); li_has_next(iter); ) {
    cur = (data_t *) li_next(iter);
    data_get(cur, &val);
    prod *= val;
  }
  return data_create(Int, &prod);
}

/*
 * Parser state handlers
 */

parser_state_t handle_expr(parser_t *parser, token_t *token) {
  expr_t *expr;

  switch (token_code(token)) {
    case TokenCodeInteger:
      return handle_number(parser, token);
    case TokenCodePlus:
      parser -> factor = 1;
      return ParserStateNumber;
    case TokenCodeMinus:
      parser -> factor = -1;
      return ParserStateNumber;
    case TokenCodeOpenPar:
      parser -> param_level++;
      return ParserStateParExpr;
    default:
      return ParserStateError;
  }
}

parser_state_t handle_number(parser_t *parser, token_t *token) {
  if (token_code(token) == TokenCodeInteger) {
    parser -> atom_expr = expr_int_literal(parser -> factor * atoi(token -> token));
    parser -> factor = 1;
    return ParserStateAtomExpr;
  } else {
    return ParserStateError;
  }
}

parser_state_t handle_atom(parser_t *parser, token_t *token) {
  if ((token_code(token) == TokenCodeAsterisk) || (token_code(token) == TokenCodeSlash)) {
    parser -> factor = (token_code(token) == TokenCodeAsterisk) ? 1 : -1;
    parser -> atom_expr_1 = parser -> atom_expr;
    return ParserStateMult;
  } else {
    return ParserStateExpr;
  }
}

parser_state_t handle_mult(parser_t *parser, token_t *token) {
  if (token_code(token) == TokenCodeInteger) {
    parser -> atom_expr = expr_int_literal(parser -> factor * atoi(token -> token));
    parser -> factor = 1;
    return ParserStateAtomExpr;
  } else {
    return ParserStateError;
  }
}


parser_t * parser_create(lexer_t *lexer) {
  parser_t *ret;

  if (ret) {
    ret -> lexer = lexer;
    ret -> factor = 1;
    ret -> param_level = 0;
  }
  return ret;
}

void parser_parse(parser_t *parser) {
  token_t *token;
  int ix;

  for (token = lexer_next_token(parser -> lexer); token;
       token = lexer_next_token(parser -> lexer)) {
    if (token_code(token) != TokenCodeWhitespace) {
      for (ix = 0; ix < sizeof(parser_state_handlers) / sizeof(parser_state_handler_t); ix++) {
        if (parser_state_handlers[ix].state == parser -> state) {
          parser -> state = parser_state_handlers[ix].handler(parser, token);
          break;
        }
      }
    }
  }
}
