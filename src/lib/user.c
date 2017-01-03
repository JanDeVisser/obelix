/*
 * user.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "libcore.h"
#include <data.h>
#include <exception.h>
#include <str.h>
#include <user.h>

typedef enum {
  UID,
  Username,
  CurrentUser
} user_new_t;

static data_t * _user_new(user_t *, va_list);
static void     _user_free(user_t *);
static char *   _user_tostring(user_t *);
static data_t * _user_resolve(user_t *, char *);

static vtable_t _vtable_User[] = {
  { .id = FunctionNew,         .fnc = (void_t) _user_new },
  { .id = FunctionFree,        .fnc = (void_t) _user_free },
  { .id = FunctionToString,    .fnc = (void_t) _user_tostring },
  { .id = FunctionResolve,     .fnc = (void_t) _user_resolve },
  { .id = FunctionHash,        .fnc = (void_t) user_hash },
  { .id = FunctionCmp,         .fnc = (void_t) user_cmp },
  { .id = FunctionNone,        .fnc = NULL }
};

int User = -1;
static int _getpwnam_bufsize = -1;

/* ------------------------------------------------------------------------ */

void _user_init(void) {
  if (User < 1) {
    typedescr_register(User, user_t);
    if ((_getpwnam_bufsize = sysconf(_SC_GETPW_R_SIZE_MAX)) < 0) {
      _getpwnam_bufsize = 1024;
    }
  }
}

/* ------------------------------------------------------------------------ */

data_t * _user_new(user_t *user, va_list args) {
  user_new_t     which_user = va_arg(args, user_new_t);
  data_t        *ret = (data_t *) user;
  struct passwd  pwd;
  struct passwd *result;
  int            err = 0;
  uid_t          uid;
  char           buf[_getpwnam_bufsize];
  char          *home = NULL;

  switch (which_user) {
    case UID:
      err = getpwuid_r(va_arg(args, uid_t), &pwd, buf, _getpwnam_bufsize, &result);
      break;
    case Username:
      err = getpwnam_r(va_arg(args, char *), &pwd, buf, _getpwnam_bufsize, &result);
      break;
    case CurrentUser:
      uid = geteuid();
      home = getenv("HOME");
      err = getpwuid_r(uid, &pwd, buf, _getpwnam_bufsize, &result);
      break;
  }
  if (!err) {
    user -> uid = pwd.pw_uid;
    user -> name = strdup(pwd.pw_name);
    user -> fullname = strdup(pwd.pw_gecos);
    user -> home_dir = strdup((home && *home) ? home : pwd.pw_dir);
  } else {
    ret = data_exception_from_my_errno(err);
  }
  return ret;
}

void _user_free(user_t *user) {
  if (user) {
    free(user -> name);
    free(user -> fullname);
    free(user -> home_dir);
  }
}

char * _user_tostring(user_t *user) {
  return (user -> fullname && *(user -> fullname)) ? user -> fullname : user -> name;
}

data_t * _user_resolve(user_t *user, char *name) {
  if (!strcmp(name, "uid")) {
    return int_to_data((int) user -> uid);
  } else if (!strcmp(name, "username")) {
    return str_to_data(user -> name);
  } else if (!strcmp(name, "fullname")) {
    return str_to_data(user -> fullname);
  } else if (!strcmp(name, "directory")) {
    return str_to_data(user -> home_dir);
  } else {
    return NULL;
  }
}

/* ------------------------------------------------------------------------ */

data_t * current_user(void) {
  _user_init();
  return data_create(User, CurrentUser);
}

data_t * create_user_byname(char *name) {
  _user_init();
  return data_create(User, Username, name);
}

data_t * create_user_byuid(uid_t uid) {
  _user_init();
  return data_create(User, UID, uid);
}

int user_cmp(user_t *user1, user_t *user2) {

  return strcmp(user1 -> name, user2 -> name);
}

unsigned int user_hash(user_t *user) {
  return strhash(user -> name);
}
