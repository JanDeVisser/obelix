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

#include <stdio.h>
#include <string.h>

#include "libcore.h"
#include <data.h>
#include <exception.h>
#include <re.h>

static void     _regexp_init(void);
static re_t *   _regexp_new(re_t *, va_list);
static void     _regexp_free(re_t *);
static int      _regexp_cmp(re_t *, re_t *);
static data_t * _regexp_call(re_t *, arguments_t *);
static data_t * _regexp_interpolate(re_t *, arguments_t *);
static data_t * _regexp_match(data_t *, char *, arguments_t *);
static data_t * _regexp_replace(data_t *, char *, arguments_t *);
static data_t * _regexp_compile(re_t *);
static void *   _regexp_reduce_children(re_t *, reduce_t, void *);

extern data_t * _regexp_create(char *, arguments_t *);

static vtable_t _vtable_Regexp[] = {
    { .id = FunctionNew,         .fnc = (void_t) _regexp_new },
    { .id = FunctionFree,        .fnc = (void_t) _regexp_free },
    { .id = FunctionCmp,         .fnc = (void_t) _regexp_cmp },
    { .id = FunctionCall,        .fnc = (void_t) _regexp_call },
    { .id = FunctionInterpolate, .fnc = (void_t) _regexp_interpolate },
    { .id = FunctionReduce,      .fnc = (void_t) _regexp_reduce_children },
    { .id = FunctionNone,        .fnc = NULL }
};

int Regexp = -1;
int regexp_debug = 0;

static methoddescr_t _methods_Regexp[] = {
  { .type = -1,     .name = "match",   .method = _regexp_match,   .argtypes = { String, Any, Any },       .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "replace", .method = _regexp_replace, .argtypes = { String, List, NoType },   .minargs = 2, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,            .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

/* ------------------------------------------------------------------------ */

void _regexp_init(void) {
  if (Regexp < 1) {
    logging_register_module(regexp);
    typedescr_register_with_methods(Regexp, re_t);
  }
}

re_t * _regexp_new(re_t *regex, va_list args) {
  char *pattern = va_arg(args, char *);
  char *flags = va_arg(args, char *);

  regex -> pattern = str_printf( "(%s)", pattern);
  regex -> re_flags = REG_EXTENDED;
  if (flags) {
    regex -> flags = strdup(flags);
    if (strchr(regex -> flags, 'i')) {
      regex -> re_flags |= REG_ICASE;
    }
  }
  regex -> is_compiled = FALSE;
  asprintf(&regex->_d.str, "/%s/%s",
           str_tostring(regex -> pattern), (regex -> flags) ? (regex -> flags) : "");
  data_set_string_semantics(regex, StrSemanticsStatic);
  debug(regexp, "Created re %s", regexp_tostring(regex));
  return regex;
}

void _regexp_free(re_t *regex) {
  if (regex) {
    free(regex -> flags);
    if (regex -> is_compiled) {
      regfree(&regex -> compiled);
    }
  }
}

int _regexp_cmp(re_t *regex1, re_t *regex2) {
  return str_cmp(regex1 -> pattern, regex2 -> pattern);
}

data_t * _regexp_call(re_t *re, arguments_t *args) {
  char *str = arguments_arg_tostring(args, 0);

  debug(regexp, "_regexp_call(%s, %s))", regexp_tostring(re), str);
  return regexp_match(re, str);
}

data_t * _regexp_interpolate(re_t *re, arguments_t *args) {
  str_t  *pat;

  debug(regexp, "_regexp_interpolate(%s, %s)",
      regexp_tostring(re), arguments_tostring(args));
  pat = str_format(str_chars(re -> pattern), args);
  str_free(re -> pattern);
  re -> pattern = pat;
  if (re -> is_compiled) {
    regfree(&re -> compiled);
    re -> is_compiled = FALSE;
  }
  debug(regexp, "_regexp_interpolate() => %s", regexp_tostring(re));
  return (data_t *) re;
}

data_t * _regexp_compile(re_t *re) {
  int retval;

  if (!re -> is_compiled) {
    if ((retval = regcomp(&re -> compiled, str_chars(re -> pattern), re -> re_flags))) {
      char msgbuf[1000];

      regerror(retval, &re -> compiled, msgbuf, sizeof(msgbuf));
      debug(regexp, "Error: %s", msgbuf);
      return data_exception(ErrorSyntax, msgbuf);
    }
    re -> is_compiled = TRUE;
  }
  return (data_t *) re;
}

void * _regexp_reduce_children(re_t *re, reduce_t reducer, void *ctx) {
  return reducer(re->pattern, ctx);
}


/* ------------------------------------------------------------------------ */

re_t * regexp_create(char *pattern, char *flags) {
  _regexp_init();
  return (re_t *) data_create(Regexp, pattern, flags);
}

data_t * regexp_match(re_t *re, char *str) {
  regmatch_t  rm[2];
  char       *ptr = str;
  size_t      len = strlen(str);
  char       *work = stralloc(len);
  array_t    *matches = data_array_create(4);
  data_t     *ret;

  debug(regexp, "%s .match(%s)", regexp_tostring(re), str);
  if (data_is_exception(ret = _regexp_compile(re))) {
    return ret;
  }
  while (!regexec(&re -> compiled, ptr, 1, rm, 0)) {
    memset(work, 0, len + 1);
    strncpy(work, ptr + rm[0].rm_so, (size_t) (rm[0].rm_eo - rm[0].rm_so));
    debug(regexp, "%s .match(%s): match at [%d-%d]: %s", regexp_tostring(re), ptr, rm[0].rm_so, rm[0].rm_eo, work);
    array_push(matches, str_to_data(work));
    ptr += rm[0].rm_eo;
  }
  if (!array_size(matches)) {
    debug(regexp, "%s .match(%s): No matches", regexp_tostring(re), str);
    ret = data_false();
  } else if (array_size(matches) == 1) {
    debug(regexp, "%s .match(%s): One match", regexp_tostring(re), str);
    ret = data_copy(data_array_get(matches, 0));
  } else {
    debug(regexp, "%s .match(%s): %d matches", regexp_tostring(re), str, array_size(matches));
    ret = (data_t *) datalist_create(matches);
  }
  array_free(matches);
  free(work);
  return ret;
}

/*
 * FIXME Implement
 */
data_t * regexp_replace(re_t *re, char *str, array_t *replacements) {
  return data_null();
}

/* ------------------------------------------------------------------------ */

data_t * _regexp_create(char _unused_ *name, arguments_t *args) {
  char *pattern = arguments_arg_tostring(args, 0);
  char *flags;
  re_t *ret;

  debug(regexp, "_regexp_create(%s))", pattern);
  flags = (arguments_args_size(args) == 2) ? arguments_arg_tostring(args, 1) : "";
  ret = regexp_create(pattern, flags);
  return (data_t *) ret;
}


data_t * _regexp_match(data_t *self, char _unused_ *name, arguments_t *args) {
  return _regexp_call(data_as_regexp(self), args);
}

data_t * _regexp_replace(data_t *self, char _unused_ *name, arguments_t *args) {
  data_t *str = data_uncopy(arguments_get_arg(args, 0));
  data_t *replacements = data_uncopy(arguments_get_arg(args, 1));
  re_t   *re = data_as_regexp(self);

  return regexp_replace(re, data_tostring(str), data_as_array(replacements));
}
