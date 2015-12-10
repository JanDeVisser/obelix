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
#include <exception.h>
#include <re.h>
#include <typedescr.h>

#include <stdio.h>

static void     _regexp_init(void) __attribute__((constructor));
static char *   _regexp_allocstring(re_t *);
static void     _regexp_free(re_t *);
static data_t * _regexp_create(data_t *, char *, array_t *, dict_t *);
static data_t * _regexp_call(re_t *, array_t *, dict_t *);
static data_t * _regexp_interpolate(re_t *, array_t *, dict_t *);
static data_t * _regexp_match(data_t *, char *, array_t *, dict_t *);
static data_t * _regexp_replace(data_t *, char *, array_t *, dict_t *);
static data_t * _regexp_compile(re_t *);

static vtable_t _vtable_re[] = {
  { .id = FunctionCmp,         .fnc = (void_t) regexp_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _regexp_free },
  { .id = FunctionAllocString, .fnc = (void_t) _regexp_allocstring },
  { .id = FunctionCall,        .fnc = (void_t) _regexp_call },
  { .id = FunctionInterpolate, .fnc = (void_t) _regexp_interpolate },
  { .id = FunctionNone,        .fnc = NULL }
};

int Regexp = -1;
int debug_regexp = 0;

static methoddescr_t _methoddescr_re[] = {
  { .type = Any,    .name = "regexp",  .method = _regexp_create,  .argtypes = { String, String, Any },    .minargs = 1, .maxargs = 2, .varargs = 0 },
  { .type = -1,     .name = "match",   .method = _regexp_match,   .argtypes = { String, Any, Any },       .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "replace", .method = _regexp_replace, .argtypes = { String, List, NoType },   .minargs = 2, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,            .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

/* ------------------------------------------------------------------------ */

void _regexp_init(void) {
  logging_register_category("regexp", &debug_regexp);
  Regexp = typedescr_create_and_register(Regexp, "regexp", _vtable_re, _methoddescr_re);
}

char * _regexp_allocstring(re_t *regex) {
  char *buf;

  asprintf(&buf, "/%s/%s",
	   str_tostring(regex -> pattern),
	   (regex -> flags) ? (regex -> flags) : "");
  return buf;
}

void _regexp_free(re_t *regex) {
  if (regex) {
    str_free(regex -> pattern);
    free(regex -> flags);
    if (regex -> is_compiled) {
      regfree(&regex -> compiled);
    }
  }
}

data_t * _regexp_call(re_t *re, array_t *args, dict_t *kwargs) {
  data_t *str = data_array_get(args, 0);

  (void) kwargs;
  if (debug_regexp) {
    debug("_regexp_call(%s, %s))", regexp_tostring(re), data_tostring(str));
  }
  return regexp_match(re, data_tostring(str));
}

data_t * _regexp_interpolate(re_t *re, array_t *args, dict_t *kwargs) {
  data_t *str = data_array_get(args, 0);
  str_t  *pat;

  (void) kwargs;
  if (debug_regexp) {
    debug("_regexp_interpolate(%s, %s, %s)",
	  regexp_tostring(re),
	  (args) ? array_tostring(args) : "[]",
          (kwargs) ? dict_tostring(kwargs) : "{}");
  }
  pat = str_format(str_chars(re -> pattern), args, kwargs);
  str_free(re -> pattern);
  re -> pattern = pat;
  if (re -> is_compiled) {
    regfree(&re -> compiled);
    re -> is_compiled = FALSE;
  }
  if (debug_regexp) {
    debug("_regexp_interpolate() => %s",
	  regexp_tostring(re));
  }
  return (data_t *) re;
}

data_t * _regexp_compile(re_t *re) {
  int retval;
  
  if (!re -> is_compiled) {
    if (retval = regcomp(&re -> compiled,
			 str_chars(re -> pattern), re -> re_flags)) {
      char msgbuf[str_len(re -> pattern)];
      
      regerror(retval, &re -> compiled, msgbuf, sizeof(msgbuf));
      debug("Error: %s", msgbuf);
      return data_exception(ErrorSyntax, msgbuf);
    }
    re -> is_compiled = TRUE;
  }
  return (data_t *) re;
}

/* ------------------------------------------------------------------------ */

re_t * regexp_create(char *pattern, char *flags) {
  re_t *ret = data_new(Regexp, re_t);

  ret -> pattern = str_printf( "(%s)", pattern);
  ret -> re_flags = REG_EXTENDED;
  if (flags) {
    ret -> flags = strdup(flags);
    if (strchr(ret -> flags, 'i')) {
      ret -> re_flags |= REG_ICASE;
    }
  }
  ret -> is_compiled = FALSE;
  if (debug_regexp) {
    debug("Created re %s", regexp_tostring(ret));
  }
  return ret;
}

re_t * regexp_vcreate(va_list args) {
  char *pattern;
  char *flags;

  pattern = va_arg(args, char *);
  flags = va_arg(args, char *);
  return regexp_create(pattern, flags);
}

int regexp_cmp(re_t *regex1, re_t *regex2) {
  return str_cmp(regex1 -> pattern, regex2 -> pattern);
}

data_t * regexp_match(re_t *re, char *str) {
  regmatch_t  rm[2];
  char       *ptr = str;
  int         len = strlen(str);
  char       *work = (char *) new(len + 1);
  array_t    *matches = data_array_create(4);
  data_t     *ret;
  char        msgbuf[100];
  int         retval;

  if (debug_regexp) {
    debug("%s .match(%s)", regexp_tostring(re), str);
  }
  if (data_is_exception(ret = _regexp_compile(re))) {
    return ret;
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
  re_t   *ret;

  (void) name;
  (void) kwargs;
  if (debug_regexp) {
    debug("_regexp_create(%s))", data_tostring(pattern));
  }
  flags = (array_size(args) == 2) ? data_tostring(data_array_get(args, 1)) : "";
  ret = regexp_create(data_tostring(pattern), flags);
  return (data_t *) ret;
}


data_t * _regexp_match(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  return _regexp_call(data_as_regexp(self), args, kwargs);
}

data_t * _regexp_replace(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *str = data_array_get(args, 0);
  data_t *replacements = data_array_get(args, 1);
  re_t   *re = data_as_regexp(self);

  (void) name;
  (void) kwargs;
  return regexp_replace(re, data_tostring(str), data_as_array(replacements));
}
