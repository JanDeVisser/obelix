/*
 * obelix/src/lexer/lexamain.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <application.h>
#include <file.h>
#include <lexa.h>

static app_description_t _app_descr_lexa = {
    .name        = "lexa",
    .shortdescr  = "Generic lexer",
    .description = "Read an input stream and tokenize it",
    .legal       = "(c) Jan de Visser <jan@finiandarcy.com> 2014-2017",
    .options     = {
        { .longopt = "scanner", .shortopt = 's', .description = "Add a scanner", .flags = CMDLINE_OPTION_FLAG_REQUIRED_ARG | CMDLINE_OPTION_FLAG_MANY_ARG },
        { .longopt = NULL,      .shortopt = 0,   .description = NULL,            .flags = 0 }
    }
};

int main(int argc, char **argv) {
  application_t *app;
  lexa_t        *lexa;
  datalist_t    *scanners;
  int            ix;
  char          *scratch = NULL;
  int            opt;

  app = application_create(&_app_descr_lexa, argc, argv);
  lexa = lexa_create();

  scanners = (datalist_t *) application_get_option(app, "scanner");
  for (ix = 0; ix < datalist_size(scanners); ix++) {
    lexa_add_scanner(lexa, data_tostring(datalist_get(scanners, ix)));
  }

  if (!application_has_args(app)) {
    lexa -> stream = (data_t *) file_create(1);
  } else {
    lexa -> stream = (data_t *) file_open(data_tostring(application_get_arg(app, 0)));
  }
  lexa_build_lexer(lexa);
  lexa_tokenize(lexa);
  return 0;
}
