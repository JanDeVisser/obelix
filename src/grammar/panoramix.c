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

#include <application.h>
#include <file.h>
#include "libgrammar.h"
#include <grammarparser.h>

int panoramix_debug;

static app_description_t   _app_descr_panoramix = {
    .name        = "panoramix",
    .shortdescr  = "Grammar parser",
    .description = "Panoramix will convert a formal grammar file into C code.",
    .legal       = "(c) Jan de Visser <jan@finiandarcy.com> 2014-2017",
    .options     = {
        { .longopt = "grammar", .shortopt = 'g', .description = "Grammar file", .flags = CMDLINE_OPTION_FLAG_REQUIRED_ARG },
        { .longopt = "syspath", .shortopt = 's', .description = "System path",  .flags = CMDLINE_OPTION_FLAG_REQUIRED_ARG },
        { .longopt = NULL,      .shortopt = 0,   .description = NULL,           .flags = 0 }
    }
};

grammar_t * load(char *sys_dir, char *grammarpath) {
  size_t            len;
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
  application_t *app;
  grammar_t     *grammar;
  char          *grammarfile = NULL;
  char          *syspath = NULL;

  app = application_create(&_app_descr_panoramix, argc, argv);
  logging_register_module(panoramix);

  if (application_has_option(app, "grammar")) {
    grammarfile = data_tostring(application_get_option(app, "grammar"));
  }
  if (application_has_option(app, "syspath")) {
    syspath = data_tostring(application_get_option(app, "syspath"));
  }
  if (!grammarfile) {
    fprintf(stderr, "No grammar file specified.\n");
    exit(1);
  }
  grammar = load(syspath, grammarfile);
  if (grammar) {
    grammar_dump(grammar);
    grammar_free(grammar);
  }
  application_terminate();
  return 0;
}
