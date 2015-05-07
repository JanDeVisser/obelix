/*
 * /obelix/src/parser/logging.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <dict.h>
#include <logging.h>

static void _logging_init(void) __attribute__((constructor(101)));
static void _logging_set(char *, int);

typedef struct _logcategory {
  char *name;
  int  *flag;
} logcategory_t;

static dict_t *_categories = NULL;
  
/* ------------------------------------------------------------------------ */

void _logging_init(void) {
  _categories = strvoid_dict_create();
}

int * _logging_set_reducer(logcategory_t *cat, int *value) {
  if (*value) {
    debug("Enabling %s logging", cat -> name);
  }
  *(cat -> flag) = *value;
  return value;
}

void _logging_set(char *category, int value) {
  logcategory_t *cat;
  
  if (strcmp(category, "all")) {
    cat = dict_get(_categories, category);
    if (cat) {
      debug("%s %s logging", (value) ? "Enabling" : "Disabling", cat -> name);
      *(cat -> flag) = value;
    }
  } else {
    dict_reduce_values(_categories, (reduce_t) _logging_set_reducer, &value);
  }
}

/* ------------------------------------------------------------------------ */

void logging_register_category(char *name, int *flag) {
  logcategory_t *cat = NEW(logcategory_t);
  
  cat -> name = strdup(name);
  cat -> flag = flag;
  *flag = 0;
  dict_put(_categories, strdup(name), cat);
}

void logging_reset(void) {
  int value = 0;
  dict_reduce_values(_categories, (reduce_t) _logging_set_reducer, &value);
}

void logging_enable(char *category) {
  _logging_set(category, 1);
}

void logging_disable(char *category) {
  _logging_set(category, 0);
}
