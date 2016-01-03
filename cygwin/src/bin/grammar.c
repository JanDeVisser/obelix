#include <grammar.h>

grammar_t * build_grammar() {
  grammar_t     *grammar;
  nonterminal_t *nonterminal;
  rule_t        *rule;
  rule_entry_t  *entry;
  token_t       *token_name, *token_value;

  grammar = grammar_create();
  grammar_set_lexer_option(grammar, LexerOptionIgnoreWhitespace, 1);
  grammar_set_lexer_option(grammar, LexerOptionIgnoreNewLines, 0);
  grammar_set_lexer_option(grammar, LexerOptionIgnoreAllWhitespace, 1);
  grammar_set_lexer_option(grammar, LexerOptionCaseSensitive, 0);
  grammar_set_lexer_option(grammar, LexerOptionHashPling, 1);
  grammar_set_lexer_option(grammar, LexerOptionSignedNumbers, 0);
  grammar_set_lexer_option(grammar, LexerOptionOnNewLine, 0);
  token_name = token_create(TokenCodeIdentifier, PREFIX_STR);
  token_value = token_create(TokenCodeIdentifier, "script_parse_");
  grammar_set_option(grammar, token_name, token_value);
  token_free(token_name);
  token_free(token_value);
  dict_put(grammar -> ge.variables, strdup("on_newline"), token_create(105, "mark_line"));

  nonterminal = nonterminal_create(grammar, "program");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "init"), NULL));
  entry = rule_entry_non_terminal(rule, "statements");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "done"), NULL));

  nonterminal = nonterminal_create(grammar, "factor");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "expr_atom");
  entry = rule_entry_non_terminal(rule, "subscript");

  nonterminal = nonterminal_create(grammar, "identifier");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "bookmark"), NULL));
  entry = rule_entry_non_terminal(rule, "_identifier");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "rollup_name"), NULL));

  nonterminal = nonterminal_create(grammar, "comprehension_or_tail");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "comprehension");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "discard_instruction_bookmark"), NULL));
  entry = rule_entry_non_terminal(rule, "entrylist_tail");

  nonterminal = nonterminal_create(grammar, "argument");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "incr"), NULL));

  nonterminal = nonterminal_create(grammar, "var_or_call");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "identifier");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "deref"), NULL));
  entry = rule_entry_non_terminal(rule, "call_or_empty");

  nonterminal = nonterminal_create(grammar, "continue");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(1118763914, "continue"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "continue"), NULL));

  nonterminal = nonterminal_create(grammar, "default");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(142955658, "default"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "else"), NULL));
  entry = rule_entry_terminal(rule, token_create(58, ":"));
  entry = rule_entry_non_terminal(rule, "statements");

  nonterminal = nonterminal_create(grammar, "case_block");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "case_prolog"), NULL));
  entry = rule_entry_non_terminal(rule, "case_stmt");
  entry = rule_entry_non_terminal(rule, "case_stmts");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "rollup_cases"), NULL));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "test"), NULL));
  entry = rule_entry_non_terminal(rule, "statements");

  nonterminal = nonterminal_create(grammar, "arglist_or_void");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "arglist");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "parlist_tail");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(44, ","));
  entry = rule_entry_non_terminal(rule, "parlist");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "factortail");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "mult_op");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "infix_op"), NULL));
  entry = rule_entry_non_terminal(rule, "factor");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "call_op"), NULL));
  entry = rule_entry_non_terminal(rule, "factortail");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "number");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(100, "d"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(102, "f"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(120, "x"));

  nonterminal = nonterminal_create(grammar, "constant");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(34, "\""));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "push_token"), NULL));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(39, "'"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "push_token"), NULL));
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "signed_number");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "list");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "object");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "regexp");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(2090770405, "true"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushconst"), data_parse(5, "bool:1")));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(258723568, "false"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushconst"), data_parse(5, "bool:0")));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(2090557760, "null"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushconst"), data_parse(5, "ptr:null")));

  nonterminal = nonterminal_create(grammar, "_identifier_tail");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(46, "."));
  entry = rule_entry_non_terminal(rule, "_identifier");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "dummy");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "leave");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(265970962, "leave"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "leave"), NULL));

  nonterminal = nonterminal_create(grammar, "reduction");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(177697, "|"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "deref_function"), data_parse(5, "reduce")));
  entry = rule_entry_non_terminal(rule, "expr");
  entry = rule_entry_non_terminal(rule, "reduction_init");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "reduce"), NULL));

  nonterminal = nonterminal_create(grammar, "conditional");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5863476, "if"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "if"), NULL));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "test"), NULL));
  entry = rule_entry_non_terminal(rule, "statements");
  entry = rule_entry_non_terminal(rule, "elif_seq");
  entry = rule_entry_non_terminal(rule, "else");
  entry = rule_entry_non_terminal(rule, "end");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "end_conditional"), NULL));

  nonterminal = nonterminal_create(grammar, "statement");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "conditional");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "while_loop");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "for_loop");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "break");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "continue");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "switch");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "func_def");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "return_stmt");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "yield_stmt");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "import_stmt");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "new");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "context_block");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "throw");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "leave");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "pass");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "nop"), NULL));
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "assignment_or_call");

  nonterminal = nonterminal_create(grammar, "func_block");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "start_function"), NULL));
  entry = rule_entry_non_terminal(rule, "baseclasses");
  entry = rule_entry_non_terminal(rule, "statements");
  entry = rule_entry_non_terminal(rule, "end");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "end_function"), NULL));
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "link_clause");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "native_function"), NULL));

  nonterminal = nonterminal_create(grammar, "_var_or_calls");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(46, "."));
  entry = rule_entry_non_terminal(rule, "var_or_calls");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "end");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(193490716, "end"));

  nonterminal = nonterminal_create(grammar, "cases_seq");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "case_block");
  entry = rule_entry_non_terminal(rule, "cases_seq");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "var_or_calls");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "var_or_call");
  entry = rule_entry_non_terminal(rule, "_var_or_calls");

  nonterminal = nonterminal_create(grammar, "_func_calls");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(46, "."));
  entry = rule_entry_non_terminal(rule, "var_or_calls");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "query_params");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(37, "%"));
  entry = rule_entry_terminal(rule, token_create(40, "("));
  entry = rule_entry_non_terminal(rule, "arglist");
  entry = rule_entry_terminal(rule, token_create(41, ")"));
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "parlist");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "param");
  entry = rule_entry_non_terminal(rule, "parlist_tail");

  nonterminal = nonterminal_create(grammar, "expr");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "predicate");
  entry = rule_entry_non_terminal(rule, "predicatetail");

  nonterminal = nonterminal_create(grammar, "elif");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(2090224421, "elif"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "elif"), NULL));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "test"), NULL));
  entry = rule_entry_non_terminal(rule, "statements");

  nonterminal = nonterminal_create(grammar, "attrlist_or_empty");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "attrlist");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "return_stmt");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(422601765, "return"));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "instruction"), data_parse(5, "Return")));

  nonterminal = nonterminal_create(grammar, "assignment_or_empty");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "dup"), NULL));
  entry = rule_entry_non_terminal(rule, "assignment");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "baseclasses");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(58, ":"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "baseclass_constructors"), NULL));
  entry = rule_entry_non_terminal(rule, "_baseclasses");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "end_constructors"), NULL));
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "logic_op");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(62, ">"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5862016, ">="));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(60, "<"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5861950, "<="));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5861983, "=="));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5861059, "!="));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5864125, "||"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5861201, "&&"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(193486360, "and"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5863686, "or"));

  nonterminal = nonterminal_create(grammar, "entrylist");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "entry");
  entry = rule_entry_non_terminal(rule, "entrylist_tail");

  nonterminal = nonterminal_create(grammar, "context_block");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(-747005238, "context"));
  entry = rule_entry_non_terminal(rule, "identifier");
  entry = rule_entry_non_terminal(rule, "assignment_or_empty");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "begin_context_block"), NULL));
  entry = rule_entry_non_terminal(rule, "statements");
  entry = rule_entry_non_terminal(rule, "end");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "end_context_block"), NULL));

  nonterminal = nonterminal_create(grammar, "parlist_or_void");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "parlist");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "baseclass");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "identifier");
  entry = rule_entry_terminal(rule, token_create(40, "("));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "setup_constructor"), NULL));
  entry = rule_entry_non_terminal(rule, "arglist_or_void");
  entry = rule_entry_terminal(rule, token_create(41, ")"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "func_call"), NULL));

  nonterminal = nonterminal_create(grammar, "param");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(105, "i"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "push"), NULL));

  nonterminal = nonterminal_create(grammar, "predicate");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "term");
  entry = rule_entry_non_terminal(rule, "termtail");

  nonterminal = nonterminal_create(grammar, "_assignment_or_call");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "assignment");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "instruction"), data_parse(5, "PushScope")));
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "deref"), NULL));
  entry = rule_entry_non_terminal(rule, "func_calls");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pop"), NULL));

  nonterminal = nonterminal_create(grammar, "throw");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(275584441, "throw"));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "throw_exception"), NULL));

  nonterminal = nonterminal_create(grammar, "query");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "init_query"), NULL));
  entry = rule_entry_terminal(rule, token_create(96, "`"));
  entry = rule_entry_non_terminal(rule, "query_params");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "query"), NULL));

  nonterminal = nonterminal_create(grammar, "term");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "factor");
  entry = rule_entry_non_terminal(rule, "factortail");

  nonterminal = nonterminal_create(grammar, "assignment");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(61, "="));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "assign"), NULL));

  nonterminal = nonterminal_create(grammar, "func_def");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "func_type");
  entry = rule_entry_terminal(rule, token_create(105, "i"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "push"), NULL));
  entry = rule_entry_terminal(rule, token_create(40, "("));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "bookmark"), NULL));
  entry = rule_entry_non_terminal(rule, "parlist_or_void");
  entry = rule_entry_terminal(rule, token_create(41, ")"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "rollup_list"), NULL));
  entry = rule_entry_non_terminal(rule, "func_block");

  nonterminal = nonterminal_create(grammar, "comprehension");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(193491852, "for"));
  entry = rule_entry_non_terminal(rule, "identifier");
  entry = rule_entry_terminal(rule, token_create(5863484, "in"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "defer_bookmarked_block"), NULL));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushconst"), data_parse(5, "int:0")));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "for"), NULL));
  entry = rule_entry_non_terminal(rule, "where_or_empty");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "comprehension"), NULL));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "end_loop"), NULL));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "set_variable"), data_parse(5, "varargs=bool:1")));

  nonterminal = nonterminal_create(grammar, "predicatetail");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "logic_op");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "infix_op"), NULL));
  entry = rule_entry_non_terminal(rule, "predicate");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "call_op"), NULL));
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "ternary");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "where");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(279128128, "where"));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "where"), NULL));

  nonterminal = nonterminal_create(grammar, "entrylist_or_empty");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "instruction_bookmark"), NULL));
  entry = rule_entry_non_terminal(rule, "entry");
  entry = rule_entry_non_terminal(rule, "comprehension_or_tail");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "yield_stmt");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(281535708, "yield"));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "instruction"), data_parse(5, "Yield")));

  nonterminal = nonterminal_create(grammar, "subscript");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(91, "["));
  entry = rule_entry_non_terminal(rule, "expr");
  entry = rule_entry_terminal(rule, token_create(93, "]"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "instruction"), data_parse(5, "Subscript")));
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "arglist_tail");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(44, ","));
  entry = rule_entry_non_terminal(rule, "arglist");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "for_loop");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(193491852, "for"));
  entry = rule_entry_non_terminal(rule, "identifier");
  entry = rule_entry_terminal(rule, token_create(5863484, "in"));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "for"), NULL));
  entry = rule_entry_non_terminal(rule, "statements");
  entry = rule_entry_non_terminal(rule, "end");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "end_loop"), NULL));

  nonterminal = nonterminal_create(grammar, "termtail");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "add_op");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "infix_op"), NULL));
  entry = rule_entry_non_terminal(rule, "term");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "call_op"), NULL));
  entry = rule_entry_non_terminal(rule, "termtail");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "reduction");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "_identifier");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(105, "i"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "push"), NULL));
  entry = rule_entry_non_terminal(rule, "_identifier_tail");

  nonterminal = nonterminal_create(grammar, "break");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(254582602, "break"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "break"), NULL));

  nonterminal = nonterminal_create(grammar, "ternary");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(63, "?"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "if"), NULL));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "test"), NULL));
  entry = rule_entry_non_terminal(rule, "expr");
  entry = rule_entry_terminal(rule, token_create(58, ":"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "else"), NULL));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "end_conditional"), NULL));

  nonterminal = nonterminal_create(grammar, "_func_call");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(40, "("));
  entry = rule_entry_non_terminal(rule, "arglist_or_void");
  entry = rule_entry_terminal(rule, token_create(41, ")"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "func_call"), NULL));

  nonterminal = nonterminal_create(grammar, "statements");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "statement");
  entry = rule_entry_non_terminal(rule, "statements");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "elif_seq");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "elif");
  entry = rule_entry_non_terminal(rule, "elif_seq");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "_baseclasses");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "baseclass");
  entry = rule_entry_non_terminal(rule, "baseclass_tail");

  nonterminal = nonterminal_create(grammar, "link_clause");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5861456, "->"));
  entry = rule_entry_terminal(rule, token_create(34, "\""));

  nonterminal = nonterminal_create(grammar, "new");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(193500239, "new"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "setup_function"), data_parse(5, "new")));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "incr"), NULL));
  entry = rule_entry_non_terminal(rule, "identifier");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushval_from_stack"), NULL));
  entry = rule_entry_non_terminal(rule, "_func_call");

  nonterminal = nonterminal_create(grammar, "call_or_empty");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "func_call");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "entry");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "incr"), NULL));

  nonterminal = nonterminal_create(grammar, "case_stmts");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "case_stmt");
  entry = rule_entry_non_terminal(rule, "case_stmts");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "pass");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(59, ";"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(2090608092, "pass"));

  nonterminal = nonterminal_create(grammar, "baseclass_tail");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(44, ","));
  entry = rule_entry_non_terminal(rule, "_baseclasses");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "func_calls");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "func_call");
  entry = rule_entry_non_terminal(rule, "_func_calls");

  nonterminal = nonterminal_create(grammar, "list");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(91, "["));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "setup_function"), data_parse(5, "list")));
  entry = rule_entry_non_terminal(rule, "entrylist_or_empty");
  entry = rule_entry_terminal(rule, token_create(93, "]"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "func_call"), NULL));

  nonterminal = nonterminal_create(grammar, "attrname");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(105, "i"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(34, "\""));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(39, "'"));

  nonterminal = nonterminal_create(grammar, "reduction_init");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(5861934, "<-"));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushval"), data_parse(6, "1")));
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushval"), data_parse(6, "0")));

  nonterminal = nonterminal_create(grammar, "attrlist_tail");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(44, ","));
  entry = rule_entry_non_terminal(rule, "attrlist");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "mult_op");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(42, "*"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(47, "/"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(94, "^"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(37, "%"));

  nonterminal = nonterminal_create(grammar, "import_stmt");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(79720320, "import"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "setup_function"), data_parse(5, "import")));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "incr"), NULL));
  entry = rule_entry_non_terminal(rule, "identifier");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushval_from_stack"), NULL));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "func_call"), NULL));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pop"), NULL));

  nonterminal = nonterminal_create(grammar, "signed_number");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "sign");
  entry = rule_entry_non_terminal(rule, "number");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "push_signed_val"), NULL));

  nonterminal = nonterminal_create(grammar, "case_stmt");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(2090140897, "case"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "incr"), NULL));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "case"), NULL));
  entry = rule_entry_terminal(rule, token_create(58, ":"));

  nonterminal = nonterminal_create(grammar, "add_op");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(43, "+"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(45, "-"));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(126, "~"));

  nonterminal = nonterminal_create(grammar, "func_call");
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "init_function"), NULL));
  entry = rule_entry_non_terminal(rule, "_func_call");

  nonterminal = nonterminal_create(grammar, "else");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(2090224750, "else"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "else"), NULL));
  entry = rule_entry_non_terminal(rule, "statements");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "where_or_empty");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "where");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "regexp");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(47, "/"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "setup_function"), data_parse(5, "regexp")));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "rollup_to"), data_parse(5, "/")));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushval_from_stack"), NULL));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "incr"), NULL));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "func_call"), NULL));

  nonterminal = nonterminal_create(grammar, "func_type");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(2090270321, "func"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushval"), data_parse(6, "0")));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(-627034807, "threadfunc"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushval"), data_parse(6, "1")));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(227293100, "generator"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushval"), data_parse(6, "2")));

  nonterminal = nonterminal_create(grammar, "cases");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "cases_seq");
  entry = rule_entry_non_terminal(rule, "default");

  nonterminal = nonterminal_create(grammar, "expr_atom");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(40, "("));
  entry = rule_entry_non_terminal(rule, "expr");
  entry = rule_entry_terminal(rule, token_create(41, ")"));
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "instruction"), data_parse(5, "PushScope")));
  entry = rule_entry_non_terminal(rule, "var_or_calls");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "new");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "constant");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "query");

  nonterminal = nonterminal_create(grammar, "sign");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(43, "+"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "push_tokenstring"), NULL));
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(45, "-"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "push_tokenstring"), NULL));
  rule = rule_create(nonterminal);
  rule_add_action(rule,
    grammar_action_create(
      grammar_resolve_function(grammar, "pushval"), data_parse(5, "+")));

  nonterminal = nonterminal_create(grammar, "while_loop");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(279132286, "while"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "start_loop"), NULL));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "test"), NULL));
  entry = rule_entry_non_terminal(rule, "statements");
  entry = rule_entry_non_terminal(rule, "end");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "end_loop"), NULL));

  nonterminal = nonterminal_create(grammar, "switch");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(482686839, "switch"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "if"), NULL));
  entry = rule_entry_non_terminal(rule, "expr");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "stash"), data_parse(6, "0")));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "new_counter"), NULL));
  entry = rule_entry_non_terminal(rule, "cases");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "discard_counter"), NULL));
  entry = rule_entry_non_terminal(rule, "end");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "end_conditional"), NULL));

  nonterminal = nonterminal_create(grammar, "assignment_or_call");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "identifier");
  entry = rule_entry_non_terminal(rule, "_assignment_or_call");

  nonterminal = nonterminal_create(grammar, "arglist");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "argument");
  entry = rule_entry_non_terminal(rule, "arglist_tail");

  nonterminal = nonterminal_create(grammar, "entrylist_tail");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(44, ","));
  entry = rule_entry_non_terminal(rule, "entrylist");
  rule = rule_create(nonterminal);

  nonterminal = nonterminal_create(grammar, "object");
  rule = rule_create(nonterminal);
  entry = rule_entry_terminal(rule, token_create(123, "{"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "setup_function"), data_parse(5, "object")));
  entry = rule_entry_non_terminal(rule, "attrlist_or_empty");
  entry = rule_entry_terminal(rule, token_create(125, "}"));
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "func_call"), NULL));

  nonterminal = nonterminal_create(grammar, "attrlist");
  rule = rule_create(nonterminal);
  entry = rule_entry_non_terminal(rule, "attrname");
  rule_entry_add_action(entry,
    grammar_action_create(
      grammar_resolve_function(grammar, "push"), NULL));
  entry = rule_entry_terminal(rule, token_create(58, ":"));
  entry = rule_entry_non_terminal(rule, "expr");
  entry = rule_entry_non_terminal(rule, "attrlist_tail");

  grammar_analyze(grammar);
  return grammar;
}

