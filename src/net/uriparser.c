/*
 * /obelix/src/net/uriparser.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#include "libnet.h"
#include <stdlib.h>
#include <unistd.h>

static uri_t * _uri_dump(uri_t *);

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

#define nullcheck(v)  ((v) ? (v) : "(null)")

uri_t * _uri_dump(uri_t *uri) {
  if (!uri -> error) {
    printf("uri: '%s' =>\n"
           "  scheme: '%s'\n"
           "  user: '%s' password: '%s'\n"
           "  host: '%s' port: %d\n"
           "  path: '%s'\n"
           "  query: %s\n"
           "  fragment: '%s'\n",
           uri -> _d.str, nullcheck(uri -> scheme), nullcheck(uri -> user),
           nullcheck(uri -> password), nullcheck(uri -> host), uri -> port,
           name_tostring_sep(uri -> path, "/"),
           dict_tostring_custom(uri -> query, "{\n", "    \"%s\": \"%s\"", ",\n  ", "\n  }"),
           nullcheck(uri -> fragment));
  } else {
    printf("uri: '%s' => error %s\n",
           uri -> _d.str, data_tostring(uri -> error));
  }
  return uri;
}

int main(int argc, char **argv) {
  uri_t *uri;
  int    ix;
  int    opt;
  char  *debug = NULL;


  while ((opt = getopt(argc, argv, "d:v:")) != -1) {
    switch (opt) {
      case 'd':
        debug = optarg;
        break;
      case 'v':
        logging_set_level(atoi(optarg));
        break;
    }
  }
  debug_settings(debug);

  for (ix = optind; ix < argc; ix++) {
    uri = uri_create(argv[ix]);
    _uri_dump(uri);
    uri_free(uri);
  }
  return 0;
}
