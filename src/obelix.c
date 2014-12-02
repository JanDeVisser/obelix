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
#include <script.h>

int main(int argc, char **argv) {
  char           *grammar;
  char           *debug;
  char           *basepath;
  int             opt;
  scriptloader_t *loader;
  data_t         *ret;
  int             retval;

  grammar = NULL;
  debug = NULL;
  basepath = NULL;
  if (getenv("OBL_PATH")) {
    basepath = strdup(getenv("OBL_PATH"));
  } else {
    basepath = getcwd(NULL, 0);
  }
  while ((opt = getopt(argc, argv, "g:d:")) != -1) {
    switch (opt) {
      case 'g':
        grammar = optarg;
        break;
      case 'd':
        debug = optarg;
        break;
      case 'p':
        free(basepath);
        basepath = strdup(optarg);
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
  if (argc == optind) {
    fprintf(stderr, "Usage: obelix <filename>\n");
    exit(1);
  }
  loader = scriptloader_create(basepath, grammar);
  free(basepath);
  ret = scriptloader_execute(loader, argv[optind]);
  debug("Exiting with exit code %s", data_tostring(ret));
  retval = (ret && data_type(ret) == Int) ? (int) ret -> intval : 0;
  data_free(ret);
  return retval;
}
