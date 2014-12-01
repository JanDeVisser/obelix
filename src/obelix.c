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

#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <file.h>
#include <grammar.h>
#include <parser.h>
#include <script.h>
#include <str.h>

int parse(char *gname, char *tname) {
  file_t *     *file_grammar;
  reader_t     *greader;
  reader_t     *treader;
  script_t     *script;
  free_t        gfree, tfree;
  int           ret;

  if (!gname) {
    //greader = (reader_t *) str_wrap(bnf_grammar);
    greader = (reader_t *) file_open("/home/jan/Projects/obelix/etc/grammar.txt");
    gfree = (free_t) file_free;
  } else {
    greader = (reader_t *) file_open(gname);
    gfree = (free_t) file_free;
  }
  if (!tname) {
    treader = (reader_t *) file_create(fileno(stdin));
  } else {
    treader = (reader_t *) file_open(tname);
  }
  ret = -1;
  if (greader && treader) {
    ret = script_parse_execute(greader, treader);
    file_free((file_t *) treader);
    gfree(greader);
  }
  return ret;
}

int main(int argc, char **argv) {
  char *grammar;
  char *debug;
  int   opt;
  int   ret;

  grammar = NULL;
  debug = NULL;
  while ((opt = getopt(argc, argv, "g:d:")) != -1) {
    switch (opt) {
      case 'g':
        grammar = optarg;
        break;
      case 'd':
        debug = optarg;
        break;
    }
  }
  if (debug) {
    debug("debug optarg: %s", debug);
    if (strstr(debug, "grammar")) {
      grammar_debug = 1;
      debug("Turned on grammar debugging");
    }
    if (strstr(debug, "parser")) {
      parser_debug = 1;
      debug("Turned on parser debugging");
    }
    if (strstr(debug, "script")) {
      script_debug = 1;
      debug("Turned on script debugging");
    }
  }
  ret = parse(grammar, (argc > optind) ? argv[optind] : NULL);
  debug("Exiting with exit code %d", ret);
  return ret;
}
