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

//#ifdef _GNU_SOURCE
//#undef _GNU_SOURCE
//#endif

#include <data.h>
#include <logging.h>
#include <re.h>
#include <typedescr.h>
#include <wrapper.h>

static void     _regexp_init(void) __attribute__((constructor));
static data_t * _regexp_create(data_t *, char *, array_t *, dict_t *);
static data_t * _regexp_match(data_t *, char *, array_t *, dict_t *);
static data_t * _regexp_replace(data_t *, char *, array_t *, dict_t *);


static vtable_t _wrapper_vtable_re[] = {
  { .id = FunctionFactory,  .fnc = (void_t) regexp_create },
  { .id = FunctionCopy,     .fnc = (void_t) regexp_copy },
  { .id = FunctionCmp,      .fnc = (void_t) regexp_cmp },
  { .id = FunctionFree,     .fnc = (void_t) regexp_free },
  { .id = FunctionToString, .fnc = (void_t) regexp_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

int Regexp = -1;
int debug_regexp = 0;

static methoddescr_t _methoddescr_re[] = {
  { .type = Any,    .name = "regexp",  .method = _regexp_create,  .argtypes = { String, String, Any },    .minargs = 1, .maxargs = 2, .varargs = 0 },
  { .type = -1,     .name = "match",   .method = _regexp_match,   .argtypes = { String, Any, Any },       .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "replace", .method = _regexp_replace, .argtypes = { String, List, NoType },   .minargs = 2, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,        .argtypes = { NoType, NoType, NoType },     .minargs = 0, .varargs = 0 },
};

/* ------------------------------------------------------------------------ */

void _regexp_init(void) {
  int ix;
  
  logging_register_category("regexp", &debug_regexp);
  Regexp = wrapper_register(Regexp, "regexp", _wrapper_vtable_re);
  for (ix = 0; _methoddescr_re[ix].type != NoType; ix++) {
    if (_methoddescr_re[ix].type == -1) {
      _methoddescr_re[ix].type = Regexp;
    }
  }
  typedescr_register_methods(_methoddescr_re);
}

/* ------------------------------------------------------------------------ */

re_t * regexp_create(va_list args) {
  re_t *ret = NEW(re_t);
  int   icase;
  int   flags = REG_EXTENDED;
  char  msgbuf[100];
  int   retval;

  ret -> pattern = strdup(va_arg(args, char *));
  ret -> flags = va_arg(args, char *);
  if (ret -> flags) {
    ret -> flags = strdup(ret -> flags);
    if (strchr(ret -> flags, 'i')) {
      flags |= REG_ICASE;
    }
  }
  if (debug_regexp) {
    debug("Created re %s", regexp_tostring(ret));
  }
  if (retval = regcomp(&ret -> compiled, ret -> pattern, flags)) {
    regerror(retval, &ret -> compiled, msgbuf, sizeof(msgbuf));
    debug("Error: %s", msgbuf);
  }
  ret -> refs = 1;
  ret -> str = NULL;
  return ret;
}

void regexp_free(re_t *regex) {
  if (regex && (--regex -> refs <= 0)) {
    free(regex -> pattern);
    free(regex -> flags);
    regfree(&regex -> compiled);
    free(regex -> str);
    free(regex);
  }
}

re_t * regexp_copy(re_t *regex) {
  if (regex) {
    regex -> refs++;
  }
  return regex;
}

int regexp_cmp(re_t *regex1, re_t *regex2) {
  return strcmp(regex1 -> pattern, regex2 -> pattern);
}

char * regexp_tostring(re_t *regex) {
  if (!regex -> str) {
    asprintf(&regex -> str, "/%s/%s", regex -> pattern, regex -> flags);
  }
  return regex -> str;
}

data_t * regexp_match(re_t *re, char *str) {
  regmatch_t  rm[2];
  char       *ptr = str;
  int         len = strlen(str);
  char       *work = (char *) new(len + 1);
  array_t    *matches = data_array_create(4);
  data_t     *ret;

  if (debug_regexp) {
    debug("%s .match(%s)", regexp_tostring(re), str);
  }
  while (!regexec(&re -> compiled, ptr, 1, rm, 0)) {
    memset(work, 0, len + 1);
    strncpy(work, ptr + rm[0].rm_so, rm[0].rm_eo - rm[0].rm_so);
    if (debug_regexp) {
      debug("%s .match(%s): match at [%d-%d]: %s", regexp_tostring(re), ptr, rm[0].rm_so, rm[0].rm_eo, work);
    }
    array_push(matches, data_create(String, work));
    ptr += rm[0].rm_eo;
  }
  if (!array_size(matches)) {
    if (debug_regexp) {
      debug("%s .match(%s): No matches", regexp_tostring(re), str);
    }
    ret = data_false();
  } else if (array_size(matches) == 1) {
    if (debug_regexp) {
      debug("%s .match(%s): One match", regexp_tostring(re), str);
    }
    ret = data_copy(data_array_get(matches, 0));
  } else {
    if (debug_regexp) {
      debug("%s .match(%s): %d matches", regexp_tostring(re), str, array_size(matches));
    }
    ret = data_create_list(matches);
  }
  array_free(matches);
  free(work);
  return ret;
}

data_t * regexp_replace(re_t *re, char *str, array_t *replacements) {
  return data_null();
}

/* ------------------------------------------------------------------------ */

data_t * _regexp_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *pattern = data_array_get(args, 0);
  char   *flags;
  data_t *ret;
  
  (void) name;
  (void) kwargs;
  if (debug_regexp) {
    debug("_regexp_create(%s))", data_tostring(pattern));
  }
  flags = (array_size(args) == 2) ? data_tostring(data_array_get(args, 1)) : "";
  ret = data_create(Regexp, data_tostring(pattern), flags);
  return ret;
}


data_t * _regexp_match(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *str = data_array_get(args, 0);
  re_t   *re = data_regexpval(self);
  
  (void) name;
  (void) kwargs;
  if (debug_regexp) {
    debug("_regexp_match(%s, %s))", regexp_tostring(re), data_tostring(str));
  }
  return regexp_match(re, data_tostring(str));
}

data_t * _regexp_replace(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *str = data_array_get(args, 0);
  data_t *replacements = data_array_get(args, 1);
  re_t   *re = data_regexpval(self);
  
  (void) name;
  (void) kwargs;
  return regexp_replace(re, data_tostring(str), data_arrayval(replacements));
}
