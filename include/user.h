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

#ifndef __USER_H__
#define	__USER_H__

typedef struct _user {
  char   *uid;
  char   *name;
  char   *home_dir;
  int     refs;
} user_t;

extern user_t *     user_create(char *);
extern void         user_free(user_t *);
extern int          user_cmp(user_t *, user_t *);
extern unsigned int user_hash(user_t *);
extern char *       user_tostring(user_t *);

#endif /* __USER_H__ */

