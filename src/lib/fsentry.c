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

typedef struct _fsentry_iter {
  data_t              _d;
  fsentry_t          *dir;
  DIR                *dirptr;
  struct dirent      *entryptr;
} fsentry_iter_t;

static void             _fsentry_init(void);
static fsentry_t *      _fsentry_new(fsentry_t *, va_list);
static void             _fsentry_free(fsentry_t *);
static data_t *         _fsentry_resolve(fsentry_t *, char *);
static fsentry_iter_t * _fsentry_iterator(fsentry_t *);
static void *           _fsentry_reduce_children(fsentry_t *, reduce_t, void *);

extern fsentry_t *      _fsentry_create(char *, arguments_t *);

static data_t *         _fsentry_open(data_t *, char *, arguments_t *);
static data_t *         _fsentry_isfile(data_t *, char *, arguments_t *);
static data_t *         _fsentry_isdir(data_t *, char *, arguments_t *);
static data_t *         _fsentry_exists(data_t *, char *, arguments_t *);

static vtable_t _vtable_FSEntry[] = {
  { .id = FunctionNew,         .fnc = (void_t) _fsentry_new },
  { .id = FunctionCmp,         .fnc = (void_t) fsentry_cmp },
  { .id = FunctionHash,        .fnc = (void_t) fsentry_hash },
  { .id = FunctionResolve,     .fnc = (void_t) _fsentry_resolve },
  { .id = FunctionIter,        .fnc = (void_t) _fsentry_iterator },
  { .id = FunctionReduce,      .fnc = (void_t) _fsentry_reduce_children },
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

static fsentry_iter_t * _fsentry_iter_new(fsentry_iter_t *, va_list);
static fsentry_iter_t * _fsentry_iter_readnext(fsentry_iter_t *);
static void             _fsentry_iter_free(fsentry_iter_t *);
static data_t *         _fsentry_iter_has_next(fsentry_iter_t *);
static data_t *         _fsentry_iter_next(fsentry_iter_t *);
static void *           _fsentry_iter_reduce_children(fsentry_iter_t *, reduce_t, void *);

static vtable_t _vtable_FSEntryIter[] = {
  { .id = FunctionNew,      .fnc = (void_t) _fsentry_iter_new },
  { .id = FunctionFree,     .fnc = (void_t) _fsentry_iter_free },
  { .id = FunctionHasNext,  .fnc = (void_t) _fsentry_iter_has_next },
  { .id = FunctionNext,     .fnc = (void_t) _fsentry_iter_next },
  { .id = FunctionReduce,   .fnc = (void_t) _fsentry_iter_reduce_children },
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

fsentry_t * _fsentry_new(fsentry_t *fse, va_list args) {
  char *name = va_arg(args, char *);

  data_set_static_string(fse, name);
  fse -> exists = (stat(name, &fse->statbuf)) ? errno : 0;
  return fse;
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

fsentry_iter_t * _fsentry_iterator(fsentry_t *dir) {
  fsentry_iter_t *ret = NULL;
  DIR            *d;

  d = opendir(fsentry_tostring(dir));
  if (d && ((intptr_t) d != -1L)) {
    ret = (fsentry_iter_t *) data_create(FSEntryIter, dir, d);
  }
  return ret;
}

void * _fsentry_reduce_children(fsentry_t *entry, reduce_t reducer, void *ctx) {
  return ctx;
}

/* -- F S E N T R Y _ T  P U B L I C  F U N C T I O N S ------------------- */

fsentry_t * fsentry_create(char *name) {
  _fsentry_init();
  return (fsentry_t *) data_create(FSEntry, name);
}

fsentry_t * fsentry_getentry(fsentry_t *dir, char *name) {
  char       n[MAX_PATH];
  fsentry_t *ret;

  if (!dir || !fsentry_isdir(dir) || !name) {
    return NULL;
  } else {
    strncpy(n, fsentry_tostring(dir), MAX_PATH-1);
    n[MAX_PATH-1] = 0;
    if (n[strlen(n) - 1] != '/') {
      strcat(n, "/");
    }
    strcat(n, name);
    ret = fsentry_create(n);
    return ret;
  }
}

unsigned int fsentry_hash(fsentry_t *e) {
  return strhash(fsentry_tostring(e));
}

int fsentry_cmp(fsentry_t *e1, fsentry_t *e2) {
  return strcmp(fsentry_tostring(e1), fsentry_tostring(e2));
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

  iter = _fsentry_iterator(dir);
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
  return file_open(fsentry_tostring(e));
}

/* -- F S E N T R Y I T E R ----------------------------------------------- */

fsentry_iter_t * _fsentry_iter_new(fsentry_iter_t *iter, va_list args) {
  fsentry_t *dir = va_arg(args, fsentry_t *);
  DIR       *dirptr = va_arg(args, DIR *);

  iter -> dir = dir;
  iter -> dirptr = dirptr;
  iter = _fsentry_iter_readnext(iter);
  data_set_static_string(iter, fsentry_tostring(dir));
  return iter;
}

fsentry_iter_t * _fsentry_iter_readnext(fsentry_iter_t *iter) {
  iter -> entryptr = readdir(iter -> dirptr);
  if (!iter -> entryptr) {
    closedir(iter -> dirptr);
    iter -> dirptr = NULL;
  }
  return iter;
}

void _fsentry_iter_free(fsentry_iter_t *iter) {
  if (iter && iter -> dirptr) {
    closedir(iter -> dirptr);
  }
}

data_t * _fsentry_iter_has_next(fsentry_iter_t *iter) {
  return int_as_bool(iter -> dirptr != NULL);
}

data_t * _fsentry_iter_next(fsentry_iter_t *iter) {
  data_t *ret;
  if (iter -> dirptr) {
    ret = data_create(FSEntry,
                      fsentry_getentry(iter -> dir,
                                       iter -> entryptr -> d_name));
    _fsentry_iter_readnext(iter);
  } else {
    ret = data_exception(ErrorExhausted, "Iterator exhausted");
  }
  return ret;
}

void * _fsentry_iter_reduce_children(fsentry_iter_t *iter, reduce_t reducer, void *ctx) {
  return reducer(iter->dir, ctx);
}

/* -- F S E N T R Y   D A T A   T Y P E   M E T H O D S ------------------- */

fsentry_t * _fsentry_create(_unused_ char *name, arguments_t *args) {
  data_t    *first_arg;
  fsentry_t *dir = NULL;

  first_arg = arguments_get_arg(args, 0);
  if (data_is_fsentry(first_arg)) {
    dir = data_as_fsentry(first_arg);
  } else {
    dir = fsentry_create(arguments_arg_tostring(args, 0));
  }
  if (dir && (arguments_args_size(args) == 1)) {
    return dir;
  } else {
    return fsentry_getentry(dir, arguments_arg_tostring(args, 1));
  }
}

data_t * _fsentry_open(_unused_ data_t *e, _unused_ char *n, arguments_t *args) {
  data_t *ret = (data_t *) fsentry_open((fsentry_t *) e);

  return (ret) ? ret : data_null();
}

data_t * _fsentry_isfile(_unused_ data_t *e, _unused_ char *n, arguments_t *args) {
  return int_as_bool(fsentry_isfile((fsentry_t *) e));
}

data_t * _fsentry_isdir(_unused_ data_t *e, _unused_ char *n, arguments_t *args) {
  return int_as_bool(fsentry_isdir((fsentry_t *) e));
}

data_t * _fsentry_exists(_unused_ data_t *e, _unused_ char *n, arguments_t *args) {
  return int_as_bool(fsentry_exists((fsentry_t *) e));
}
