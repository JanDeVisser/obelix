/*
 * obelix.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
 * 
 * This file is part of Obelix.
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
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <data.h>
#include <file.h>
#include <lexer.h>

#if 0
#include <grammarparser.h>
int main(int argc, char **argv) {
  file_t           *gf;
  grammar_parser_t *gp;
  grammar_t        *g;

  grammar_debug = 1;

  gf = file_open("/home/jan/Projects/obelix/etc/grammar.txt");
  gp = grammar_parser_create(gf);
  gp -> dryrun = TRUE;
  g = grammar_parser_parse(gp);
  grammar_parser_free(gp);
  grammar_free(g);
  file_free(gf);
}
#endif

#include <namespace.h>
#include <script.h>

void debug_settings(char *debug) {
  int debug_all = 0;
  
  if (debug) {
    debug("debug optarg: %s", debug);
    debug_all = strstr(debug, "all") != NULL;
    if (debug_all || strstr(debug, "grammar")) {
      grammar_debug = 1;
      debug("Turned on grammar debugging");
    }
    if (debug_all || strstr(debug, "parser")) {
      parser_debug = 1;
      debug("Turned on parser debugging");
    }
    if (debug_all || strstr(debug, "script")) {
      script_debug = 1;
      debug("Turned on script debugging");
    }
    if (debug_all || strstr(debug, "file")) {
      file_debug = 1;
      debug("Turned on file debugging");
    }
    if (debug_all || strstr(debug, "namespace")) {
      ns_debug = 1;
      debug("Turned on namespace debugging");
    }
    if (debug_all || strstr(debug, "resolution")) {
      res_debug = 1;
      debug("Turned on name resolution debugging");
    }
    if (debug_all || strstr(debug, "lexer")) {
      lexer_debug = 1;
      debug("Turned on name lexer debugging");
    }
  }
}

int main(int argc, char **argv) {
  char           *grammar = NULL;
  char           *debug = NULL;
  char           *basepath;
  char           *syspath = NULL;
  int             opt;
  scriptloader_t *loader;
  data_t         *ret;
  script_t       *script;
  int             retval;

  while ((opt = getopt(argc, argv, "s:g:d:p:l:")) != -1) {
    switch (opt) {
      case 's':
        syspath = optarg;
        break;
      case 'g':
        grammar = optarg;
        break;
      case 'd':
        debug = optarg;
        break;
      case 'p':
        basepath = strdup(optarg);
        break;
      case 'l':
        log_level = atoi(optarg);
        break;
    }
  }
  if (argc == optind) {
    fprintf(stderr, "Usage: obelix <filename>\n");
    exit(1);
  }
  debug_settings(debug);
  
  if (!basepath) {
    basepath = getcwd(NULL, 0);
  }
  
  loader = scriptloader_create(syspath, basepath, grammar);
  free(basepath);
  ret = scriptloader_load(loader, argv[optind]);
  if (data_is_script(ret)) {
    script = script_copy(data_scriptval(ret));
    data_free(ret);
    ret = script_execute(script, NULL, NULL, NULL);
  }
  debug("Exiting with exit code %s", data_tostring(ret));
  retval = (ret && data_type(ret) == Int) ? (int) ret -> intval : 0;
  data_free(ret);
  script_free(script);
  scriptloader_free(loader);
  return retval;
}

