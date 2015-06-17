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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <exception.h>
#include <file.h>
#include <fsentry.h>

static void     _fsentry_init(void) __attribute__((constructor));
static void     _fsentry_free(fsentry_t *);
static char *   _fsentry_tostring(fsentry_t *);
static data_t * _fsentry_resolve(fsentry_t *, char *);

static data_t * _fsentry_open(data_t *, char *, array_t *, dict_t *);
static data_t * _fsentry_isfile(data_t *, char *, array_t *, dict_t *);
static data_t * _fsentry_isdir(data_t *, char *, array_t *, dict_t *);
static data_t * _fsentry_exists(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_fsentry[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionCmp,      .fnc = (void_t) fsentry_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _fsentry_free },
  { .id = FunctionToString, .fnc = (void_t) _fsentry_tostring },
  { .id = FunctionHash,     .fnc = (void_t) fsentry_hash },
  { .id = FunctionResolve,  .fnc = (void_t) _fsentry_resolve },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methoddescr_fsentry[] = {
  { .type = -1,     .name = "open",    .method = _fsentry_open,   .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 },
  { .type = -1,     .name = "isfile",  .method = _fsentry_isfile, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 },
  { .type = -1,     .name = "isdir",   .method = _fsentry_isdir,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 },
  { .type = -1,     .name = "exists",  .method = _fsentry_exists, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,            .argtypes = { NoType, NoType, NoType }, .minargs = 0, .maxargs = 0, .varargs = 0 }
};

int FSEntry = -1;

/* ------------------------------------------------------------------------ */

typedef struct _fsentry_iter {
  data_t         _d;
  fsentry_t     *dir;
  DIR           *dirptr;
  struct dirent *entryptr;
} fsentry_iter_t;

static fsentry_iter_t * _fsentry_iter_readnext(fsentry_iter_t *);
static fsentry_iter_t * _fsentry_iter_create(fsentry_t *);
static void             _fsentry_iter_free(fsentry_iter_t *);
static char *           _fsentry_iter_tostring(fsentry_iter_t *);
static data_t *         _fsentry_iter_has_next(fsentry_iter_t *);
static data_t *         _fsentry_iter_next(fsentry_iter_t *);

static vtable_t _vtable_fsentry_iter[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionFree,     .fnc = (void_t) _fsentry_iter_free },
  { .id = FunctionToString, .fnc = (void_t) _fsentry_iter_tostring },
  { .id = FunctionHasNext,  .fnc = (void_t) _fsentry_iter_has_next },
  { .id = FunctionNext,     .fnc = (void_t) _fsentry_iter_next },
  { .id = FunctionNone,     .fnc = NULL }
};

int FSEntryIter = -1;

/* ------------------------------------------------------------------------ */

void _fsentry_init(void) {
  FSEntry = typedescr_create_and_register(FSEntry, "fsentry",
					  _vtable_fsentry, _methoddescr_fsentry);
  FSEntryIter = typedescr_create_and_register(FSEntryIter, "fsentryiter",
					      _vtable_fsentry_iter, NULL);
}

/* -- F S E N T R Y _ T  S T A T I C   F U N C T I O N S ------------------ */

void _fsentry_free(fsentry_t *e) {
  if (e) {
    free(e -> name);
    free(e);
  }
}

char * _fsentry_tostring(fsentry_t *e) {
  if (!e -> _d.str) {
    e -> _d.str = strdup(e -> name);
  }
  return NULL;
}

data_t * _fsentry_resolve(fsentry_t *entry, char *name) {
  fsentry_t *e = NULL;

  if (!strcmp(name, "name")) {
   return data_create(String, fsentry_tostring(entry)) ;
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

fsentry_t * fsentry_create (char *name) {
  fsentry_t *ret;

  ret = data_new(FSEntry, fsentry_t);
  ret -> name = strdup(name);
  ret -> exists = (stat(ret->name, &ret->statbuf)) ? errno : 0;
  return ret;
}

fsentry_t * fsentry_getentry(fsentry_t *dir, char *name) {
  char      *n;
  fsentry_t *ret;

  if (!fsentry_isdir (dir)) {
    return NULL;
  }
  asprintf(&n, "%s/%s", dir -> name, name);
  ret = fsentry_create(n);
  free(n);
  return ret;
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
  return !access(e -> name, R_OK);
}

int fsentry_canwrite(fsentry_t *e) {
  return !access(e -> name, W_OK);
}

int fsentry_canexecute(fsentry_t *e) {
  return !access(e -> name, X_OK);
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
  iter -> entryptr = readdir(iter -> dirptr);
  if (!iter -> entryptr) {
    closedir(iter -> dirptr);
    iter -> dirptr = NULL;
  }
  return iter;
}

fsentry_iter_t * _fsentry_iter_create(fsentry_t *dir) {
  fsentry_iter_t *ret = NULL;
  DIR            *d;

  d = opendir(dir -> name);
  if (d) {
    ret = data_new(FSEntryIter, fsentry_iter_t);
    ret -> dir = dir;
    ret -> dirptr = d;
    ret = _fsentry_iter_readnext(ret);
  }
  return ret;
}

void _fsentry_iter_free(fsentry_iter_t *iter) {
  if (iter) {
    if (iter -> dirptr) {
      closedir(iter -> dirptr);
    }
    free(iter);
  }
}

char * _fsentry_iter_tostring(fsentry_iter_t *iter) {
  return fsentry_tostring(iter -> dir);
}

data_t * _fsentry_iter_has_next(fsentry_iter_t *iter) {
  return data_create(Bool, iter -> entryptr != NULL);
}

data_t * _fsentry_iter_next(fsentry_iter_t *iter) {
  if (iter -> entryptr) {
    return data_create(FSEntry,
                       fsentry_getentry(iter -> dir,
                                        iter -> entryptr -> d_name));
  } else {
    return data_exception(ErrorExhausted, "Iterator exhausted");
  }
}

/* -- F S E N T R Y   D A T A   T Y P E   M E T H O D S ------------------- */

data_t * _fsentry_open(data_t *e, char *n, array_t *args, dict_t *kwargs) {
  data_t *ret = (data_t *) fsentry_open((fsentry_t *) e);

  (void) n;
  (void) args;
  (void) kwargs;
  return (ret) ? ret : data_null();
}

data_t * _fsentry_isfile(data_t *e, char *n, array_t *args, dict_t *kwargs) {
  (void) n;
  (void) args;
  (void) kwargs;
  return data_create(Bool, fsentry_isfile((fsentry_t *) e));
}

data_t * _fsentry_isdir(data_t *e, char *n, array_t *args, dict_t *kwargs) {
  (void) n;
  (void) args;
  (void) kwargs;
  return data_create(Bool, fsentry_isdir((fsentry_t *) e));
}

data_t * _fsentry_exists(data_t *e, char *n, array_t *args, dict_t *kwargs) {
  (void) n;
  (void) args;
  (void) kwargs;
  return data_create(Bool, fsentry_exists((fsentry_t *) e));
}
