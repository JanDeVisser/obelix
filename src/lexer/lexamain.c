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
#include <unistd.h>
#include <file.h>
#include <lexa.h>

int main(int argc, char **argv) {
  lexa_t   *lexa;
  char     *scratch = NULL;
  int       opt;

  lexa = lexa_create();
  while ((opt = getopt(argc, argv, "d:v:s:")) != -1) {
    switch (opt) {
      case 'd':
        if (!lexa -> debug) {
          lexa -> debug = strdup(optarg);
        } else {
          asprintf(&scratch, "%s,%s", lexa -> debug, optarg);
          free(lexa -> debug);
          lexa -> debug = scratch;
        }
        break;
      case 's':
        lexa_add_scanner(lexa, optarg);
        break;
      case 'v':
        lexa -> log_level = (log_level_t) atoi(optarg);
        break;
    }
  }
  lexa_debug_settings(lexa);
  if (optind >= argc) {
    lexa -> stream = (data_t *) file_create(1);
  } else {
    lexa -> stream = (data_t *) file_open(argv[optind]);
  }
  lexa_build_lexer(lexa);
  lexa_tokenize(lexa);
  lexa_free(lexa);
  return 0;
}
