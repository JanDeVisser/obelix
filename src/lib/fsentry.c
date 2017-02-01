/*
 * /obelix/src/lib/fsentry.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include "libcore.h"
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <exception.h>
#include <file.h>
#include <fsentry.h>

static void     _fsentry_init(void);
static void     _fsentry_free(fsentry_t *);
static char *   _fsentry_allocstring(fsentry_t *);
static data_t * _fsentry_resolve(fsentry_t *, char *);

extern data_t * _fsentry_create(char *, arguments_t *);

static data_t * _fsentry_open(data_t *, char *, arguments_t *);
static data_t * _fsentry_isfile(data_t *, char *, arguments_t *);
static data_t * _fsentry_isdir(data_t *, char *, arguments_t *);
static data_t * _fsentry_exists(data_t *, char *, arguments_t *);

static vtable_t _vtable_FSEntry[] = {
  { .id = FunctionCmp,         .fnc = (void_t) fsentry_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _fsentry_free },
  { .id = FunctionAllocString, .fnc = (void_t) _fsentry_allocstring },
  { .id = FunctionHash,        .fnc = (void_t) fsentry_hash },
  { .id = FunctionResolve,     .fnc = (void_t) _fsentry_resolve },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_FSEntry[] = {
  { .type = -1,     .name = "open",    .method = _fsentry_open,   .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 },
  { .type = -1,     .name = "isfile",  .method = _fsentry_isfile, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 },
  { .type = -1,     .name = "isdir",   .method = _fsentry_isdir,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 },
  { .type = -1,     .name = "exists",  .method = _fsentry_exists, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,            .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 }
};

static int FSEntry = -1;

/* ------------------------------------------------------------------------ */

typedef struct _fsentry_iter {
  data_t              _d;
  fsentry_t          *dir;
#ifdef HAVE_DIRENT_H
  DIR                *dirptr;
  struct dirent      *entryptr;
#elif HAVE__FINDFIRST
  intptr_t            dirptr;
  struct _finddata_t  entry;
#endif /* HAVE_DIRENT_H / HAVE__FINDFIRST */
} fsentry_iter_t;

static fsentry_iter_t * _fsentry_iter_readnext(fsentry_iter_t *);
static fsentry_iter_t * _fsentry_iter_create(fsentry_t *);
static void             _fsentry_iter_free(fsentry_iter_t *);
static char *           _fsentry_iter_tostring(fsentry_iter_t *);
static data_t *         _fsentry_iter_has_next(fsentry_iter_t *);
static data_t *         _fsentry_iter_next(fsentry_iter_t *);

static vtable_t _vtable_FSEntryIter[] = {
  { .id = FunctionFree,     .fnc = (void_t) _fsentry_iter_free },
  { .id = FunctionToString, .fnc = (void_t) _fsentry_iter_tostring },
  { .id = FunctionHasNext,  .fnc = (void_t) _fsentry_iter_has_next },
  { .id = FunctionNext,     .fnc = (void_t) _fsentry_iter_next },
  { .id = FunctionNone,     .fnc = NULL }
};

int FSEntryIter = -1;

/* ------------------------------------------------------------------------ */

void _fsentry_init(void) {
  if (FSEntry < 1) {
    typedescr_register_with_methods(FSEntry, fsentry_t);
    typedescr_register(FSEntryIter, fsentry_iter_t);
  }
}

/* -- F S E N T R Y _ T  S T A T I C   F U N C T I O N S ------------------ */

void _fsentry_free(fsentry_t *e) {
  if (e) {
    free(e -> name);
  }
}

char * _fsentry_allocstring(fsentry_t *e) {
  return strdup(e -> name);
}

data_t * _fsentry_resolve(fsentry_t *entry, char *name) {
  fsentry_t *e = NULL;

  if (!strcmp(name, "name")) {
   return str_to_data(fsentry_tostring(entry)) ;
  } else {
    e = fsentry_getentry(entry, name);
    if (e) {
      if (!fsentry_exists(e)) {
        fsentry_free(e);
        e = NULL;
      }
    }
    return (e) ? (data_t *) e : data_null();
  }
}

/* -- F S E N T R Y _ T  P U B L I C  F U N C T I O N S ------------------- */

fsentry_t * fsentry_create(char *name) {
  fsentry_t *ret;

  _fsentry_init();
  ret = data_new(FSEntry, fsentry_t);
  ret -> name = strdup(name);
  ret -> exists = (stat(ret->name, &ret->statbuf)) ? errno : 0;
  return ret;
}

fsentry_t * fsentry_getentry(fsentry_t *dir, char *name) {
  char       n[MAX_PATH];
  fsentry_t *ret;

  if (!dir || !fsentry_isdir(dir) || !name) {
    return NULL;
  } else {
    strcpy(n, dir -> name);
    if (n[strlen(n) - 1] != '/') {
      strcat(n, "/");
    }
    strcat(n, name);
    ret = fsentry_create(n);
    return ret;
  }
}

unsigned int fsentry_hash(fsentry_t *e) {
  return strhash(e -> name);
}

int fsentry_cmp(fsentry_t *e1, fsentry_t *e2) {
  return strcmp(e1 -> name, e2 -> name);
}

int fsentry_exists(fsentry_t *e) {
  return e -> exists == 0;
}

int fsentry_isfile(fsentry_t *e) {
  return !e -> exists && (e -> statbuf.st_mode & S_IFREG);
}

int fsentry_isdir (fsentry_t *e) {
  return !e -> exists && (e -> statbuf.st_mode & S_IFDIR);
}

int fsentry_canread(fsentry_t *e) {
  return !e -> exists && (e -> statbuf.st_mode & S_IREAD);
}

int fsentry_canwrite(fsentry_t *e) {
  return !e -> exists && (e -> statbuf.st_mode & S_IWRITE);
}

int fsentry_canexecute(fsentry_t *e) {
  return !e -> exists && (e -> statbuf.st_mode & S_IEXEC);
}

list_t * fsentry_getentries(fsentry_t *dir) {
  list_t         *ret = NULL;
  fsentry_iter_t *iter;
  data_t         *d;

  iter = _fsentry_iter_create(dir);
  if (iter) {
    ret = data_list_create();
    for (d = _fsentry_iter_has_next(iter);
         data_intval(d);
         d = _fsentry_iter_has_next(iter)) {
      data_free(d);
      list_push(ret, _fsentry_iter_next(iter));
    }
  }
  return ret;
}

file_t * fsentry_open(fsentry_t *e) {
  if (!fsentry_isfile(e)) {
    return NULL;
  }
  return file_open(e -> name);
}

/* -- F S E N T R Y I T E R ----------------------------------------------- */

fsentry_iter_t * _fsentry_iter_readnext(fsentry_iter_t *iter) {
#ifdef HAVE_DIRENT_H
  iter -> entryptr = readdir(iter -> dirptr);
  if (!iter -> entryptr) {
    closedir(iter -> dirptr);
    iter -> dirptr = NULL;
#elif defined(HAVE__FINDFIRST)
  if (_findnext(iter -> dirptr, &iter -> entry)) {
    _findclose(iter -> dirptr);
    iter -> dirptr = 0;
#endif
  }
  return iter;
}

fsentry_iter_t * _fsentry_iter_create(fsentry_t *dir) {
  fsentry_iter_t *ret = NULL;
#ifdef HAVE_DIRENT_H
  DIR            *d;
#elif defined(HAVE__FINDFIRST)
  intptr_t            d;
  struct _finddata_t  attrib;
#endif

#ifdef HAVE_DIRENT_H
  d = opendir(dir -> name);
#elif defined(HAVE__FINDFIRST)
  d =_findfirst("*.*", &attrib);
#endif
  if (d && ((intptr_t) d != -1L)) {
    ret = data_new(FSEntryIter, fsentry_iter_t);
    ret -> dir = dir;
    ret -> dirptr = d;
#ifdef HAVE_DIRENT_H
    ret = _fsentry_iter_readnext(ret);
#elf defined(HAVE__FINDFIRST)
    memcpy(&ret -> entry, &attrib, sizeof(struct _finddata_t));
  } else {
    ret -> dirptr = 0;
#endif
  }
  return ret;
}

void _fsentry_iter_free(fsentry_iter_t *iter) {
  if (iter) {
    if (iter -> dirptr) {
#ifdef HAVE_DIRENT_H
      closedir(iter -> dirptr);
#elif defined(HAVE__FINDFIRST)
      _findclose(iter -> dirptr);
#endif
    }
    free(iter);
  }
}

char * _fsentry_iter_tostring(fsentry_iter_t *iter) {
  return fsentry_tostring(iter -> dir);
}

data_t * _fsentry_iter_has_next(fsentry_iter_t *iter) {
#ifdef HAVE_DIRENT_H
  return int_as_bool(iter -> dirptr != NULL);
#elif defined(HAVE__FINDFIRST)
  return int_as_bool(iter -> dirptr != 0);
#endif
}

data_t * _fsentry_iter_next(fsentry_iter_t *iter) {
  data_t *ret;
  if (iter -> dirptr) {
    ret = data_create(FSEntry,
                      fsentry_getentry(iter -> dir,
#ifdef HAVE_DIRENT_H
                                       iter -> entryptr -> d_name));
#elif defined(HAVE__FINDFIRST)
                                       iter -> entry.name));
#endif
    _fsentry_iter_readnext(iter);
  } else {
    ret = data_exception(ErrorExhausted, "Iterator exhausted");
  }
  return ret;
}

/* -- F S E N T R Y   D A T A   T Y P E   M E T H O D S ------------------- */

data_t * _fsentry_create(char *name, arguments_t *args) {
  (void) name;

  return (data_t *) fsentry_create(arguments_arg_tostring(args, 0));
}

data_t * _fsentry_open(data_t *e, char *n, arguments_t *args) {
  data_t *ret = (data_t *) fsentry_open((fsentry_t *) e);

  (void) n;
  (void) args;
  return (ret) ? ret : data_null();
}

data_t * _fsentry_isfile(data_t *e, char *n, arguments_t *args) {
  (void) n;
  (void) args;
  return int_as_bool(fsentry_isfile((fsentry_t *) e));
}

data_t * _fsentry_isdir(data_t *e, char *n, arguments_t *args) {
  (void) n;
  (void) args;
  return int_as_bool(fsentry_isdir((fsentry_t *) e));
}

data_t * _fsentry_exists(data_t *e, char *n, arguments_t *args) {
  (void) n;
  (void) args;
  return int_as_bool(fsentry_exists((fsentry_t *) e));
}
