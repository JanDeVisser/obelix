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

#include <file.h>

static void     _fsentry_init(void) __attribute__((constructor));
static void     _fsentry_free(fsentry_t *);
static char *   _fsentry_tostring(fsentry_t *);
static data_t * _fsentry_resolve(fsentry_t *, char *);

static data_t * _fsentry_open(data_t *, char *, array_t *, dict_t *);
static data_t * _fsentry_isfile(data_t *, char *, array_t *, dict_t *);
static data_t * _fsentry_isdir(data_t *, char *, array_t *, dict_t *);
static data_t * _fsentry_exists(data_t *, char *, array_t *, dict_t *);

static vtable_t _fsentry_string[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionCmp,      .fnc = (void_t) fsentry_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _fsentry_free },
  { .id = FunctionToString, .fnc = (void_t) _fsentry_tostring },
  { .id = FunctionHash,     .fnc = (void_t) fsentry_hash },
  { .id = FunctionResolve,  .fnc = (void_t) _fsentry_resolve },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methoddescr_fsentry[] = {
  { .type = FSEntry, .name = "open",    .method = _fsentry_open,   .argtypes = { NoType, NoType, NoType }, .minargs = 0, maxargs = 0, .varargs = 0 },
  { .type = FSEntry, .name = "isfile",  .method = _fsentry_isfile, .argtypes = { NoType, NoType, NoType }, .minargs = 0, maxargs = 0, .varargs = 0 },
  { .type = FSEntry, .name = "isdir",   .method = _fsentry_isdir,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, maxargs = 0, .varargs = 0 },
  { .type = FSEntry, .name = "exists",  .method = _fsentry_exists, .argtypes = { NoType, NoType, NoType }, .minargs = 0, maxargs = 0, .varargs = 0 },
  { .type = NoType,  .name = NULL,      .method = NULL,            .argtypes = { NoType, NoType, NoType }, .minargs = 0, maxargs = 0, .varargs = 0 }
};

/* ------------------------------------------------------------------------ */

void _fsentry_init(void) {
  typedescr_create_and_register("fsentry", _vtable_fsentry, _methoddescr_str);
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

data_t * _fsentry_resolve(fsentry_t *dir, char *name) {
  fsentry_t *e = fsentry_getentry(dir, name);
  
  if (e) {
    if (!fsentry_exists(e)) {
      fsentry_free(e);
      e = NULL;
    }
  }
  return (e) ? e : data_null();
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
  list_t        *ret;
  DIR           *dirpointer;
  struct dirent *entrypointer;
  
  if (!fsentry_isdir(dir)) {
    return NULL;
  }
  
  ret = NULL;
  dirpointer = opendir(dir -> name);
  if (dirpointer) {
    ret = list_create();
    list_set_free(ret, fsentry_free);
    list_set_hash(ret, fsentry_hash);
    list_set_tostring(ret, fsentry_tostring);
    list_set_cmp(ret, fsentry_cmp);
    for (entrypointer = readdir(dirpointer);
         entrypointer;
    entrypointer = readdir(dirpointer)) {
      list_push(ret, fsentry_getentry(dir, entrypointer -> d_name));
    }
    closedir (dirpointer);
  }
  return ret;
}

file_t * fsentry_open(fsentry_t *e) {
  if (!fsentry_isfile(e)) {
    return NULL;
  }
  return file_open(e -> name);
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
