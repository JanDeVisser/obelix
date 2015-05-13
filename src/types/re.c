/*
 * /obelix/src/types/re.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <data.h>
#include <re.h>
#include <typedescr.h>
#include <wrapper.h>

static void     _re_init(void) __attribute__((constructor));
static data_t * _re_create(data_t *, char *, array_t *, dict_t *);
static data_t * _re_match(data_t *, char *, array_t *, dict_t *);
static data_t * _re_replace(data_t *, char *, array_t *, dict_t *);


static vtable_t _wrapper_vtable_re[] = {
  { .id = FunctionFactory,  .fnc = (void_t) re_create },
  { .id = FunctionCopy,     .fnc = (void_t) re_copy },
  { .id = FunctionCmp,      .fnc = (void_t) re_cmp },
  { .id = FunctionFree,     .fnc = (void_t) re_free },
  { .id = FunctionToString, .fnc = (void_t) re_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

int Regex = -1;
int re_debug = 0;

static methoddescr_t _methoddescr_re[] = {
  { .type = Any,    .name = "regex",   .method = _re_create,  .argtypes = { String, Any, Any },       .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "match",   .method = _re_match,   .argtypes = { String, Any, Any },       .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "replace", .method = _re_replace, .argtypes = { String, List, NoType },   .minargs = 2, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,        .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

/* ------------------------------------------------------------------------ */

void _re_init(void) {
  int ix;
  
  logging_register_category("regex", &re_debug);
  Regex = wrapper_register(Regex, "regex", _wrapper_vtable_re);
  for (ix = 0; _methoddescr_re[ix].type != NoType; ix++) {
    if (_methoddescr_re[ix].type == -1) {
      _methoddescr_re[ix].type = Regex;
    }
  }
  typedescr_register_methods(_methoddescr_re);
}

/* ------------------------------------------------------------------------ */

re_t * re_create(va_list args) {
  re_t *ret = NEW(re_t);
  int   icase;
  int   flags;
  
  ret -> pattern = strdup(va_arg(args, char *));
  memset(ret -> flags, 0, sizeof(ret -> flags));
  icase = va_arg(args, int);
  flags = (icase) ? REG_ICASE : 0;
  if (icase) {
    strcat(ret -> flags, "i");
  }
  flags |= REG_EXTENDED; 
  if (regcomp(&ret -> compiled, ret -> pattern, flags)) {
    //
  }
  ret -> refs = 1;
  ret -> str = NULL;
  return ret;
}

void re_free(re_t *regex) {
  if (regex && (--regex -> refs <= 0)) {
    free(regex -> pattern);
    regfree(&regex -> compiled);
    free(regex -> str);
    free(regex);
  }
}

re_t * re_copy(re_t *regex) {
  if (regex) {
    regex -> refs++;
  }
  return regex;
}

int re_cmp(re_t *regex1, re_t *regex2) {
  return strcmp(regex1 -> pattern, regex2 -> pattern);
}

char * re_tostring(re_t *regex) {
  if (!regex -> str) {
    asprintf(&regex -> str, "/%s/%s", regex -> pattern, regex -> flags);
  }
  return regex -> str;
}

data_t * re_match(re_t *re, char *str) {
  regmatch_t  rm[2];
  char       *ptr = str;
  int         len = strlen(str);
  char       *work = (char *) new(len + 1);
  array_t    *matches = data_array_create(4);
  data_t     *ret;
  
  while (!regexec(&re -> compiled, ptr, 1, rm, 0)) {
    memset(work, 0, len + 1);
    strncpy(work, ptr + rm[0].rm_so, rm[0].rm_eo - rm[0].rm_so);
    array_push(matches, data_create(String, work));
    ptr += rm[0].rm_eo;
  }
  if (!array_size(matches)) {
    ret = data_false();
  } else if (array_size(matches) == 1) {
    ret = data_copy(data_array_get(matches, 0));
  } else {
    ret = data_create_list(matches);
  }
  array_free(matches);
  free(work);
  return ret;
}

data_t * re_replace(re_t *re, char *str, array_t *replacements) {
  return data_null();
}

/* ------------------------------------------------------------------------ */

data_t * _re_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *pattern = data_array_get(args, 0);
  data_t *ret = data_create(Regex, data_tostring(pattern));
  
  (void) name;
  (void) kwargs;
  return ret;
}


data_t * _re_match(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *str = data_array_get(args, 0);
  re_t   *re = data_regexval(self);
  
  (void) name;
  (void) kwargs;
  return re_match(re, data_charval(str));
}

data_t * _re_replace(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *str = data_array_get(args, 0);
  data_t *replacements = data_array_get(args, 1);
  re_t   *re = data_regexval(self);
  
  (void) name;
  (void) kwargs;
  return re_replace(re, data_tostring(str), data_arrayval(replacements));
}
