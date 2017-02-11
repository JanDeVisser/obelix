/*
 * user.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifdef HAVE_GETPWNAM
#include <pwd.h>
#elif defined(HAVE_GETUSERNAME)
#include <windows.h>
#endif
#include <sys/types.h>

#include <data.h>

#ifndef __USER_H__
#define	__USER_H__

typedef struct _user {
  data_t  _d;
#ifdef HAVE_GETPWNAM
  uid_t   uid;
#elif defined(HAVE_GETUSERNAME)
  PSID    sid;
#endif
  char   *name;
  char   *fullname;
  char   *home_dir;
} user_t;

OBLCORE_IMPEXP data_t *     current_user(void);
OBLCORE_IMPEXP data_t *     create_user_byuid(uid_t);
OBLCORE_IMPEXP data_t *     create_user_byname(char *);
OBLCORE_IMPEXP int          user_cmp(user_t *, user_t *);
OBLCORE_IMPEXP unsigned int user_hash(user_t *);

OBLCORE_IMPEXP int User;

#define data_is_user(d)     ((d) && data_hastype((d), User))
#define data_as_user(d)     (data_is_user((d)) ? ((user_t *) (d)) : NULL)
#define user_copy(u)        ((user_t *) data_copy((data_t *) (u)))
#define user_tostring(u)    (data_tostring((data_t *) (u))
#define user_free(u)        (data_free((data_t *) (u)))

#endif /* __USER_H__ */
