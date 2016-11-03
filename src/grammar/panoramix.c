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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <data.h>
#include <file.h>
#include <grammar.h>
#include <grammarparser.h>
#include <logging.h>

int panoramix_debug;

void debug_settings(char *debug) {
  array_t *cats;
  int      ix;

  if (debug) {
    _debug("debug optarg: %s", debug);
    cats = array_split(debug, ",");
    for (ix = 0; ix < array_size(cats); ix++) {
      logging_enable(str_array_get(cats, ix));
    }
  }
}

grammar_t * load(char *sys_dir, char *grammarpath) {
  int               len;
  char             *system_dir;
  char             *grammar_p = NULL;
  grammar_t        *ret;
  grammar_parser_t *gp;
  file_t           *file;

  if (!sys_dir) {
    sys_dir = getenv(OBL_DIR);
  }
  if (!sys_dir) {
    sys_dir = OBELIX_DATADIR;
  }
  len = strlen(sys_dir);
  system_dir = (char *) new (len + ((*(sys_dir + (len - 1)) != '/') ? 2 : 1));
  strcpy(system_dir, sys_dir);
  if ((*system_dir + (strlen(system_dir) - 1)) != '/') {
    strcat(system_dir, "/");
  }

  if (!grammarpath) {
    grammar_p = (char *) new(strlen(system_dir) + strlen("grammar.txt") + 1);
    strcpy(grammar_p, system_dir);
    strcat(grammar_p, "grammar.txt");
    grammarpath = grammar_p;
  }
  debug(panoramix, "system dir: %s", system_dir);
  debug(panoramix, "grammar file: %s", grammarpath);

  file = file_open(grammarpath);
  assert(file_isopen(file));
  gp = grammar_parser_create((data_t *) file);
  gp -> dryrun = 1;
  ret = grammar_parser_parse(gp);
  grammar_parser_free(gp);
  if (!grammar_analyze(ret)) {
    grammar_free(ret);
    ret = NULL;
  } else {
    info("  Loaded grammar");
  }
  file_free(file);
  free(grammar_p);
  return ret;
}

int main(int argc, char **argv) {
  grammar_t *grammar;
  char      *grammarfile = NULL;
  char      *debug = NULL;
  char      *syspath = NULL;
  int        opt;

  logging_register_category("panoramix", &panoramix_debug);
  while ((opt = getopt(argc, argv, "s:g:d:v:")) != -1) {
    switch (opt) {
      case 's':
        syspath = optarg;
        break;
      case 'g':
        grammarfile = optarg;
        break;
      case 'd':
        debug = optarg;
        break;
      case 'v':
        logging_set_level(atoi(optarg));
        break;
    }
  }
  debug_settings(debug);

  grammar = load(syspath, grammarfile);
  if (grammar) {
    grammar_dump(grammar);
    grammar_free(grammar);
  }
  return 0;
}
