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

#include "oblconfig.h"

#if defined(HAVE_GETPWNAM) && defined(HAVE_UNISTD_H)
  #include <unistd.h>
#elif defined(HAVE_GETUSERNAME)
  #define SECURITY_WIN32
  #include <windows.h>
  #include <sddl.h>
  #include <security.h>
  #include <secext.h>
#endif

#include <string.h>
#include <sys/types.h>

#include "libcore.h"
#include <data.h>
#include <user.h>

typedef enum {
  UID,
  Username,
  CurrentUser
} user_new_t;

#ifdef HAVE_PWD_H
static data_t * _user_new_getpwnam(user_t *, va_list);
#elif defined(HAVE_GETUSERNAME)
static data_t * _user_new_getusername(user_t *, va_list);
#endif
static void     _user_free(user_t *);
static char *   _user_tostring(user_t *);
static data_t * _user_resolve(user_t *, char *);

static vtable_t _vtable_User[] = {
#ifdef HAVE_GETPWNAM
  { .id = FunctionNew,         .fnc = (void_t) _user_new_getpwnam },
#elif defined(HAVE_GETUSERNAME)
  { .id = FunctionNew,         .fnc = (void_t) _user_new_getusername },
#endif
  { .id = FunctionFree,        .fnc = (void_t) _user_free },
  { .id = FunctionToString,    .fnc = (void_t) _user_tostring },
  { .id = FunctionResolve,     .fnc = (void_t) _user_resolve },
  { .id = FunctionHash,        .fnc = (void_t) user_hash },
  { .id = FunctionCmp,         .fnc = (void_t) user_cmp },
  { .id = FunctionNone,        .fnc = NULL }
};

int User = -1;
#ifdef HAVE_GETPWNAM
static int _getpwnam_bufsize = -1;
#endif

/* ------------------------------------------------------------------------ */

void _user_init(void) {
  if (User < 1) {
    typedescr_register(User, user_t);
#if defined(HAVE_GETPWNAM) && defined(HAVE_SYSCONF)
    if ((_getpwnam_bufsize = sysconf(_SC_GETPW_R_SIZE_MAX)) < 0) {
      _getpwnam_bufsize = 1024;
    }
#endif
  }
}

/* ------------------------------------------------------------------------ */

#ifdef HAVE_GETPWNAM

data_t * _user_new_getpwnam(user_t *user, va_list args) {
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

#elif defined(HAVE_GETUSERNAME) /* !HAVE_PWD_H */

data_t * _user_new_getusername(user_t *user, va_list args) {
  user_new_t    which_user = va_arg(args, user_new_t);
  data_t       *ret = NULL;
  char          buf[1024];
  DWORD         sz;
  DWORD         zero = 0;
  SID_NAME_USE  use;

  switch (which_user) {
    case UID:
      ret = data_exception(ErrorInternalError, "Cannot look up user by UID on Windows");
      break;
    case Username:
      ret = data_exception(ErrorInternalError, "Cannot look up user by name on Windows");
      break;
    case CurrentUser:
      sz = 1024;
      if (!GetUserName(buf, &sz)) {
        ret = data_exception_from_my_errno(GetLastError());
      }
      if (!ret) {
        user -> name = strdup(buf);
        if (!GetUserNameEx(NameDisplay, buf, &sz)) {
          sz = 1024;
          ret = data_exception_from_my_errno(GetLastError());
        }
      }
      if (!ret) {
        user -> fullname = strdup(buf);
        sz = 1024;
        if (!LookupAccountName(NULL, user -> name, buf, &sz, NULL, &zero, &use)) {
          ret = data_exception_from_my_errno(GetLastError());
        }
      }
      if (!ret) {
        user -> sid = malloc(sz);
        memcpy(user -> sid, buf, sz);
      }
      asprintf(&user -> home_dir, "%s%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
      break;
  }
  if (!ret) {
    ret = (data_t *) user;
  }
  return ret;
}

#endif

void _user_free(user_t *user) {
  if (user) {
    free(user -> name);
    free(user -> fullname);
    free(user -> home_dir);
#ifdef HAVE_GETUSERNAME
    free(user -> sid);
#endif
  }
}

char * _user_tostring(user_t *user) {
  return (user -> fullname && *(user -> fullname)) ? user -> fullname : user -> name;
}

data_t * _user_resolve(user_t *user, char *name) {
  data_t *ret;

  if (!strcmp(name, "uid")) {
#ifdef HAVE_GETPWNAM
    return int_to_data((int) user -> uid);
#elif (HAVE_GETUSERNAME)
    char *sid;

    if (!ConvertSidToStringSid(user -> sid, &sid)) {
      ret = data_exception_from_my_errno(GetLastError());
    } else {
      ret = str_to_data(sid);
      LocalFree(sid);
    }
    return ret;
#endif
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

#ifdef HAVE_GETPWNAM
data_t * create_user_byname(char *name) {
  _user_init();
  return data_create(User, Username, name);
}

data_t * create_user_byuid(uid_t uid) {
  _user_init();
  return data_create(User, UID, uid);
}
#endif

int user_cmp(user_t *user1, user_t *user2) {
  return strcmp(user1 -> name, user2 -> name);
}

unsigned int user_hash(user_t *user) {
  return strhash(user -> name);
}
