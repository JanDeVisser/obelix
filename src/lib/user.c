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

#include <core.h>
#include <user.h>

user_t * user_create(char *uid) {
  user_t        *ret = NULL;
  struct passwd  pwd;
  struct passwd *result;
  int            err;
  char           buf[1024];
  
  err = getpwnam_r(uid, &pwd, buf, 1024, &result);
  if (!err && result) {
    ret = NEW(user_t);
    ret -> uid = strdup(uid);
    ret -> name = strdup(pwd.pw_gecos);
    ret -> home_dir = strdup(pwd.pw_dir);
    ret -> refs = 1;
  }
  return ret;
}

void user_free(user_t *user) {
  if (user) {
    user -> refs--;
    if (user -> refs <= 0) {
      free(user -> uid);
      free(user -> name);
      free(user -> home_dir);
      free(user);
    }
  }
}

int user_cmp(user_t *user1, user_t *user2) {
  return strcmp(user1 -> uid, user2 -> uid);
}

unsigned int user_hash(user_t *user) {
  return strhash(user -> uid);
}

char * user_tostring(user_t *user) {
  return (user -> name && user -> name[0]) ? user -> name : user -> uid;
}

