/*
 * /obelix/src/grammarparser.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "libgrammar.h"
#include <grammarparser.h>
#include <nvp.h>

typedef struct _gp_state_rec {
  char     *name;
  reduce_t  handler;
} gp_state_rec_t;

static grammar_parser_t * _grammar_parser_set_option(grammar_parser_t *, token_t *);
static grammar_parser_t * _grammar_parser_state_options_end(token_t *, grammar_parser_t *);

static grammar_parser_t * _grammar_parser_state_start(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_options(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_option_name(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_option_value(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_header(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_nonterminal(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_rule(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_entry(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_modifier(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_separator(token_t *, grammar_parser_t *);
static void               _grammar_parser_syntax_error(grammar_parser_t *, exception_t *);

static gp_state_rec_t _gp_state_recs[] = {
  { "GPStateStart",        (reduce_t) _grammar_parser_state_start },
  { "GPStateOptions",      (reduce_t) _grammar_parser_state_options },
  { "GPStateOptionName",   (reduce_t) _grammar_parser_state_option_name },
  { "GPStateOptionValue",  (reduce_t) _grammar_parser_state_option_value },
  { "GPStateHeader",       (reduce_t) _grammar_parser_state_header },
  { "GPStateNonTerminal",  (reduce_t) _grammar_parser_state_nonterminal },
  { "GPStateRule",         (reduce_t) _grammar_parser_state_rule },
  { "GPStateEntry",        (reduce_t) _grammar_parser_state_entry },
  { "GPStateModifier",     (reduce_t) _grammar_parser_state_modifier },
  { "GPStateSeparator",    (reduce_t) _grammar_parser_state_separator }
};


/*
 * grammar_parser_t static functions
 */

 grammar_parser_t * _grammar_parser_set_option(grammar_parser_t *grammar_parser, token_t *value) {
   data_t *val = (value) ? (data_t *) str_copy_chars(token_token(value)) : NULL;

   if (grammar_parser -> last_token) {
     data_set_attribute((data_t *) grammar_parser -> ge,
                        token_token(grammar_parser -> last_token),
                        val);
     data_free(val);
     token_free(grammar_parser -> last_token);
     grammar_parser -> last_token = NULL;
   }
   return grammar_parser;
}

data_t * _grammar_parser_xform(grammar_parser_t *gp, token_t *token) {
  char         *str;
  token_code_t  code;
  size_t        len;
  data_t       *ret = NULL;
  nvp_t        *kw;

  str = token_token(token);
  code = (token_code_t) token_code(token);
  switch (code) {
    case TokenCodeDQuotedStr:
      len = strlen(str);
      if (len >= 1) {
        token_free(token);
        assert(gp -> grammar -> lexer);
        token = (token_t *) dict_get(gp -> keywords, str);
        if (!token) {
          token = token_create(gp -> next_keyword_code++, str);
          kw = nvp_create(data_uncopy((data_t *) str_wrap("keyword")),
                          (data_t *) token_copy(token));
          lexer_config_set(gp -> grammar -> lexer, "keyword", (data_t *) kw);
          nvp_free(kw);
          dict_put(gp -> keywords, strdup(str), token);
        }
      } else { /* len == 0 */
        ret = (data_t *) exception_create(ErrorSyntax,
          "The empty string cannot be a keyword");
      }
      break;

    case TokenCodeSQuotedStr:
      len = strlen(str);
      if (len == 1) {
        code = (token_code_t) str[0];
        token_free(token);
        token = token_create(code, str);
      } else if (len == 0) {
        ret = (data_t *) exception_create(ErrorSyntax,
          "The empty single-quoted string cannot be used in a rule or rule "
          "entry definition", str);
      } else { /* len > 0 */
        ret = (data_t *) exception_create(ErrorSyntax,
          "Single-quoted string longer than 1 character '%s' cannot be used in "
          "a rule or rule entry definition", str);
      }
      break;

    default:
      if ((code < '!') || (code > '~')) {
        ret = (data_t *) exception_create(ErrorSyntax,
          "Token '%s' cannot be used in a rule or rule entry definition", str);
      }
      break;
  }
  return (ret) ? ret : (data_t *) token;
}

grammar_parser_t * _grammar_parser_replace_entry(grammar_parser_t *gp, char *nt) {
  rule_entry_t *entry;

  /*
   * We now have the synthetic nonterminal. Either it already existed or we
   * built it. Now need to replace entry in current rule with reference to
   * synthetic nonterminal:
   */

  /* Pop current entry from current rule and free it: */
  entry = array_pop(gp -> rule -> entries);
  rule_entry_free(entry);

  /* Build new entry: */
  gp -> entry = rule_entry_non_terminal(gp -> rule, nt);
  gp -> ge = (ge_t *) gp -> entry;
  return gp;
}

grammar_parser_t * _grammar_parser_make_optional(grammar_parser_t *gp) {
  char          *synthetic_nt_name;
  nonterminal_t *nt;
  rule_t        *rule;

  asprintf(&synthetic_nt_name, "%s?", rule_entry_tostring(gp -> entry));
  nt = grammar_get_nonterminal(gp -> grammar, synthetic_nt_name);
  if (!nt) {
    /*
     * Build synthetic nonterminal. We are converting
     *   nonterminal := ... entry ? ...
     * into
     *   nonterminal := ... entry_? ...
     *   entry_?     := entry |
     */
    nt = nonterminal_create(gp -> grammar, synthetic_nt_name);
    rule = rule_create(nt);
    /* Add entry currently being processed to synthetic rule: */
    array_push(rule -> entries, rule_entry_copy(gp -> entry));
    /* Create empty rule: */
    rule_create(nt);
  }
  _grammar_parser_replace_entry(gp, synthetic_nt_name);
  free(synthetic_nt_name);
  gp -> state = GPStateRule;
  return gp;
}

grammar_parser_t * _grammar_parser_expand_modifier(grammar_parser_t *gp, token_t *sep) {
  char          *nt_star_sep;
  char          *nt_plus_sep;
  char          *nt_sep;
  char          *sepstr;
  nonterminal_t *nt;
  rule_t        *rule;

  sepstr = (sep) ? token_token(sep) : "[None]";
  asprintf(&nt_star_sep, "%s*%s", rule_entry_tostring(gp -> entry), sepstr);
  asprintf(&nt_plus_sep, "%s+%s", rule_entry_tostring(gp -> entry), sepstr);
  asprintf(&nt_sep, "%s%s", sepstr, rule_entry_tostring(gp -> entry));

  nt = grammar_get_nonterminal(gp -> grammar, nt_star_sep);
  if (!nt) {
    nt = nonterminal_create(gp -> grammar, nt_star_sep);
    rule = rule_create(nt);
    rule_entry_non_terminal(rule, nt_plus_sep);
    /* Create empty rule: */
    rule_create(nt);
  }

  nt = grammar_get_nonterminal(gp -> grammar, nt_plus_sep);
  if (!nt) {
    nt = nonterminal_create(gp -> grammar, nt_plus_sep);
    rule = rule_create(nt);
    array_push(rule -> entries, rule_entry_copy(gp -> entry));
    rule_entry_non_terminal(rule, nt_sep);
  }

  nt = grammar_get_nonterminal(gp -> grammar, nt_sep);
  if (!nt) {
    nt = nonterminal_create(gp -> grammar, nt_sep);
    rule = rule_create(nt);
    if (sep) {
      rule_entry_terminal(rule, sep);
    }
    rule_entry_non_terminal(rule, nt_plus_sep);
    /* Create empty rule: */
    rule_create(nt);
  }

  _grammar_parser_replace_entry(gp,
    (gp -> modifier == '+') ? nt_plus_sep : nt_star_sep);
  free(nt_plus_sep);
  free(nt_star_sep);
  free(nt_sep);
  gp -> state = GPStateRule;
  return gp;
}

grammar_parser_t * _grammar_parser_state_start(token_t *token, grammar_parser_t *grammar_parser) {
  int        code;
  grammar_t *g;
  char      *str;

  g = grammar_parser -> grammar;
  code = token_code(token);
  str = token_token(token);
  switch (code) {
    case TokenCodeIdentifier:
      grammar_parser -> state = GPStateNonTerminal;
      grammar_parser -> nonterminal = nonterminal_create(grammar_parser -> grammar, str);
      grammar_parser -> rule = NULL;
      grammar_parser -> entry = NULL;
      grammar_parser -> ge = (ge_t *) grammar_parser -> nonterminal;
      break;

    case TokenCodePercent:
      grammar_parser -> old_state = GPStateStart;
      grammar_parser -> state = GPStateOptions;
      grammar_parser -> ge = (ge_t *) g;
      break;

    case TokenCodeOpenBracket:
      grammar_parser -> old_state = GPStateNonTerminal;
      grammar_parser -> state = GPStateOptions;
      break;

    default:
      _grammar_parser_syntax_error(grammar_parser,
        exception_create(
          ErrorSyntax, "Unexpected token '%s' at start of grammar text", str));
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_options_end(token_t *token, grammar_parser_t *grammar_parser) {
  _grammar_parser_set_option(grammar_parser, NULL);
  grammar_parser -> state = grammar_parser -> old_state;
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_options(token_t *token, grammar_parser_t *grammar_parser) {
  grammar_parser_t *ret = NULL;

  switch (token_code(token)) {
    case TokenCodeIdentifier:
      grammar_parser -> last_token = token_copy(token);
      grammar_parser -> state = GPStateOptionName;
      ret = grammar_parser;
      break;

    case TokenCodePercent:
      if (grammar_parser -> old_state == GPStateStart) {
        ret = _grammar_parser_state_options_end(token, grammar_parser);
      }
      break;

    case TokenCodeCloseBracket:
      if (grammar_parser -> old_state != GPStateStart) {
        ret = _grammar_parser_state_options_end(token, grammar_parser);
      }
      break;
  }
  if (!ret) {
    _grammar_parser_syntax_error(grammar_parser,
      exception_create(ErrorSyntax,
        "Unexpected token '%s' in option block", token_tostring(token)));
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_option_name(token_t *token, grammar_parser_t *grammar_parser) {
  grammar_parser_t *ret = NULL;

  switch (token_code(token)) {
    case TokenCodeColon:
    case TokenCodeEquals:
      grammar_parser -> state = GPStateOptionValue;
      ret = grammar_parser;
      break;

    case TokenCodePercent:
      if (grammar_parser -> old_state == GPStateStart) {
        ret = _grammar_parser_state_options_end(token, grammar_parser);
      }
      break;

    case TokenCodeCloseBracket:
      if (grammar_parser -> old_state != GPStateStart) {
        ret = _grammar_parser_state_options_end(token, grammar_parser);
      }
      break;

    case TokenCodeIdentifier:
      _grammar_parser_set_option(grammar_parser, NULL);
      ret = _grammar_parser_state_options(token, grammar_parser);
      break;
  }
  if (!ret) {
    _grammar_parser_syntax_error(grammar_parser,
      exception_create(ErrorSyntax,
        "Unexpected token '%s' in option block", token_tostring(token)));
  }
  return ret;
}

grammar_parser_t * _grammar_parser_state_option_value(token_t *token, grammar_parser_t *grammar_parser) {
  switch (token_code(token)) {
    case TokenCodeIdentifier:
    case TokenCodeInteger:
    case TokenCodeHexNumber:
    case TokenCodeFloat:
    case TokenCodeSQuotedStr:
    case TokenCodeDQuotedStr:
    case TokenCodeBQuotedStr:
      _grammar_parser_set_option(grammar_parser, token);
      grammar_parser -> state = GPStateOptions;
      break;

    default:
      _grammar_parser_syntax_error(grammar_parser,
        exception_create(ErrorSyntax,
          "Unexpected token '%s' in option block", token_tostring(token)));
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_header(token_t *token, grammar_parser_t *grammar_parser) {
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_nonterminal(token_t *token, grammar_parser_t *grammar_parser) {
  switch (token_code(token)) {
    case TokenCodeIdentifier:
      info("Non-terminal '%s'", token_token(token));
      grammar_parser -> nonterminal = nonterminal_create(grammar_parser -> grammar,
                                                         token_token(token));
      grammar_parser -> rule = NULL;
      grammar_parser -> entry = NULL;
      grammar_parser -> ge = (ge_t *) grammar_parser -> nonterminal;
      break;

    case TokenCodeOpenBracket:
      grammar_parser -> old_state = GPStateNonTerminal;
      grammar_parser -> state = GPStateOptions;
      break;

    case NONTERMINAL_DEF:
      if (grammar_parser -> nonterminal) {
        grammar_parser -> rule = rule_create(grammar_parser -> nonterminal);
        grammar_parser -> state = GPStateRule;
        grammar_parser -> ge = (ge_t *) grammar_parser -> rule;
      } else {
        _grammar_parser_syntax_error(grammar_parser,
          exception_create(ErrorSyntax,
            "The ':=' operator must be preceded by a non-terminal name",
            token_tostring(token)));
      }
      break;

    case TokenCodeEOF:
      if (grammar_parser -> nonterminal) {
        _grammar_parser_syntax_error(grammar_parser,
          exception_create(ErrorSyntax,
            "Unexpected end-of-file in definition of non-terminal '%s'",
            grammar_parser -> nonterminal -> name));
      }
      break;

    default:
      if (grammar_parser -> nonterminal) {
        _grammar_parser_syntax_error(grammar_parser,
          exception_create(ErrorSyntax,
            "Unexpected token '%s' in definition of non-terminal '%s'",
            token_tostring(token), grammar_parser -> nonterminal -> name));
      } else {
        _grammar_parser_syntax_error(grammar_parser,
          exception_create(ErrorSyntax,
            "Unexpected token '%s', was expecting non-terminal definition",
            token_tostring(token)));
      }
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_rule(token_t *token, grammar_parser_t *grammar_parser) {
  data_t  *xformed;
  token_t *xformed_token;
  int      code;

  xformed = _grammar_parser_xform(grammar_parser, token);
  if (data_is_exception(xformed)) {
    _grammar_parser_syntax_error(grammar_parser, (exception_t *) xformed);
    return grammar_parser;
  }
  xformed_token = (token_t *) xformed;
  code = token_code(token);
  switch (code) {
    case TokenCodePipe:
      grammar_parser -> rule = rule_create(grammar_parser -> nonterminal);
      grammar_parser -> state = GPStateRule;
      grammar_parser -> ge = (ge_t *) grammar_parser -> rule;
      break;

    case TokenCodeSemiColon:
      grammar_parser -> nonterminal = NULL;
      grammar_parser -> rule = NULL;
      grammar_parser -> entry = NULL;
      grammar_parser -> state = GPStateNonTerminal;
      break;

    case TokenCodeOpenBracket:
      grammar_parser -> old_state = grammar_parser -> state;
      grammar_parser -> state = GPStateOptions;
      break;

    case TokenCodeIdentifier:
      grammar_parser -> entry = rule_entry_non_terminal(
        grammar_parser -> rule, token_token(token));
      grammar_parser -> ge = (ge_t *) grammar_parser -> entry;
      grammar_parser -> state = GPStateEntry;
      break;

    case TokenCodeDQuotedStr:
    case TokenCodeSQuotedStr:
      grammar_parser -> entry = rule_entry_terminal(
        grammar_parser -> rule, xformed_token);
      grammar_parser -> ge = (ge_t *) grammar_parser -> entry;
      grammar_parser -> state = GPStateEntry;
      break;

    default:
      grammar_parser -> entry = rule_entry_terminal(grammar_parser -> rule, token);
      grammar_parser -> ge = (ge_t *) grammar_parser -> entry;
      grammar_parser -> state = GPStateEntry;
      break;
  }
  if (xformed_token != token) {
    token_free(xformed_token);
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_entry(token_t *token, grammar_parser_t *grammar_parser) {
  int    code;

  code = token_code(token);
  switch (code) {
    case TokenCodeQMark:
      _grammar_parser_make_optional(grammar_parser);
      grammar_parser -> state = GPStateRule;
      break;

    case TokenCodePlus:
    case TokenCodeAsterisk:
      grammar_parser -> state = GPStateModifier;
      grammar_parser -> modifier = code;
      break;

    default:
      return _grammar_parser_state_rule(token, grammar_parser);
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_modifier(token_t *token, grammar_parser_t *grammar_parser) {
  switch (token_code(token)) {
    case TokenCodeComma:
      grammar_parser -> state = GPStateSeparator;
      break;
    default:
      _grammar_parser_expand_modifier(grammar_parser, NULL);
      return _grammar_parser_state_rule(token, grammar_parser);
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_separator(token_t *token, grammar_parser_t *gp) {
  data_t *xformed;

  xformed = _grammar_parser_xform(gp, token);
  if (data_is_token(xformed)) {
    _grammar_parser_expand_modifier(gp, (token_t *) xformed);
    if ((token_t *) xformed != token) {
      data_free(xformed);
    }
  } else {
    _grammar_parser_syntax_error(gp, (exception_t *) xformed);
  }
  return gp;
}

void _grammar_parser_syntax_error(grammar_parser_t *gp, exception_t *error) {
  error("Syntax error in grammar: %s", exception_tostring(error));
  gp -> state = GPStateError;
}


lexer_config_t * _grammar_token_handler(token_t *token, lexer_config_t *lexer) {
  gp_state_t        state;
  grammar_parser_t *grammar_parser;

  if (token_code(token) == TokenCodeEnd) {
    return NULL;
  }
  grammar_parser = (grammar_parser_t *) lexer -> data;
  state = grammar_parser -> state;
  debug(grammar, "%-18.18s %s", _gp_state_recs[state].name, token_tostring(token));
  _gp_state_recs[state].handler(token, grammar_parser);
  return (grammar_parser -> state != GPStateError) ? lexer : NULL;
}

/*
 * grammar_parser_t public functions
 */

grammar_parser_t * grammar_parser_create(data_t *reader) {
  grammar_parser_t *grammar_parser;

  grammar_parser = NEW(grammar_parser_t);
  grammar_parser -> reader = reader;
  grammar_parser -> state = GPStateStart;
  grammar_parser -> grammar = NULL;
  grammar_parser -> last_token = NULL;
  grammar_parser -> nonterminal = NULL;
  grammar_parser -> rule = NULL;
  grammar_parser -> entry = NULL;
  grammar_parser -> ge = NULL;
  grammar_parser -> dryrun = FALSE;
  grammar_parser -> keywords = strdata_dict_create();
  grammar_parser -> next_keyword_code = 300;
  return grammar_parser;
}

void grammar_parser_free(grammar_parser_t *grammar_parser) {
  if (grammar_parser) {
    token_free(grammar_parser -> last_token);
    dict_free(grammar_parser -> keywords);
    free(grammar_parser);
  }
}

grammar_t * grammar_parser_parse(grammar_parser_t *gp) {
  lexer_config_t   *lexer;
  scanner_config_t *scanner;
  token_t          *nonterminal_def;

  gp -> grammar = grammar_create();
  gp -> grammar -> dryrun = gp -> dryrun;
  lexer = lexer_config_create();

  scanner = lexer_config_add_scanner(lexer, "keyword");
  nonterminal_def = token_create(NONTERMINAL_DEF, NONTERMINAL_DEF_STR);
  scanner_config_setvalue(scanner, "keyword", (data_t *) nonterminal_def);
  token_free(nonterminal_def);

  lexer_config_add_scanner(lexer, "whitespace: ignoreall=1");
  lexer_config_add_scanner(lexer, "identifier");
  lexer_config_add_scanner(lexer, "number");
  lexer_config_add_scanner(lexer, "qstring");
  lexer_config_add_scanner(lexer, "comment: marker=/* */;marker=//;marker=^#");

  lexer -> data = (data_t *) gp;

  lexer_config_tokenize(lexer, (reduce_t) _grammar_token_handler, gp -> reader);
  if (gp -> state != GPStateError) {
    if (grammar_analyze(gp -> grammar)) {
      if (grammar_debug) {
        info("Grammar successfully analyzed");
      }
    } else {
      error("Error(s) analyzing grammar - re-run with -d grammar for details");
    }
  }
  lexer_config_free(lexer);
  return gp -> grammar;
}
